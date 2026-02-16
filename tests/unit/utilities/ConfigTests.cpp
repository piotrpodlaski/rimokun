#include <gtest/gtest.h>

#include <Config.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
std::filesystem::path writeTempConfig(const std::string& content) {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto path = std::filesystem::temp_directory_path() /
                    ("rimokun_config_test_" + std::to_string(stamp) + ".yaml");
  std::ofstream out(path);
  out << content;
  out.close();
  return path;
}
}  // namespace

TEST(ConfigTests, GetRequiredReturnsConfiguredValue) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  Demo:
    value: 42
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  EXPECT_EQ(utl::Config::instance().getRequired<int>("Demo", "value"), 42);

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, GetRequiredThrowsOnMissingKey) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  Demo:
    value: 42
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  EXPECT_THROW((void)utl::Config::instance().getRequired<int>("Demo", "missing"),
               std::runtime_error);

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, GetOptionalReturnsDefaultWhenMissing) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  Demo:
    value: 42
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  EXPECT_EQ(
      utl::Config::instance().getOptional<int>("Demo", "missing_optional", 99),
      99);

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, GetClassConfigThrowsWhenClassesSectionMissing) {
  const auto configPath = writeTempConfig(R"yaml(
foo: bar
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  EXPECT_THROW((void)utl::Config::instance().getClassConfig("Demo"),
               std::runtime_error);

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, GetOptionalThrowsOnTypeMismatch) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  Demo:
    value: abc
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  EXPECT_THROW((void)utl::Config::instance().getOptional<int>("Demo", "value", 1),
               std::runtime_error);

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, MissingMachineSectionThrowsWhenRequested) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  Demo:
    value: 1
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  EXPECT_THROW((void)utl::Config::instance().getClassConfig("Machine"),
               std::runtime_error);

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, MachineIntervalTypeMismatchThrows) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  Machine:
    loopIntervalMS: abc
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  EXPECT_THROW(
      (void)utl::Config::instance().getOptional<int>("Machine", "loopIntervalMS", 10),
      std::runtime_error);

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, ControlPanelTransportConfigCanBeRead) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  ControlPanel:
    comm:
      type: serial
      serial:
        port: /dev/ttyS0
        baudRate: BAUD_115200
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  const auto controlPanel = utl::Config::instance().getClassConfig("ControlPanel");
  ASSERT_TRUE(controlPanel["comm"]);
  EXPECT_EQ(controlPanel["comm"]["type"].as<std::string>(), "serial");
  EXPECT_EQ(controlPanel["comm"]["serial"]["port"].as<std::string>(), "/dev/ttyS0");

  std::filesystem::remove(configPath);
}

TEST(ConfigTests, MissingControlPanelTransportTypeThrows) {
  const auto configPath = writeTempConfig(R"yaml(
classes:
  ControlPanel:
    comm:
      serial:
        port: /dev/ttyS0
)yaml");

  utl::Config::instance().setConfigPath(configPath.string());
  const auto controlPanel = utl::Config::instance().getClassConfig("ControlPanel");
  EXPECT_THROW((void)controlPanel["comm"]["type"].as<std::string>(),
               YAML::Exception);

  std::filesystem::remove(configPath);
}
