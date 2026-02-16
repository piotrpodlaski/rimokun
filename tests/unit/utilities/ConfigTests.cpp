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
