#include <gtest/gtest.h>

#include <GuiCommandJsonAdapter.hpp>

TEST(GuiCommandJsonAdapterTests, ToolChangerCommandRoundTrip) {
  GuiCommand command;
  command.payload =
      GuiToolChangerCommand{.arm = utl::EArm::Left,
                            .action = utl::EToolChangerAction::Open};

  const auto json = GuiCommandJsonAdapter::toJson(command);
  ASSERT_TRUE(json.contains("type"));
  EXPECT_EQ(json["type"].get<std::string>(), "toolChanger");
  EXPECT_EQ(json["position"].get<std::string>(), "Left");
  EXPECT_EQ(json["action"].get<std::string>(), "Open");
}

TEST(GuiCommandJsonAdapterTests, ResponseParsingKeepsMessageAndPayload) {
  nlohmann::json response{
      {"status", "Error"},
      {"message", "bad command"},
      {"response", {{"debug", 42}}},
  };

  const auto decoded = GuiCommandJsonAdapter::fromJson(response);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_FALSE(decoded->ok);
  EXPECT_EQ(decoded->message, "bad command");
  ASSERT_TRUE(decoded->payload.has_value());
  EXPECT_EQ(decoded->payload->at("debug").get<int>(), 42);
}
