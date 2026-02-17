#include <gtest/gtest.h>

#include <RobotControlPolicy.hpp>

#include <stdexcept>

TEST(RobotControlPolicyTests, ErrorStateRequestsToolChangerErrorBlinking) {
  DefaultRobotControlPolicy policy;
  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt,
      MachineComponent::State::Error, utl::RobotStatus{});

  EXPECT_TRUE(decision.setToolChangerErrorBlinking);
  EXPECT_FALSE(decision.outputs.has_value());
}

TEST(RobotControlPolicyTests, MissingInputsRequestToolChangerErrorBlinking) {
  DefaultRobotControlPolicy policy;
  const auto decision = policy.decide(std::nullopt, std::nullopt,
                                      MachineComponent::State::Normal,
                                      utl::RobotStatus{});

  EXPECT_TRUE(decision.setToolChangerErrorBlinking);
  EXPECT_FALSE(decision.outputs.has_value());
}

TEST(RobotControlPolicyTests, NormalInputsMapButtonsToLights) {
  DefaultRobotControlPolicy policy;
  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt,
      MachineComponent::State::Normal, utl::RobotStatus{});

  ASSERT_FALSE(decision.setToolChangerErrorBlinking);
  ASSERT_TRUE(decision.outputs.has_value());
  EXPECT_TRUE(decision.outputs->at("light1"));
  EXPECT_FALSE(decision.outputs->at("light2"));
}

TEST(RobotControlPolicyTests, MissingButtonSignalsThrow) {
  DefaultRobotControlPolicy policy;
  EXPECT_THROW(
      (void)policy.decide(IRobotControlPolicy::SignalMap{{"button1", true}},
                          std::nullopt,
                          MachineComponent::State::Normal, utl::RobotStatus{}),
      std::runtime_error);
}

TEST(RobotControlPolicyTests, RimoKunPolicyGeneratesMotorIntentsFromJoystick) {
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  status.joystics[utl::EArm::Left] = {.x = 0.0, .y = 0.7, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.0, .y = -0.5, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.0, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  EXPECT_FALSE(decision.setToolChangerErrorBlinking);
  ASSERT_TRUE(decision.outputs.has_value());
  EXPECT_TRUE(decision.outputs->at("light1"));
  EXPECT_FALSE(decision.outputs->at("light2"));
  EXPECT_EQ(decision.motorIntents.size(), 6u);
}

TEST(RobotControlPolicyTests, RimoKunPolicyUsesPositionModeWhenJoystickButtonPressed) {
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  status.joystics[utl::EArm::Left] = {.x = 0.8, .y = 0.1, .btn = true};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", true}},
      std::nullopt, MachineComponent::State::Normal, status);

  ASSERT_FALSE(decision.motorIntents.empty());
  const auto first = decision.motorIntents.front();
  ASSERT_TRUE(first.mode.has_value());
  EXPECT_EQ(*first.mode, MotorControlMode::Position);
  EXPECT_TRUE(first.position.has_value());
  EXPECT_TRUE(first.startMovement);
}
