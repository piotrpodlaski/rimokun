#include <gtest/gtest.h>

#include <Config.hpp>
#include <Machine.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

#include "server/fakes/FakeClock.hpp"

namespace {
std::filesystem::path writeTempConfigMissingLight2() {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto suffix = std::to_string(stamp);
  const auto path = std::filesystem::temp_directory_path() /
                    ("rimokun_machine_mapping_test_" + suffix + ".yaml");

  std::ofstream out(path);
  out << "classes:\n";
  out << "  RimoServer:\n";
  out << "    statusAddress: \"inproc://rimoStatus_mapping_" << suffix << "\"\n";
  out << "    commandAddress: \"inproc://rimoCommand_mapping_" << suffix << "\"\n";
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
  out.close();
  return path;
}
}  // namespace

TEST(MachineMappingTests, MissingRequiredMappingsFailFast) {
  const auto configPath = writeTempConfigMissingLight2();
  utl::Config::instance().setConfigPath(configPath.string());

  auto fakeClock = std::make_shared<FakeClock>();
  try {
    (void)Machine(fakeClock);
    FAIL() << "Expected constructor to throw on missing required mapping.";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("light2"), std::string::npos);
  }

  std::filesystem::remove(configPath);
}

TEST(MachineMappingTests, OutOfRangeMappingIsRejected) {
  EXPECT_THROW((void)Machine::validateMappedIndex("toolChangerLeft", 8, 8, "output"),
               std::runtime_error);
  EXPECT_NO_THROW((void)Machine::validateMappedIndex("toolChangerLeft", 7, 8, "output"));
}
