#include <gtest/gtest.h>

#include <ControlPanelCommFactory.hpp>
#include <IControlPanelComm.hpp>
#include <SerialControlPanelComm.hpp>

#include <memory>

#include <yaml-cpp/yaml.h>

namespace {

void runCommonContract(const std::function<std::unique_ptr<IControlPanelComm>()>& makeComm) {
  auto comm = makeComm();
  ASSERT_TRUE(comm);
  EXPECT_FALSE(comm->describe().empty());
  comm->closeNoThrow();
  comm->closeNoThrow();
}

YAML::Node serialConfig(const std::string& port) {
  YAML::Node cfg;
  cfg["type"] = "serial";
  cfg["serial"]["port"] = port;
  cfg["serial"]["baudRate"] = "BAUD_115200";
  return cfg;
}

}  // namespace

TEST(ControlPanelCommContractTests, SerialImplementationSatisfiesCommonContract) {
  runCommonContract([] { return std::make_unique<SerialControlPanelComm>(serialConfig("/dev/null")); });
}

TEST(ControlPanelCommContractTests, FactoryCreatedSerialSatisfiesCommonContract) {
  runCommonContract([] { return makeControlPanelComm(serialConfig("/dev/null")); });
}

TEST(ControlPanelCommContractTests, SerialOpenFailureIsReportedAsException) {
  auto comm = std::make_unique<SerialControlPanelComm>(
      serialConfig("/dev/definitely_missing_serial_device"));
  EXPECT_THROW(comm->open(), std::exception);
  comm->closeNoThrow();
}

