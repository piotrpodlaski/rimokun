#include "RobotControlPolicy.hpp"

#include <Config.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>

IRobotControlPolicy::ControlDecision DefaultRobotControlPolicy::decide(
    const std::optional<SignalMap>& inputs,
    const std::optional<SignalMap>& currentOutputs,
    const MachineComponent::State contecState,
    const utl::RobotStatus& robotStatus) const {
  (void)currentOutputs;
  (void)robotStatus;
  if (!inputs || contecState == MachineComponent::State::Error) {
    return {.outputs = std::nullopt, .setToolChangerErrorBlinking = true};
  }
  if (!inputs->contains("button1") || !inputs->contains("button2")) {
    throw std::runtime_error(
        "Missing required input signals 'button1'/'button2' for control policy.");
  }

  SignalMap desiredOutputs;
  desiredOutputs["light1"] = inputs->at("button1");
  desiredOutputs["light2"] = inputs->at("button2");
  return {.outputs = std::move(desiredOutputs),
          .setToolChangerErrorBlinking = false};
}

namespace {
double clampAxis(const double value) {
  return std::clamp(value, -1.0, 1.0);
}

utl::JoystickStatus joystickOrNeutral(const utl::RobotStatus& status,
                                      const utl::EArm arm) {
  const auto it = status.joystics.find(arm);
  if (it == status.joystics.end()) {
    return {.x = 0.0, .y = 0.0, .btn = false};
  }
  return it->second;
}

std::int32_t speedStepsPerSecFromAxis(const double axis,
                                      const double maxLinearSpeedMmPerSec,
                                      const double stepsPerMm) {
  const auto linear = std::abs(clampAxis(axis)) * maxLinearSpeedMmPerSec;
  return static_cast<std::int32_t>(std::llround(linear * stepsPerMm));
}
}  // namespace

RimoKunControlPolicy::RimoKunControlPolicy() {
  try {
    const auto machineCfg = utl::Config::instance().getClassConfig("Machine");
    const auto motionCfg = machineCfg["motion"];
    if (!motionCfg || !motionCfg.IsMap()) {
      return;
    }
    if (const auto commonThreshold = motionCfg["neutralAxisActivationThreshold"];
        commonThreshold) {
      const auto value = commonThreshold.as<double>();
      _motion.leftArmX.neutralAxisActivationThreshold = value;
      _motion.leftArmY.neutralAxisActivationThreshold = value;
      _motion.rightArmX.neutralAxisActivationThreshold = value;
      _motion.rightArmY.neutralAxisActivationThreshold = value;
      _motion.gantryZ.neutralAxisActivationThreshold = value;
    }
    const auto axesCfg = motionCfg["axes"];
    if (!axesCfg || !axesCfg.IsMap()) {
      return;
    }

    const auto readAxis = [](const YAML::Node& axesNode, const char* key,
                             AxisConfig& axis) {
      const auto axisNode = axesNode[key];
      if (!axisNode || !axisNode.IsMap()) {
        return;
      }
      if (const auto threshold = axisNode["neutralAxisActivationThreshold"];
          threshold) {
        axis.neutralAxisActivationThreshold = threshold.as<double>();
      }
      if (const auto maxSpeed = axisNode["maxLinearSpeedMmPerSec"]; maxSpeed) {
        axis.maxLinearSpeedMmPerSec = maxSpeed.as<double>();
      }
      if (const auto stepsPerMm = axisNode["stepsPerMm"]; stepsPerMm) {
        axis.stepsPerMm = stepsPerMm.as<double>();
      }
    };

    readAxis(axesCfg, "leftArmX", _motion.leftArmX);
    readAxis(axesCfg, "leftArmY", _motion.leftArmY);
    readAxis(axesCfg, "rightArmX", _motion.rightArmX);
    readAxis(axesCfg, "rightArmY", _motion.rightArmY);
    readAxis(axesCfg, "gantryZ", _motion.gantryZ);
    if (const auto gantryYNode = axesCfg["gantryY"];
        gantryYNode && gantryYNode.IsMap()) {
      readAxis(axesCfg, "gantryY", _motion.gantryZ);
    }
  } catch (const std::exception&) {
    // Keep defaults when config is unavailable in unit-test contexts.
  }
}

IRobotControlPolicy::ControlDecision RimoKunControlPolicy::decide(
    const std::optional<SignalMap>& inputs, const std::optional<SignalMap>& outputs,
    const MachineComponent::State contecState,
    const utl::RobotStatus& robotStatus) const {
  (void)outputs;
  if (!inputs || contecState == MachineComponent::State::Error) {
    return {.outputs = std::nullopt, .setToolChangerErrorBlinking = true};
  }
  if (!inputs->contains("button1") || !inputs->contains("button2")) {
    throw std::runtime_error(
        "Missing required input signals 'button1'/'button2' for control policy.");
  }

  SignalMap ioOutputs;
  ioOutputs["light1"] = inputs->at("button1");
  ioOutputs["light2"] = inputs->at("button2");

  std::vector<MotorIntent> motorIntents;
  motorIntents.reserve(6);
  const auto leftJs = joystickOrNeutral(robotStatus, utl::EArm::Left);
  const auto rightJs = joystickOrNeutral(robotStatus, utl::EArm::Right);
  const auto gantryJs = joystickOrNeutral(robotStatus, utl::EArm::Gantry);

  const auto appendSpeedIntent = [&](const utl::EMotor motorId,
                                     const double axisValue,
                                     const AxisConfig& axisCfg) {
    MotorIntent intent;
    intent.motorId = motorId;
    if (std::abs(clampAxis(axisValue)) <
        axisCfg.neutralAxisActivationThreshold) {
      intent.stopMovement = true;
      motorIntents.push_back(intent);
      return;
    }
    intent.mode = MotorControlMode::Speed;  // default operating mode
    intent.direction = axisValue >= 0.0 ? MotorControlDirection::Forward
                                        : MotorControlDirection::Reverse;
    intent.speed = std::max<std::int32_t>(
        1, speedStepsPerSecFromAxis(axisValue, axisCfg.maxLinearSpeedMmPerSec,
                                    axisCfg.stepsPerMm));
    intent.startMovement = true;
    motorIntents.push_back(intent);
  };

  // One joystick per arm: left joystick drives left X/Y, right joystick drives right X/Y.
  appendSpeedIntent(utl::EMotor::XLeft, leftJs.x, _motion.leftArmX);
  appendSpeedIntent(utl::EMotor::YLeft, leftJs.y, _motion.leftArmY);
  appendSpeedIntent(utl::EMotor::XRight, rightJs.x, _motion.rightArmX);
  appendSpeedIntent(utl::EMotor::YRight, rightJs.y, _motion.rightArmY);

  // Gantry Z: single joystick axis drives both Z motors with the same command.
  appendSpeedIntent(utl::EMotor::ZLeft, gantryJs.y, _motion.gantryZ);
  appendSpeedIntent(utl::EMotor::ZRight, gantryJs.y, _motion.gantryZ);

  return {.outputs = std::move(ioOutputs),
          .motorIntents = std::move(motorIntents),
          .setToolChangerErrorBlinking = false};
}
