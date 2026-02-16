#include <gtest/gtest.h>

#include <RobotControlPolicy.hpp>

#include <stdexcept>

TEST(RobotControlPolicyTests, ErrorStateRequestsToolChangerErrorBlinking) {
  DefaultRobotControlPolicy policy;
  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      MachineComponent::State::Error, utl::RobotStatus{});

  EXPECT_TRUE(decision.setToolChangerErrorBlinking);
  EXPECT_FALSE(decision.outputs.has_value());
}

TEST(RobotControlPolicyTests, MissingInputsRequestToolChangerErrorBlinking) {
  DefaultRobotControlPolicy policy;
  const auto decision = policy.decide(std::nullopt, MachineComponent::State::Normal,
                                      utl::RobotStatus{});

  EXPECT_TRUE(decision.setToolChangerErrorBlinking);
  EXPECT_FALSE(decision.outputs.has_value());
}

TEST(RobotControlPolicyTests, NormalInputsMapButtonsToLights) {
  DefaultRobotControlPolicy policy;
  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
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
                          MachineComponent::State::Normal, utl::RobotStatus{}),
      std::runtime_error);
}
