#include <gtest/gtest.h>

#include <JsonExtensions.hpp>

TEST(JsonExtensionsTests, RobotStatusRoundTripPreservesSafetyOn) {
  utl::RobotStatus status;
  status.safetyOn = true;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::On;

  const auto json = nlohmann::json(status);
  ASSERT_TRUE(json.contains("safetyOn"));
  EXPECT_TRUE(json.at("safetyOn").get<bool>());

  const auto roundTrip = json.get<utl::RobotStatus>();
  ASSERT_TRUE(roundTrip.safetyOn.has_value());
  EXPECT_TRUE(*roundTrip.safetyOn);
}
