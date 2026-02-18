#include <gtest/gtest.h>

#include <Config.hpp>
#include <MachineRuntime.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace {
std::filesystem::path writeTempConfig() {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto suffix = std::to_string(stamp);
  const auto path = std::filesystem::temp_directory_path() /
                    ("rimokun_machine_runtime_test_" + suffix + ".yaml");

  std::ofstream out(path);
  out << "classes:\n";
  out << "  RimoServer:\n";
  out << "    statusAddress: \"inproc://rimoStatus_runtime_" << suffix << "\"\n";
  out << "    commandAddress: \"inproc://rimoCommand_runtime_" << suffix << "\"\n";
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
  out << "    transport:\n";
  out << "      type: \"serialRtu\"\n";
  out << "      serial:\n";
  out << "        device: \"/dev/null\"\n";
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

  return path;
}
}  // namespace

TEST(MachineRuntimeTests, InitializeAndShutdownCompleteWithinDeadline) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  MachineRuntime runtime;
  const auto initStart = std::chrono::steady_clock::now();
  ASSERT_NO_THROW(runtime.initialize());
  const auto initElapsed = std::chrono::steady_clock::now() - initStart;
  EXPECT_LT(initElapsed, 2s);

  std::this_thread::sleep_for(50ms);

  const auto shutdownStart = std::chrono::steady_clock::now();
  EXPECT_NO_THROW(runtime.shutdown());
  const auto shutdownElapsed = std::chrono::steady_clock::now() - shutdownStart;
  EXPECT_LT(shutdownElapsed, 2s);

  std::filesystem::remove(configPath);
}

TEST(MachineRuntimeTests, ShutdownIsSafeWithoutInitializeAndWhenRepeated) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  MachineRuntime runtime;

  const auto firstShutdownStart = std::chrono::steady_clock::now();
  EXPECT_NO_THROW(runtime.shutdown());
  const auto firstShutdownElapsed =
      std::chrono::steady_clock::now() - firstShutdownStart;
  EXPECT_LT(firstShutdownElapsed, 500ms);

  ASSERT_NO_THROW(runtime.initialize());
  std::this_thread::sleep_for(20ms);
  EXPECT_NO_THROW(runtime.shutdown());

  const auto secondShutdownStart = std::chrono::steady_clock::now();
  EXPECT_NO_THROW(runtime.shutdown());
  const auto secondShutdownElapsed =
      std::chrono::steady_clock::now() - secondShutdownStart;
  EXPECT_LT(secondShutdownElapsed, 500ms);

  std::filesystem::remove(configPath);
}

TEST(MachineRuntimeTests, DoubleInitializeIsRejected) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  MachineRuntime runtime;
  ASSERT_NO_THROW(runtime.initialize());
  EXPECT_THROW((void)runtime.initialize(), std::runtime_error);
  EXPECT_NO_THROW(runtime.shutdown());

  std::filesystem::remove(configPath);
}

TEST(MachineRuntimeTests, ConcurrentShutdownCallsCompleteWithoutDeadlock) {
  const auto configPath = writeTempConfig();
  utl::Config::instance().setConfigPath(configPath.string());

  MachineRuntime runtime;
  ASSERT_NO_THROW(runtime.initialize());
  std::this_thread::sleep_for(20ms);

  const auto start = std::chrono::steady_clock::now();
  std::thread t1([&]() { EXPECT_NO_THROW(runtime.shutdown()); });
  std::thread t2([&]() { EXPECT_NO_THROW(runtime.shutdown()); });
  t1.join();
  t2.join();
  const auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_LT(elapsed, 2s);

  std::filesystem::remove(configPath);
}
