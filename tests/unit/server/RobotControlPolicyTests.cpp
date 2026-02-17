#include <gtest/gtest.h>

#include <RobotControlPolicy.hpp>

#include <Config.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
std::filesystem::path writePolicyConfigWithPerMotorLimits() {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto path = std::filesystem::temp_directory_path() /
                    ("rimokun_policy_limits_" + std::to_string(stamp) + ".yaml");
  std::ofstream out(path);
  out << "classes:\n";
  out << "  Machine:\n";
  out << "    motion:\n";
  out << "      neutralAxisActivationThreshold: 0.50\n";
  out << "      axes:\n";
  out << "        leftArmX:\n";
  out << "          neutralAxisActivationThreshold: 0.05\n";
  out << "          maxLinearSpeedMmPerSec: 40.0\n";
  out << "          stepsPerMm: 10.0\n";
  out << "        leftArmY:\n";
  out << "          maxLinearSpeedMmPerSec: 100.0\n";
  out << "          stepsPerMm: 10.0\n";
  out << "        rightArmX:\n";
  out << "          maxLinearSpeedMmPerSec: 120.0\n";
  out << "          stepsPerMm: 10.0\n";
  out << "        rightArmY:\n";
  out << "          maxLinearSpeedMmPerSec: 100.0\n";
  out << "          stepsPerMm: 10.0\n";
  out << "        gantryZ:\n";
  out << "          maxLinearSpeedMmPerSec: 100.0\n";
  out << "          stepsPerMm: 10.0\n";
  out.close();
  return path;
}
}  // namespace

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
  status.joystics[utl::EArm::Left] = {.x = 0.4, .y = 0.7, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = -0.6, .y = -0.5, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.2, .y = 0.3, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  EXPECT_FALSE(decision.setToolChangerErrorBlinking);
  ASSERT_TRUE(decision.outputs.has_value());
  EXPECT_TRUE(decision.outputs->at("light1"));
  EXPECT_FALSE(decision.outputs->at("light2"));
  EXPECT_EQ(decision.motorIntents.size(), 6u);

  for (const auto& intent : decision.motorIntents) {
    ASSERT_TRUE(intent.mode.has_value());
    EXPECT_EQ(*intent.mode, MotorControlMode::Speed);
  }
}

TEST(RobotControlPolicyTests, RimoKunPolicyGantryUsesSingleAxisForBothZMotors) {
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  status.joystics[utl::EArm::Left] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = -0.9, .y = 0.45, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", true}},
      std::nullopt, MachineComponent::State::Normal, status);

  ASSERT_EQ(decision.motorIntents.size(), 6u);
  const auto zLeft = decision.motorIntents[4];
  const auto zRight = decision.motorIntents[5];
  ASSERT_TRUE(zLeft.mode.has_value());
  ASSERT_TRUE(zRight.mode.has_value());
  EXPECT_EQ(*zLeft.mode, MotorControlMode::Speed);
  EXPECT_EQ(*zRight.mode, MotorControlMode::Speed);
  ASSERT_TRUE(zLeft.direction.has_value());
  ASSERT_TRUE(zRight.direction.has_value());
  EXPECT_EQ(*zLeft.direction, *zRight.direction);
  ASSERT_TRUE(zLeft.speed.has_value());
  ASSERT_TRUE(zRight.speed.has_value());
  EXPECT_EQ(*zLeft.speed, *zRight.speed);
}

TEST(RobotControlPolicyTests, RimoKunPolicyAppliesPerMotorSpeedLimits) {
  const auto configPath = writePolicyConfigWithPerMotorLimits();
  utl::Config::instance().setConfigPath(configPath.string());

  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  status.joystics[utl::EArm::Left] = {.x = 1.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 1.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.0, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  ASSERT_EQ(decision.motorIntents.size(), 6u);
  const auto xLeft = decision.motorIntents[0];
  const auto xRight = decision.motorIntents[2];
  ASSERT_TRUE(xLeft.speed.has_value());
  ASSERT_TRUE(xRight.speed.has_value());
  EXPECT_LT(*xLeft.speed, *xRight.speed);
  EXPECT_EQ(*xLeft.speed, 400);   // 40 mm/s * 10 steps/mm
  EXPECT_EQ(*xRight.speed, 1200); // 120 mm/s * 10 steps/mm

  std::filesystem::remove(configPath);
}

TEST(RobotControlPolicyTests, RimoKunPolicyUsesCommonThresholdWithPerAxisOverride) {
  const auto configPath = writePolicyConfigWithPerMotorLimits();
  utl::Config::instance().setConfigPath(configPath.string());

  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  status.joystics[utl::EArm::Left] = {.x = 0.2, .y = 0.2, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.2, .y = 0.2, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.2, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  ASSERT_EQ(decision.motorIntents.size(), 6u);
  EXPECT_TRUE(decision.motorIntents[0].startMovement);  // leftArmX override 0.05
  EXPECT_FALSE(decision.motorIntents[1].startMovement); // leftArmY uses common 0.50
  EXPECT_FALSE(decision.motorIntents[2].startMovement); // rightArmX uses common 0.50
  EXPECT_FALSE(decision.motorIntents[3].startMovement); // rightArmY uses common 0.50
  EXPECT_FALSE(decision.motorIntents[4].startMovement); // gantryZ uses common 0.50
  EXPECT_FALSE(decision.motorIntents[5].startMovement);

  std::filesystem::remove(configPath);
}
