#include <gtest/gtest.h>

#include <SerialControlPanelComm.hpp>

#include <yaml-cpp/yaml.h>

TEST(SerialControlPanelCommTests, MissingSerialMapThrows) {
  const auto cfg = YAML::Load(R"yaml(
type: serial
)yaml");

  EXPECT_THROW((void)SerialControlPanelComm(cfg), std::runtime_error);
}

TEST(SerialControlPanelCommTests, MissingPortThrows) {
  const auto cfg = YAML::Load(R"yaml(
type: serial
serial:
  baudRate: BAUD_115200
)yaml");

  EXPECT_THROW((void)SerialControlPanelComm(cfg), std::runtime_error);
}

TEST(SerialControlPanelCommTests, MissingBaudRateThrows) {
  const auto cfg = YAML::Load(R"yaml(
type: serial
serial:
  port: /dev/ttyS0
)yaml");

  EXPECT_THROW((void)SerialControlPanelComm(cfg), std::runtime_error);
}

TEST(SerialControlPanelCommTests, DescribeIncludesPort) {
  const auto cfg = YAML::Load(R"yaml(
type: serial
serial:
  port: /dev/ttyS7
  baudRate: BAUD_115200
)yaml");

  SerialControlPanelComm comm(cfg);
  EXPECT_EQ(comm.describe(), "serial(/dev/ttyS7)");
}

TEST(SerialControlPanelCommTests, OptionalSerialFieldsAreAccepted) {
  const auto cfg = YAML::Load(R"yaml(
type: serial
serial:
  port: /dev/ttyS8
  baudRate: BAUD_115200
  characterSize: CHAR_SIZE_8
  flowControl: FLOW_CONTROL_NONE
  parity: PARITY_NONE
  stopBits: STOP_BITS_1
  readTimeoutMS: 15
  lineTerminator: \r
)yaml");

  SerialControlPanelComm comm(cfg);
  EXPECT_EQ(comm.describe(), "serial(/dev/ttyS8)");
}

