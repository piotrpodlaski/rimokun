#include <gtest/gtest.h>

#include <Config.hpp>
#include <Machine.hpp>
#include <MachineRuntime.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "server/fakes/FakeClock.hpp"

namespace {
std::filesystem::path writeTempConfig() {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto suffix = std::to_string(stamp);
  const auto path = std::filesystem::temp_directory_path() /
                    ("rimokun_machine_loop_test_" + suffix + ".yaml");

  std::ofstream out(path);
  out << "classes:\n";
  out << "  RimoServer:\n";
  out << "    statusAddress: \"inproc://rimoStatus_test_" << suffix << "\"\n";
  out << "    commandAddress: \"inproc://rimoCommand_test_" << suffix << "\"\n";
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

class TestMachine final : public Machine {
 public:
  explicit TestMachine(const std::shared_ptr<IClock>& clock) : Machine(clock) {}

  int controlCycles{0};
  int publishedUpdates{0};

 protected:
  void controlLoopTasks() override { ++controlCycles; }
  void updateStatus() override { ++publishedUpdates; }
};

class SlowTestMachine final : public Machine {
 public:
  SlowTestMachine(const std::shared_ptr<FakeClock>& clock,
                  const std::chrono::milliseconds simulatedWork)
      : Machine(clock), _clock(clock), _simulatedWork(simulatedWork) {}

 protected:
  void controlLoopTasks() override { _clock->advanceBy(_simulatedWork); }
  void updateStatus() override {}

 private:
  std::shared_ptr<FakeClock> _clock;
  std::chrono::milliseconds _simulatedWork;
};
}  // namespace

TEST(MachineLoopTests, RunOneCycleAtConfiguredCadence) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  TestMachine machine(fakeClock);
  MachineRuntime::wireMachine(machine);
  auto state = machine.makeInitialLoopState();

  for (int i = 0; i < 12; ++i) {
    machine.runOneCycle(state);
  }

  EXPECT_EQ(machine.controlCycles, 12);
  // With 10ms loop and 50ms update interval, publishing happens at t=0,50,100.
  EXPECT_EQ(machine.publishedUpdates, 3);
  EXPECT_EQ(fakeClock->now(), IClock::time_point{std::chrono::milliseconds{110}});

  std::filesystem::remove(configPath);
}

TEST(MachineLoopTests, OverrunResynchronizesToFutureTick) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  SlowTestMachine machine(fakeClock, std::chrono::milliseconds{25});
  MachineRuntime::wireMachine(machine);
  auto state = machine.makeInitialLoopState();

  machine.runOneCycle(state);
  EXPECT_EQ(state.nextLoopAt, IClock::time_point{std::chrono::milliseconds{30}});

  machine.runOneCycle(state);
  EXPECT_EQ(state.nextLoopAt, IClock::time_point{std::chrono::milliseconds{60}});

  std::filesystem::remove(configPath);
}
