#include <gtest/gtest.h>

#include <CommonDefinitions.hpp>
#include <yaml-cpp/yaml.h>

TEST(YamlExtensionsTests, EnumRoundTripWorks) {
  const auto node = YAML::convert<utl::EMotor>::encode(utl::EMotor::XLeft);
  ASSERT_TRUE(node.IsScalar());
  EXPECT_EQ(node.as<std::string>(), "XLeft");
  EXPECT_EQ(node.as<utl::EMotor>(), utl::EMotor::XLeft);
}

TEST(YamlExtensionsTests, RobotStatusRoundTripWorks) {
  utl::RobotStatus input;
  input.motors[utl::EMotor::XLeft] = {
      .currentPosition = 10.5,
      .targetPosition = 20.5,
      .speed = 5.0,
      .torque = 9,
      .flags = {{utl::EMotorStatusFlags::BrakeApplied, utl::ELEDState::On},
                {utl::EMotorStatusFlags::Enabled, utl::ELEDState::Error}}};
  input.toolChangers[utl::EArm::Left] = {
      .flags = {{utl::EToolChangerStatusFlags::ProxSen, utl::ELEDState::On}}};
  input.robotComponents[utl::ERobotComponent::ControlPanel] = utl::ELEDState::Off;
  input.joystics[utl::EArm::Gantry] = {.x = 0.25, .y = -0.75, .btn = true};

  const auto encoded = YAML::convert<utl::RobotStatus>::encode(input);
  const auto dumped = YAML::Dump(encoded);
  auto decoded = YAML::Load(dumped).as<utl::RobotStatus>();

  ASSERT_TRUE(decoded.motors.contains(utl::EMotor::XLeft));
  EXPECT_DOUBLE_EQ(decoded.motors.at(utl::EMotor::XLeft).currentPosition, 10.5);
  EXPECT_EQ(decoded.motors.at(utl::EMotor::XLeft).torque, 9);
  EXPECT_EQ(decoded.robotComponents.at(utl::ERobotComponent::ControlPanel),
            utl::ELEDState::Off);
  ASSERT_TRUE(decoded.joystics.contains(utl::EArm::Gantry));
  EXPECT_TRUE(decoded.joystics.at(utl::EArm::Gantry).btn);
}

TEST(YamlExtensionsTests, RobotStatusDecodeWithoutJoysticsIsSupported) {
  auto node = YAML::Load(R"yaml(
motors: {}
toolChangers: {}
robotComponents: {}
)yaml");

  auto decoded = node.as<utl::RobotStatus>();
  EXPECT_TRUE(decoded.joystics.empty());
}
