#include <gtest/gtest.h>

#include <Config.hpp>
#include <Machine.hpp>
#include <MachineRuntime.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>

#include "server/fakes/FakeClock.hpp"

using namespace std::chrono_literals;

namespace {
std::filesystem::path writeTempConfig() {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto suffix = std::to_string(stamp);
  const auto path = std::filesystem::temp_directory_path() /
                    ("rimokun_machine_cmd_test_" + suffix + ".yaml");

  std::ofstream out(path);
  out << "classes:\n";
  out << "  RimoServer:\n";
  out << "    statusAddress: \"inproc://rimoStatus_cmd_" << suffix << "\"\n";
  out << "    commandAddress: \"inproc://rimoCommand_cmd_" << suffix << "\"\n";
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
  return path;
}

class CommandTestMachine final : public Machine {
 public:
  explicit CommandTestMachine(const std::shared_ptr<IClock>& clock)
      : Machine(clock) {}

  int reconnectCalls{0};

 protected:
  void controlLoopTasks() override {}
  void updateStatus() override {}
  void handleReconnectCommand(const cmd::ReconnectCommand&) override {
    ++reconnectCalls;
  }
  void handleToolChangerCommand(const cmd::ToolChangerCommand&) override {}
};

class FlakyCommandTestMachine final : public Machine {
 public:
  explicit FlakyCommandTestMachine(const std::shared_ptr<IClock>& clock)
      : Machine(clock) {}

  int reconnectCalls{0};
  bool failNextReconnect{true};

 protected:
  void controlLoopTasks() override {}
  void updateStatus() override {}
  void handleReconnectCommand(const cmd::ReconnectCommand&) override {
    if (failNextReconnect) {
      failNextReconnect = false;
      throw std::runtime_error("forced reconnect failure");
    }
    ++reconnectCalls;
  }
  void handleToolChangerCommand(const cmd::ToolChangerCommand&) override {}
};
}  // namespace

TEST(MachineCommandTests, ValidCommandReturnsOkResponse) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  CommandTestMachine machine(fakeClock);
  MachineRuntime::wireMachine(machine);
  auto state = machine.makeInitialLoopState();

  cmd::Command command;
  command.payload = cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};
  auto resultFuture = command.reply.get_future();
  ASSERT_TRUE(machine.submitCommand(std::move(command)));

  machine.runOneCycle(state);
  ASSERT_EQ(resultFuture.wait_for(50ms), std::future_status::ready);
  EXPECT_EQ(resultFuture.get(), "");
  EXPECT_EQ(machine.reconnectCalls, 1);

  std::filesystem::remove(configPath);
}

TEST(MachineCommandTests, CommandTimeoutReturnsError) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  CommandTestMachine machine(fakeClock);

  cmd::Command command;
  command.payload = cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};

  const auto reply = machine.dispatchCommandAndWait(std::move(command), 5ms);
  EXPECT_EQ(reply, "Command processing timed out");
  EXPECT_EQ(machine.reconnectCalls, 0);

  std::filesystem::remove(configPath);
}

TEST(MachineCommandTests, CommandFailureIsReturnedAndSubsequentCommandSucceeds) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  FlakyCommandTestMachine machine(fakeClock);
  MachineRuntime::wireMachine(machine);
  auto state = machine.makeInitialLoopState();

  cmd::Command failingCommand;
  failingCommand.payload =
      cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};
  auto failingFuture = failingCommand.reply.get_future();
  ASSERT_TRUE(machine.submitCommand(std::move(failingCommand)));
  machine.runOneCycle(state);
  ASSERT_EQ(failingFuture.wait_for(50ms), std::future_status::ready);
  EXPECT_EQ(failingFuture.get(), "forced reconnect failure");

  cmd::Command succeedingCommand;
  succeedingCommand.payload =
      cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};
  auto succeedingFuture = succeedingCommand.reply.get_future();
  ASSERT_TRUE(machine.submitCommand(std::move(succeedingCommand)));
  machine.runOneCycle(state);
  ASSERT_EQ(succeedingFuture.wait_for(50ms), std::future_status::ready);
  EXPECT_EQ(succeedingFuture.get(), "");
  EXPECT_EQ(machine.reconnectCalls, 1);

  std::filesystem::remove(configPath);
}

TEST(MachineCommandTests, DispatchAfterShutdownReturnsMachineShuttingDown) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  CommandTestMachine machine(fakeClock);
  machine.shutdown();

  cmd::Command command;
  command.payload = cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};

  const auto reply = machine.dispatchCommandAndWait(std::move(command), 5ms);
  EXPECT_EQ(reply, "Machine is shutting down");

  std::filesystem::remove(configPath);
}

TEST(MachineCommandTests, PendingCommandGetsShutdownReplyWhenMachineStops) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  CommandTestMachine machine(fakeClock);

  cmd::Command command;
  command.payload = cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};
  auto future = command.reply.get_future();
  ASSERT_TRUE(machine.submitCommand(std::move(command)));

  machine.shutdown();

  ASSERT_EQ(future.wait_for(50ms), std::future_status::ready);
  EXPECT_EQ(future.get(), "Machine is shutting down");

  std::filesystem::remove(configPath);
}

TEST(MachineCommandTests, SubmitCommandAfterShutdownIsRejected) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  CommandTestMachine machine(fakeClock);
  machine.shutdown();

  cmd::Command command;
  command.payload = cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};
  EXPECT_FALSE(machine.submitCommand(std::move(command)));

  std::filesystem::remove(configPath);
}
