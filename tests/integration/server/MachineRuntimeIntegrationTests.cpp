#include <gtest/gtest.h>

#include <Config.hpp>
#include <Machine.hpp>
#include <MachineRuntime.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include <yaml-cpp/yaml.h>
#include <zmq.hpp>

using namespace std::chrono_literals;

namespace {

struct Endpoints {
  std::string statusAddress;
  std::string commandAddress;
};

struct ScopedConfig {
  std::filesystem::path path;
  std::filesystem::path statusSocketPath;
  std::filesystem::path commandSocketPath;
  Endpoints endpoints;
};

ScopedConfig writeIntegrationConfig() {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto suffix = std::to_string(stamp);
  const auto runDir = std::filesystem::current_path();
  const auto path = runDir /
                    ("rimokun_machine_runtime_it_" + suffix + ".yaml");
  const auto statusSockName = "rits_" + suffix + ".sock";
  const auto commandSockName = "ritc_" + suffix + ".sock";
  const auto statusSockPath = runDir / statusSockName;
  const auto commandSockPath = runDir / commandSockName;

  const auto statusAddress = "ipc://" + statusSockName;
  const auto commandAddress = "ipc://" + commandSockName;

  std::ofstream out(path);
  out << "classes:\n";
  out << "  RimoServer:\n";
  out << "    statusAddress: \"" << statusAddress << "\"\n";
  out << "    commandAddress: \"" << commandAddress << "\"\n";
  out << "  Contec:\n";
  out << "    ipAddress: \"127.0.0.1\"\n";
  out << "    port: 1502\n";
  out << "    slaveId: 1\n";
  out << "    nDI: 16\n";
  out << "    nDO: 8\n";
  out << "  ControlPanel:\n";
  out << "    comm:\n";
  out << "      type: \"serial\"\n";
  out << "      serial:\n";
  out << "        port: \"/dev/null\"\n";
  out << "        baudRate: \"BAUD_115200\"\n";
  out << "    processing:\n";
  out << "      movingAverageDepth: 5\n";
  out << "      baselineSamples: 10\n";
  out << "      buttonDebounceSamples: 2\n";
  out << "  MotorControl:\n";
  out << "    model: \"AR-KD2\"\n";
  out << "    device: \"/dev/null\"\n";
  out << "    baud: 115200\n";
  out << "    parity: \"E\"\n";
  out << "    dataBits: 8\n";
  out << "    stopBits: 1\n";
  out << "    responseTimeoutMS: 1000\n";
  out << "    motorAddresses:\n";
  out << "      XLeft: 1\n";
  out << "      XRight: 2\n";
  out << "      YLeft: 3\n";
  out << "      YRight: 4\n";
  out << "      ZLeft: 5\n";
  out << "      ZRight: 6\n";
  out << "  Machine:\n";
  out << "    loopIntervalMS: 10\n";
  out << "    updateIntervalMS: 50\n";
  out << "    inputMapping:\n";
  out << "      button1: 0\n";
  out << "      button2: 1\n";
  out << "    outputMapping:\n";
  out << "      toolChangerLeft: 0\n";
  out << "      toolChangerRight: 1\n";
  out << "      light1: 2\n";
  out << "      light2: 3\n";
  out.close();
  return {.path = path,
          .statusSocketPath = statusSockPath,
          .commandSocketPath = commandSockPath,
          .endpoints = {.statusAddress = statusAddress,
                        .commandAddress = commandAddress}};
}

std::optional<YAML::Node> sendCommand(const std::string& address,
                                      const YAML::Node& command,
                                      const std::chrono::milliseconds timeout =
                                          1200ms) {
  zmq::context_t context{1};
  zmq::socket_t req{context, zmq::socket_type::req};
  req.set(zmq::sockopt::sndtimeo, static_cast<int>(timeout.count()));
  req.set(zmq::sockopt::rcvtimeo, static_cast<int>(timeout.count()));
  req.connect(address);
  req.send(zmq::buffer(YAML::Dump(command)));

  zmq::message_t reply;
  if (!req.recv(reply)) {
    return std::nullopt;
  }
  return YAML::Load(reply.to_string());
}

bool isPermissionDeniedError(const std::exception& e) {
  return std::string_view(e.what()).find("Operation not permitted") !=
         std::string_view::npos;
}

bool isSocketPathLimitError(const std::exception& e) {
  return std::string_view(e.what()).find("File name too long") !=
         std::string_view::npos;
}

template <typename Fn>
void runOrSkipIfTransportUnavailable(Fn&& fn) {
  try {
    fn();
  } catch (const std::exception& e) {
    if (isPermissionDeniedError(e) || isSocketPathLimitError(e)) {
      GTEST_SKIP() << "Socket transport unavailable in this environment: "
                   << e.what();
    }
    throw;
  }
}

}  // namespace

TEST(MachineRuntimeIntegrationTests,
     InitializeServesCommandsAndShutdownStopsServer) {
  runOrSkipIfTransportUnavailable([&] {
    const auto scoped = writeIntegrationConfig();
    utl::Config::instance().setConfigPath(scoped.path.string());

    Machine machine;
    MachineRuntime::wireMachine(machine);
    machine.initialize();
    std::this_thread::sleep_for(50ms);

    YAML::Node command;
    command["type"] = "reset";
    command["system"] = "ControlPanel";
    const auto response = sendCommand(scoped.endpoints.commandAddress, command);
    ASSERT_TRUE(response.has_value());
    EXPECT_TRUE((*response)["status"]);
    EXPECT_TRUE((*response)["message"]);

    machine.shutdown();

    const auto afterShutdown =
        sendCommand(scoped.endpoints.commandAddress, command, 300ms);
    EXPECT_FALSE(afterShutdown.has_value());

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}

TEST(MachineRuntimeIntegrationTests,
     ShutdownWhileCommandServerIsIdleCompletesPromptly) {
  runOrSkipIfTransportUnavailable([&] {
    const auto scoped = writeIntegrationConfig();
    utl::Config::instance().setConfigPath(scoped.path.string());

    Machine machine;
    MachineRuntime::wireMachine(machine);
    machine.initialize();
    std::this_thread::sleep_for(50ms);

    const auto start = std::chrono::steady_clock::now();
    machine.shutdown();
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, 1500ms);

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}

TEST(MachineRuntimeIntegrationTests,
     RepeatedInitializeShutdownCyclesRemainStable) {
  runOrSkipIfTransportUnavailable([&] {
    const auto scoped = writeIntegrationConfig();
    utl::Config::instance().setConfigPath(scoped.path.string());

    Machine machine;
    MachineRuntime::wireMachine(machine);

    for (int i = 0; i < 3; ++i) {
      machine.initialize();
      std::this_thread::sleep_for(40ms);

      YAML::Node command;
      command["type"] = "reset";
      command["system"] = "Contec";
      const auto response = sendCommand(scoped.endpoints.commandAddress, command);
      ASSERT_TRUE(response.has_value());
      EXPECT_TRUE((*response)["status"]);

      machine.shutdown();
    }

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}

TEST(MachineRuntimeIntegrationTests,
     ShutdownDuringInFlightRequestReturnsOrTimesOutWithoutHanging) {
  runOrSkipIfTransportUnavailable([&] {
    const auto scoped = writeIntegrationConfig();
    utl::Config::instance().setConfigPath(scoped.path.string());

    Machine machine;
    MachineRuntime::wireMachine(machine);
    machine.initialize();
    std::this_thread::sleep_for(50ms);

    YAML::Node command;
    command["type"] = "reset";
    command["system"] = "MotorControl";

    std::optional<YAML::Node> response;
    std::thread requester([&] {
      response = sendCommand(scoped.endpoints.commandAddress, command, 1500ms);
    });
    std::this_thread::sleep_for(5ms);
    machine.shutdown();
    requester.join();

    if (response.has_value()) {
      EXPECT_TRUE((*response)["status"]);
    }

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}
