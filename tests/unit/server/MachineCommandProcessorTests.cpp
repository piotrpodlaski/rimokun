#include <gtest/gtest.h>

#include <MachineCommandProcessor.hpp>

#include <chrono>
#include <string>

using namespace std::chrono_literals;

TEST(MachineCommandProcessorTests, RejectsNonMapCommand) {
  MachineCommandProcessor processor;
  const auto response = processor.processCommand(
      YAML::Node("not-a-map"), [](cmd::Command, std::chrono::milliseconds) {
        return std::string{};
      });

  EXPECT_EQ(response["status"].as<std::string>(), "Error");
}

TEST(MachineCommandProcessorTests, UnknownTypeReturnsError) {
  MachineCommandProcessor processor;
  YAML::Node command;
  command["type"] = "unknown";

  const auto response = processor.processCommand(
      command, [](cmd::Command, std::chrono::milliseconds) {
        return std::string{};
      });

  EXPECT_EQ(response["status"].as<std::string>(), "Error");
  EXPECT_TRUE(response["message"].as<std::string>().find("Unknown command type") !=
              std::string::npos);
}

TEST(MachineCommandProcessorTests, ToolChangerDispatchesWithTwoSecondTimeout) {
  MachineCommandProcessor processor;
  YAML::Node command;
  command["type"] = "toolChanger";
  command["position"] = "Left";
  command["action"] = "Open";

  bool dispatched = false;
  std::chrono::milliseconds seenTimeout{0};

  const auto response = processor.processCommand(
      command, [&](cmd::Command c, const std::chrono::milliseconds timeout) {
        dispatched = true;
        seenTimeout = timeout;
        EXPECT_TRUE(
            std::holds_alternative<cmd::ToolChangerCommand>(c.payload));
        return std::string{};
      });

  EXPECT_TRUE(dispatched);
  EXPECT_EQ(seenTimeout, 2s);
  EXPECT_EQ(response["status"].as<std::string>(), "OK");
}

TEST(MachineCommandProcessorTests, ResetDispatchErrorPropagates) {
  MachineCommandProcessor processor;
  YAML::Node command;
  command["type"] = "reset";
  command["system"] = "ControlPanel";

  const auto response = processor.processCommand(
      command, [](cmd::Command c, std::chrono::milliseconds) {
        EXPECT_TRUE(std::holds_alternative<cmd::ReconnectCommand>(c.payload));
        return std::string{"boom"};
      });

  EXPECT_EQ(response["status"].as<std::string>(), "Error");
  EXPECT_EQ(response["message"].as<std::string>(), "boom");
}

TEST(MachineCommandProcessorTests, ToolChangerMissingFieldsReturnsErrorWithoutDispatch) {
  MachineCommandProcessor processor;
  YAML::Node command;
  command["type"] = "toolChanger";
  command["position"] = "Left";

  bool dispatched = false;
  const auto response = processor.processCommand(
      command, [&](cmd::Command, std::chrono::milliseconds) {
        dispatched = true;
        return std::string{};
      });

  EXPECT_FALSE(dispatched);
  EXPECT_EQ(response["status"].as<std::string>(), "Error");
  EXPECT_TRUE(
      response["message"].as<std::string>().find("requires both 'position' and 'action'") !=
      std::string::npos);
}

TEST(MachineCommandProcessorTests, ResetMissingSystemReturnsErrorWithoutDispatch) {
  MachineCommandProcessor processor;
  YAML::Node command;
  command["type"] = "reset";

  bool dispatched = false;
  const auto response = processor.processCommand(
      command, [&](cmd::Command, std::chrono::milliseconds) {
        dispatched = true;
        return std::string{};
      });

  EXPECT_FALSE(dispatched);
  EXPECT_EQ(response["status"].as<std::string>(), "Error");
  EXPECT_TRUE(response["message"].as<std::string>().find("requires a 'system' field") !=
              std::string::npos);
}

TEST(MachineCommandProcessorTests, ToolChangerInvalidEnumReturnsError) {
  MachineCommandProcessor processor;
  YAML::Node command;
  command["type"] = "toolChanger";
  command["position"] = "invalid-arm";
  command["action"] = "Open";

  bool dispatched = false;
  const auto response = processor.processCommand(
      command, [&](cmd::Command, std::chrono::milliseconds) {
        dispatched = true;
        return std::string{};
      });

  EXPECT_FALSE(dispatched);
  EXPECT_EQ(response["status"].as<std::string>(), "Error");
  EXPECT_TRUE(
      response["message"].as<std::string>().find("Invalid toolChanger command") !=
      std::string::npos);
}

TEST(MachineCommandProcessorTests, ResetInvalidEnumReturnsError) {
  MachineCommandProcessor processor;
  YAML::Node command;
  command["type"] = "reset";
  command["system"] = "not-a-component";

  bool dispatched = false;
  const auto response = processor.processCommand(
      command, [&](cmd::Command, std::chrono::milliseconds) {
        dispatched = true;
        return std::string{};
      });

  EXPECT_FALSE(dispatched);
  EXPECT_EQ(response["status"].as<std::string>(), "Error");
  EXPECT_TRUE(response["message"].as<std::string>().find("Invalid reset command") !=
              std::string::npos);
}
