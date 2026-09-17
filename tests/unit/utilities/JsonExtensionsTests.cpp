#include <gtest/gtest.h>

#include <JsonExtensions.hpp>

TEST(JsonExtensionsTests, RobotStatusRoundTripPreservesSafetyOn) {
  utl::RobotStatus status;
  status.safetyOn = true;
  status.cpuOverride = true;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::On;

  const auto json = nlohmann::json(status);
  ASSERT_TRUE(json.contains("safetyOn"));
  EXPECT_TRUE(json.at("safetyOn").get<bool>());
  ASSERT_TRUE(json.contains("cpuOverride"));
  EXPECT_TRUE(json.at("cpuOverride").get<bool>());

  const auto roundTrip = json.get<utl::RobotStatus>();
  ASSERT_TRUE(roundTrip.safetyOn.has_value());
  EXPECT_TRUE(*roundTrip.safetyOn);
  ASSERT_TRUE(roundTrip.cpuOverride.has_value());
  EXPECT_TRUE(*roundTrip.cpuOverride);
}

TEST(JsonExtensionsTests, RobotStatusRoundTripPreservesHomingSession) {
  utl::RobotStatus status;
  status.homing.state = utl::EHomingState::Running;
  status.homing.message = "Homing XLeft...";
  status.homing.axes = {
      {.axis = utl::EHomingAxis::XLeft,
       .state = utl::EHomingState::Running,
       .message = "Homing in progress"},
      {.axis = utl::EHomingAxis::YLeft,
       .state = utl::EHomingState::Pending,
       .message = ""},
  };

  const auto json = nlohmann::json(status);
  ASSERT_TRUE(json.contains("homing"));
  ASSERT_TRUE(json.at("homing").contains("axes"));

  const auto roundTrip = json.get<utl::RobotStatus>();
  EXPECT_EQ(roundTrip.homing.state, utl::EHomingState::Running);
  EXPECT_EQ(roundTrip.homing.message, "Homing XLeft...");
  ASSERT_EQ(roundTrip.homing.axes.size(), 2u);
  EXPECT_EQ(roundTrip.homing.axes[0].axis, utl::EHomingAxis::XLeft);
  EXPECT_EQ(roundTrip.homing.axes[0].state, utl::EHomingState::Running);
  EXPECT_EQ(roundTrip.homing.axes[1].axis, utl::EHomingAxis::YLeft);
  EXPECT_EQ(roundTrip.homing.axes[1].state, utl::EHomingState::Pending);
}
