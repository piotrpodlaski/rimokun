#include <gtest/gtest.h>

#include <RobotControlPolicy.hpp>

#include <Config.hpp>

#include <algorithm>
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

std::filesystem::path writePolicyConfigLowThreshold() {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto path =
      std::filesystem::temp_directory_path() /
      ("rimokun_policy_low_threshold_" + std::to_string(stamp) + ".yaml");
  std::ofstream out(path);
  out << "classes:\n";
  out << "  Machine:\n";
  out << "    motion:\n";
  out << "      neutralAxisActivationThreshold: 0.05\n";
  out << "      speedUpdateAxisDeltaThreshold: 0.02\n";
  out << "      axes:\n";
  out << "        leftArmX: { maxLinearSpeedMmPerSec: 80.0, stepsPerMm: 10.0 }\n";
  out << "        leftArmY: { maxLinearSpeedMmPerSec: 80.0, stepsPerMm: 10.0 }\n";
  out << "        rightArmX: { maxLinearSpeedMmPerSec: 80.0, stepsPerMm: 10.0 }\n";
  out << "        rightArmY: { maxLinearSpeedMmPerSec: 80.0, stepsPerMm: 10.0 }\n";
  out << "        gantryZ: { maxLinearSpeedMmPerSec: 80.0, stepsPerMm: 10.0 }\n";
  out.close();
  return path;
}

void setAllComponentsOn(utl::RobotStatus& status) {
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::On;
  status.robotComponents[utl::ERobotComponent::MotorControl] = utl::ELEDState::On;
  status.robotComponents[utl::ERobotComponent::ControlPanel] = utl::ELEDState::On;
}

const IRobotControlPolicy::MotorIntent* findIntent(
    const std::vector<IRobotControlPolicy::MotorIntent>& intents,
    const utl::EMotor motorId) {
  const auto it = std::find_if(intents.begin(), intents.end(),
                               [&](const auto& intent) {
                                 return intent.motorId == motorId;
                               });
  return it == intents.end() ? nullptr : &(*it);
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
  const auto configPath = writePolicyConfigLowThreshold();
  utl::Config::instance().setConfigPath(configPath.string());
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  setAllComponentsOn(status);
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
  EXPECT_EQ(decision.motorIntents.size(), 6u);  // first active cycle emits all intents

  for (const auto& intent : decision.motorIntents) {
    ASSERT_TRUE(intent.mode.has_value());
    EXPECT_EQ(*intent.mode, MotorControlMode::Speed);
  }
  std::filesystem::remove(configPath);
}

TEST(RobotControlPolicyTests, RimoKunPolicyGantryUsesSingleAxisForBothZMotors) {
  const auto configPath = writePolicyConfigLowThreshold();
  utl::Config::instance().setConfigPath(configPath.string());
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  setAllComponentsOn(status);
  status.joystics[utl::EArm::Left] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = -0.9, .y = 1.0, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", true}},
      std::nullopt, MachineComponent::State::Normal, status);

  const auto* zLeft = findIntent(decision.motorIntents, utl::EMotor::ZLeft);
  const auto* zRight = findIntent(decision.motorIntents, utl::EMotor::ZRight);
  ASSERT_NE(zLeft, nullptr);
  ASSERT_NE(zRight, nullptr);
  ASSERT_TRUE(zLeft->mode.has_value());
  ASSERT_TRUE(zRight->mode.has_value());
  EXPECT_EQ(*zLeft->mode, MotorControlMode::Speed);
  EXPECT_EQ(*zRight->mode, MotorControlMode::Speed);
  ASSERT_TRUE(zLeft->direction.has_value());
  ASSERT_TRUE(zRight->direction.has_value());
  EXPECT_EQ(*zLeft->direction, *zRight->direction);
  ASSERT_TRUE(zLeft->speed.has_value());
  ASSERT_TRUE(zRight->speed.has_value());
  EXPECT_EQ(*zLeft->speed, *zRight->speed);
  std::filesystem::remove(configPath);
}

TEST(RobotControlPolicyTests, RimoKunPolicyAppliesPerMotorSpeedLimits) {
  const auto configPath = writePolicyConfigWithPerMotorLimits();
  utl::Config::instance().setConfigPath(configPath.string());

  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  setAllComponentsOn(status);
  status.joystics[utl::EArm::Left] = {.x = 1.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.0, .btn = false};

  const auto decisionLeft = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  const auto* xLeft = findIntent(decisionLeft.motorIntents, utl::EMotor::XLeft);
  ASSERT_NE(xLeft, nullptr);
  ASSERT_TRUE(xLeft->speed.has_value());
  EXPECT_EQ(*xLeft->speed, 400);   // 40 mm/s * 10 steps/mm

  RimoKunControlPolicy policyRight;
  status.joystics[utl::EArm::Left] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 1.0, .y = 0.0, .btn = false};
  const auto decisionRight = policyRight.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);
  const auto* xRight =
      findIntent(decisionRight.motorIntents, utl::EMotor::XRight);
  ASSERT_NE(xRight, nullptr);
  ASSERT_TRUE(xRight->speed.has_value());
  EXPECT_EQ(*xRight->speed, 1200); // 120 mm/s * 10 steps/mm

  std::filesystem::remove(configPath);
}

TEST(RobotControlPolicyTests, RimoKunPolicyUsesCommonThresholdWithPerAxisOverride) {
  const auto configPath = writePolicyConfigWithPerMotorLimits();
  utl::Config::instance().setConfigPath(configPath.string());

  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  setAllComponentsOn(status);
  status.joystics[utl::EArm::Left] = {.x = 0.2, .y = 0.2, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.2, .y = 0.2, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.2, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", false}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  const auto* leftX = findIntent(decision.motorIntents, utl::EMotor::XLeft);
  const auto* leftY = findIntent(decision.motorIntents, utl::EMotor::YLeft);
  const auto* rightX = findIntent(decision.motorIntents, utl::EMotor::XRight);
  const auto* rightY = findIntent(decision.motorIntents, utl::EMotor::YRight);
  const auto* zLeft = findIntent(decision.motorIntents, utl::EMotor::ZLeft);
  const auto* zRight = findIntent(decision.motorIntents, utl::EMotor::ZRight);

  ASSERT_NE(leftX, nullptr);  // leftArmX override 0.05
  EXPECT_TRUE(leftX->startMovement);
  EXPECT_EQ(leftY, nullptr);   // leftArmY uses common 0.50
  EXPECT_EQ(rightX, nullptr);  // rightArmX uses common 0.50
  EXPECT_EQ(rightY, nullptr);  // rightArmY uses common 0.50
  EXPECT_EQ(zLeft, nullptr);   // gantryZ uses common 0.50
  EXPECT_EQ(zRight, nullptr);

  std::filesystem::remove(configPath);
}

TEST(RobotControlPolicyTests,
     RimoKunPolicyWithoutComponentStatusStaysFailSafeAndRequestsBlinking) {
  RimoKunControlPolicy policy;
  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, utl::RobotStatus{});

  EXPECT_TRUE(decision.setToolChangerErrorBlinking);
  EXPECT_FALSE(decision.outputs.has_value());
  EXPECT_TRUE(decision.motorIntents.empty());
}

TEST(RobotControlPolicyTests, RimoKunPolicySkipsMotorCommandsWhenMotorControlDown) {
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::On;
  status.robotComponents[utl::ERobotComponent::MotorControl] = utl::ELEDState::Error;
  status.robotComponents[utl::ERobotComponent::ControlPanel] = utl::ELEDState::On;
  status.joystics[utl::EArm::Left] = {.x = 0.8, .y = 0.8, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.8, .y = 0.8, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.8, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  EXPECT_FALSE(decision.setToolChangerErrorBlinking);
  ASSERT_TRUE(decision.outputs.has_value());
  EXPECT_TRUE(decision.outputs->at("light1"));
  EXPECT_FALSE(decision.outputs->at("light2"));
  EXPECT_TRUE(decision.motorIntents.empty());
}

TEST(RobotControlPolicyTests,
     RimoKunPolicyTreatsWarningComponentStateAsOperational) {
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::Warning;
  status.robotComponents[utl::ERobotComponent::MotorControl] =
      utl::ELEDState::Warning;
  status.robotComponents[utl::ERobotComponent::ControlPanel] =
      utl::ELEDState::Warning;
  status.joystics[utl::EArm::Left] = {.x = 0.8, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.0, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  EXPECT_FALSE(decision.setToolChangerErrorBlinking);
  ASSERT_TRUE(decision.outputs.has_value());
  EXPECT_TRUE(decision.outputs->at("light1"));
  EXPECT_FALSE(decision.outputs->at("light2"));
  EXPECT_FALSE(decision.motorIntents.empty());
}

TEST(RobotControlPolicyTests, RimoKunPolicyDisablesMotionWhenAnyMotorIsInAlarm) {
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  setAllComponentsOn(status);
  status.motors[utl::EMotor::XLeft].state = utl::ELEDState::Error;
  status.motors[utl::EMotor::XLeft].flags[utl::EMotorStatusFlags::Alarm] =
      utl::ELEDState::Error;
  status.joystics[utl::EArm::Left] = {.x = 0.8, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.0, .btn = false};

  const auto decision = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", false}},
      std::nullopt, MachineComponent::State::Normal, status);

  ASSERT_TRUE(decision.outputs.has_value());
  EXPECT_TRUE(decision.outputs->at("light1"));
  EXPECT_FALSE(decision.outputs->at("light2"));
  EXPECT_TRUE(decision.motorIntents.empty());
}

TEST(RobotControlPolicyTests, RimoKunPolicyUpdatesSpeedOnlyAfterAxisDeltaThreshold) {
  RimoKunControlPolicy policy;
  utl::RobotStatus status;
  setAllComponentsOn(status);
  status.joystics[utl::EArm::Left] = {.x = 0.50, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Right] = {.x = 0.0, .y = 0.0, .btn = false};
  status.joystics[utl::EArm::Gantry] = {.x = 0.0, .y = 0.0, .btn = false};

  const auto first = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", true}},
      std::nullopt, MachineComponent::State::Normal, status);
  const auto* firstX = findIntent(first.motorIntents, utl::EMotor::XLeft);
  ASSERT_NE(firstX, nullptr);
  ASSERT_TRUE(firstX->speed.has_value());
  EXPECT_TRUE(firstX->startMovement);

  status.joystics[utl::EArm::Left] = {.x = 0.51, .y = 0.0, .btn = false};  // delta 0.01 < 0.02
  const auto second = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", true}},
      std::nullopt, MachineComponent::State::Normal, status);
  const auto* secondX = findIntent(second.motorIntents, utl::EMotor::XLeft);
  EXPECT_EQ(secondX, nullptr);

  status.joystics[utl::EArm::Left] = {.x = 0.54, .y = 0.0, .btn = false};  // delta 0.04 >= 0.02
  const auto third = policy.decide(
      IRobotControlPolicy::SignalMap{{"button1", true}, {"button2", true}},
      std::nullopt, MachineComponent::State::Normal, status);
  const auto* thirdX = findIntent(third.motorIntents, utl::EMotor::XLeft);
  ASSERT_NE(thirdX, nullptr);
  ASSERT_TRUE(thirdX->speed.has_value());
  EXPECT_FALSE(thirdX->startMovement);
}
