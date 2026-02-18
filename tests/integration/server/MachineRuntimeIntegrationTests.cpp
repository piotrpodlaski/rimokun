#include <gtest/gtest.h>

#include <Config.hpp>
#include <Machine.hpp>
#include <MachineRuntime.hpp>
#include <JsonExtensions.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>
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

struct IntegrationConfigOverrides {
  std::string controlPanelPort = "/dev/null";
  std::string motorDevice = "/dev/null";
};

ScopedConfig writeIntegrationConfig(
    const IntegrationConfigOverrides& overrides = {}) {
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
  out << "        port: \"" << overrides.controlPanelPort << "\"\n";
  out << "        baudRate: \"BAUD_115200\"\n";
  out << "    processing:\n";
  out << "      movingAverageDepth: 5\n";
  out << "      baselineSamples: 10\n";
  out << "      buttonDebounceSamples: 2\n";
  out << "  MotorControl:\n";
  out << "    model: \"AR-KD2\"\n";
  out << "    transport:\n";
  out << "      type: \"serialRtu\"\n";
  out << "      serial:\n";
  out << "        device: \"" << overrides.motorDevice << "\"\n";
  out << "        baud: 115200\n";
  out << "        parity: \"E\"\n";
  out << "        dataBits: 8\n";
  out << "        stopBits: 1\n";
  out << "    responseTimeoutMS: 1000\n";
  out << "    motors:\n";
  out << "      XLeft: { address: 1 }\n";
  out << "      XRight: { address: 2 }\n";
  out << "      YLeft: { address: 3 }\n";
  out << "      YRight: { address: 4 }\n";
  out << "      ZLeft: { address: 5 }\n";
  out << "      ZRight: { address: 6 }\n";
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

std::optional<nlohmann::json> sendCommand(const std::string& address,
                                      const nlohmann::json& command,
                                      const std::chrono::milliseconds timeout =
                                          1200ms) {
  zmq::context_t context{1};
  zmq::socket_t req{context, zmq::socket_type::req};
  req.set(zmq::sockopt::sndtimeo, static_cast<int>(timeout.count()));
  req.set(zmq::sockopt::rcvtimeo, static_cast<int>(timeout.count()));
  req.connect(address);
  const auto payload = nlohmann::json::to_msgpack(command);
  req.send(zmq::buffer(payload));

  zmq::message_t reply;
  if (!req.recv(reply)) {
    return std::nullopt;
  }
  const auto json = nlohmann::json::from_msgpack(
      static_cast<const std::uint8_t*>(reply.data()),
      static_cast<const std::uint8_t*>(reply.data()) + reply.size());
  return json;
}

class StatusSubscriber {
 public:
  explicit StatusSubscriber(const std::string& address)
      : _context(1), _socket(_context, zmq::socket_type::sub) {
    _socket.set(zmq::sockopt::subscribe, "");
    _socket.connect(address);
  }

  std::optional<utl::RobotStatus> receiveFor(
      const std::chrono::milliseconds timeout) {
    _socket.set(zmq::sockopt::rcvtimeo, static_cast<int>(timeout.count()));
    zmq::message_t msg;
    if (!_socket.recv(msg)) {
      return std::nullopt;
    }
    const auto json = nlohmann::json::from_msgpack(
        static_cast<const std::uint8_t*>(msg.data()),
        static_cast<const std::uint8_t*>(msg.data()) + msg.size());
    return json.get<utl::RobotStatus>();
  }

 private:
  zmq::context_t _context;
  zmq::socket_t _socket;
};

bool isPermissionDeniedError(const std::exception& e) {
  return std::string_view(e.what()).find("Operation not permitted") !=
         std::string_view::npos;
}

bool isSocketPathLimitError(const std::exception& e) {
  return std::string_view(e.what()).find("File name too long") !=
         std::string_view::npos;
}

bool isSocketTransportUnavailable(const std::exception& e) {
  const std::string_view what{e.what()};
  return what.find("Address family not supported") != std::string_view::npos ||
         what.find("Protocol not supported") != std::string_view::npos;
}

bool isContecUnavailableMessage(const std::string& message) {
  const std::string_view text{message};
  return text.find("Contec is in error state") != std::string_view::npos ||
         text.find("Failed to connect to") != std::string_view::npos ||
         text.find("Failed to create Modbus TCP client") !=
             std::string_view::npos;
}

template <typename Fn>
void runOrSkipIfTransportUnavailable(Fn&& fn) {
  try {
    fn();
  } catch (const std::exception& e) {
    if (isPermissionDeniedError(e) || isSocketPathLimitError(e) ||
        isSocketTransportUnavailable(e)) {
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

    nlohmann::json command{
        {"type", "reset"},
        {"system", "ControlPanel"},
    };
    const auto response = sendCommand(scoped.endpoints.commandAddress, command);
    ASSERT_TRUE(response.has_value());
    EXPECT_TRUE(response->contains("status"));
    EXPECT_TRUE(response->contains("message"));

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

      nlohmann::json command{
          {"type", "reset"},
          {"system", "Contec"},
      };
      const auto response = sendCommand(scoped.endpoints.commandAddress, command);
      ASSERT_TRUE(response.has_value());
      EXPECT_TRUE(response->contains("status"));

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

    nlohmann::json command{
        {"type", "reset"},
        {"system", "MotorControl"},
    };

    std::optional<nlohmann::json> response;
    std::thread requester([&] {
      response = sendCommand(scoped.endpoints.commandAddress, command, 1500ms);
    });
    std::this_thread::sleep_for(5ms);
    machine.shutdown();
    requester.join();

    if (response.has_value()) {
      EXPECT_TRUE(response->contains("status"));
    }

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}

TEST(MachineRuntimeIntegrationTests,
     PartialStartupFailuresStillServeCommandsAndPublishErrorStates) {
  runOrSkipIfTransportUnavailable([&] {
    IntegrationConfigOverrides overrides;
    overrides.controlPanelPort = "/dev/definitely_missing_control_panel";
    overrides.motorDevice = "/dev/definitely_missing_modbus";
    const auto scoped = writeIntegrationConfig(overrides);
    utl::Config::instance().setConfigPath(scoped.path.string());

    Machine machine;
    MachineRuntime::wireMachine(machine);
    StatusSubscriber subscriber(scoped.endpoints.statusAddress);
    machine.initialize();

    nlohmann::json command{
        {"type", "reset"},
        {"system", "ControlPanel"},
    };
    const auto response = sendCommand(scoped.endpoints.commandAddress, command);
    ASSERT_TRUE(response.has_value());
    EXPECT_TRUE(response->contains("status"));
    EXPECT_TRUE(response->contains("message"));

    bool sawErrorState = false;
    const auto deadline = std::chrono::steady_clock::now() + 1500ms;
    while (std::chrono::steady_clock::now() < deadline && !sawErrorState) {
      const auto status = subscriber.receiveFor(150ms);
      if (!status.has_value()) {
        continue;
      }
      const auto cpIt =
          status->robotComponents.find(utl::ERobotComponent::ControlPanel);
      const auto mcIt =
          status->robotComponents.find(utl::ERobotComponent::MotorControl);
      if (cpIt != status->robotComponents.end() &&
          mcIt != status->robotComponents.end() &&
          cpIt->second == utl::ELEDState::Error &&
          mcIt->second == utl::ELEDState::Error) {
        sawErrorState = true;
      }
    }
    EXPECT_TRUE(sawErrorState);
    machine.shutdown();

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}

TEST(MachineRuntimeIntegrationTests,
     ToolChangerCommandsPropagateToPublishedStatus) {
  runOrSkipIfTransportUnavailable([&] {
    const auto scoped = writeIntegrationConfig();
    utl::Config::instance().setConfigPath(scoped.path.string());

    Machine machine;
    MachineRuntime::wireMachine(machine);
    StatusSubscriber subscriber(scoped.endpoints.statusAddress);
    machine.initialize();

    nlohmann::json openCmd{
        {"type", "toolChanger"},
        {"position", "Left"},
        {"action", "Open"},
    };
    const auto openResponse = sendCommand(scoped.endpoints.commandAddress, openCmd);
    ASSERT_TRUE(openResponse.has_value());
    const auto openStatus = openResponse->at("status").get<std::string>();
    if (openStatus == "Error") {
      const auto openMessage =
          openResponse->value("message", std::string{});
      if (isContecUnavailableMessage(openMessage)) {
        machine.shutdown();
        std::filesystem::remove(scoped.path);
        std::filesystem::remove(scoped.statusSocketPath);
        std::filesystem::remove(scoped.commandSocketPath);
        GTEST_SKIP()
            << "Contec backend unavailable in this environment: " << openMessage;
      }
    }
    EXPECT_EQ(openStatus, "OK");

    bool sawOpenState = false;
    const auto openDeadline = std::chrono::steady_clock::now() + 1500ms;
    while (std::chrono::steady_clock::now() < openDeadline && !sawOpenState) {
      const auto status = subscriber.receiveFor(120ms);
      if (!status.has_value()) {
        continue;
      }
      const auto tcIt = status->toolChangers.find(utl::EArm::Left);
      if (tcIt == status->toolChangers.end()) {
        continue;
      }
      const auto& flags = tcIt->second.flags;
      const auto openIt = flags.find(utl::EToolChangerStatusFlags::OpenValve);
      const auto closedIt =
          flags.find(utl::EToolChangerStatusFlags::ClosedValve);
      if (openIt != flags.end() && closedIt != flags.end() &&
          openIt->second == utl::ELEDState::On &&
          closedIt->second == utl::ELEDState::Off) {
        sawOpenState = true;
      }
    }
    EXPECT_TRUE(sawOpenState);

    nlohmann::json closeCmd{
        {"type", "toolChanger"},
        {"position", "Left"},
        {"action", "Close"},
    };
    const auto closeResponse =
        sendCommand(scoped.endpoints.commandAddress, closeCmd);
    ASSERT_TRUE(closeResponse.has_value());
    EXPECT_EQ(closeResponse->at("status").get<std::string>(), "OK");

    bool sawCloseState = false;
    const auto closeDeadline = std::chrono::steady_clock::now() + 1500ms;
    while (std::chrono::steady_clock::now() < closeDeadline && !sawCloseState) {
      const auto status = subscriber.receiveFor(120ms);
      if (!status.has_value()) {
        continue;
      }
      const auto tcIt = status->toolChangers.find(utl::EArm::Left);
      if (tcIt == status->toolChangers.end()) {
        continue;
      }
      const auto& flags = tcIt->second.flags;
      const auto openIt = flags.find(utl::EToolChangerStatusFlags::OpenValve);
      const auto closedIt =
          flags.find(utl::EToolChangerStatusFlags::ClosedValve);
      if (openIt != flags.end() && closedIt != flags.end() &&
          openIt->second == utl::ELEDState::Off &&
          closedIt->second == utl::ELEDState::On) {
        sawCloseState = true;
      }
    }
    EXPECT_TRUE(sawCloseState);

    machine.shutdown();

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}

TEST(MachineRuntimeIntegrationTests,
     ShutdownDuringParallelRequestBurstDoesNotDeadlock) {
  runOrSkipIfTransportUnavailable([&] {
    const auto scoped = writeIntegrationConfig();
    utl::Config::instance().setConfigPath(scoped.path.string());

    Machine machine;
    MachineRuntime::wireMachine(machine);
    machine.initialize();
    std::this_thread::sleep_for(40ms);

    std::atomic<bool> keepSending{true};
    std::atomic<int> completedRequests{0};
    std::vector<std::thread> requesters;
    requesters.reserve(4);

    for (int i = 0; i < 4; ++i) {
      requesters.emplace_back([&] {
        while (keepSending.load(std::memory_order_acquire)) {
          nlohmann::json command{
              {"type", "reset"},
              {"system", "Contec"},
          };
          auto response =
              sendCommand(scoped.endpoints.commandAddress, command, 250ms);
          if (response.has_value()) {
            ++completedRequests;
          }
        }
      });
    }

    std::this_thread::sleep_for(80ms);
    const auto start = std::chrono::steady_clock::now();
    machine.shutdown();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    keepSending.store(false, std::memory_order_release);

    for (auto& t : requesters) {
      t.join();
    }

    EXPECT_LT(elapsed, 2s);
    EXPECT_GE(completedRequests.load(), 0);

    std::filesystem::remove(scoped.path);
    std::filesystem::remove(scoped.statusSocketPath);
    std::filesystem::remove(scoped.commandSocketPath);
  });
}
