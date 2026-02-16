#include <gtest/gtest.h>

#include <ControlPanelCommFactory.hpp>

#include <yaml-cpp/yaml.h>

TEST(ControlPanelCommFactoryTests, SerialTransportCreatesSerialComm) {
  const auto cfg = YAML::Load(R"yaml(
type: serial
serial:
  port: /dev/ttyS0
  baudRate: BAUD_115200
)yaml");

  auto comm = makeControlPanelComm(cfg);
  ASSERT_TRUE(comm);
  EXPECT_EQ(comm->describe(), "serial(/dev/ttyS0)");
}

TEST(ControlPanelCommFactoryTests, TransportTypeIsCaseInsensitive) {
  const auto cfg = YAML::Load(R"yaml(
type: SeRiAl
serial:
  port: /dev/ttyS1
  baudRate: BAUD_115200
)yaml");

  auto comm = makeControlPanelComm(cfg);
  ASSERT_TRUE(comm);
  EXPECT_EQ(comm->describe(), "serial(/dev/ttyS1)");
}

TEST(ControlPanelCommFactoryTests, MissingTypeThrows) {
  const auto cfg = YAML::Load(R"yaml(
serial:
  port: /dev/ttyS0
  baudRate: BAUD_115200
)yaml");

  EXPECT_THROW((void)makeControlPanelComm(cfg), std::runtime_error);
}

TEST(ControlPanelCommFactoryTests, NonScalarTypeThrows) {
  const auto cfg = YAML::Load(R"yaml(
type:
  nested: invalid
serial:
  port: /dev/ttyS0
  baudRate: BAUD_115200
)yaml");

  EXPECT_THROW((void)makeControlPanelComm(cfg), std::runtime_error);
}

TEST(ControlPanelCommFactoryTests, SerialWithoutCommSerialMapThrows) {
  const auto cfg = YAML::Load(R"yaml(
type: serial
)yaml");

  EXPECT_THROW((void)makeControlPanelComm(cfg), std::runtime_error);
}

TEST(ControlPanelCommFactoryTests, TcpTransportIsRecognizedButNotImplemented) {
  const auto cfg = YAML::Load(R"yaml(
type: tcp
)yaml");

  EXPECT_THROW((void)makeControlPanelComm(cfg), std::runtime_error);
}

TEST(ControlPanelCommFactoryTests, UnsupportedTransportThrows) {
  const auto cfg = YAML::Load(R"yaml(
type: udp
)yaml");

  EXPECT_THROW((void)makeControlPanelComm(cfg), std::runtime_error);
}

