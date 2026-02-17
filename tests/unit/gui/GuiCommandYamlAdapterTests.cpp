#include <gtest/gtest.h>

#include <GuiCommandYamlAdapter.hpp>
#include <GuiYamlExtensions.hpp>

TEST(GuiCommandYamlAdapterTests, ToolChangerCommandRoundTrip) {
  GuiCommand command;
  command.payload =
      GuiToolChangerCommand{.arm = utl::EArm::Left,
                            .action = utl::EToolChangerAction::Open};

  const auto yaml = GuiCommandYamlAdapter::toYaml(command);
  ASSERT_TRUE(yaml["type"]);
  EXPECT_EQ(yaml["type"].as<std::string>(), "toolChanger");
  EXPECT_EQ(yaml["position"].as<utl::EArm>(), utl::EArm::Left);
  EXPECT_EQ(yaml["action"].as<utl::EToolChangerAction>(),
            utl::EToolChangerAction::Open);

  const auto decoded = yaml.as<GuiCommand>();
  ASSERT_TRUE(std::holds_alternative<GuiToolChangerCommand>(decoded.payload));
  const auto toolChanger = std::get<GuiToolChangerCommand>(decoded.payload);
  EXPECT_EQ(toolChanger.arm, utl::EArm::Left);
  EXPECT_EQ(toolChanger.action, utl::EToolChangerAction::Open);
}

TEST(GuiCommandYamlAdapterTests, ResponseParsingKeepsMessageAndPayload) {
  YAML::Node response;
  response["status"] = "Error";
  response["message"] = "bad command";
  response["response"]["debug"] = 42;

  const auto decoded = GuiCommandYamlAdapter::fromYaml(response);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_FALSE(decoded->ok);
  EXPECT_EQ(decoded->message, "bad command");
  ASSERT_TRUE(decoded->payload.has_value());
  EXPECT_EQ((*decoded->payload)["debug"].as<int>(), 42);
}
