#include "RobotControlPolicy.hpp"

#include <algorithm>
#include <cmath>
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

std::int32_t scaledSpeedFromAxis(const double axis, const std::int32_t maxSpeed) {
  const auto magnitude = std::abs(clampAxis(axis));
  return static_cast<std::int32_t>(magnitude * static_cast<double>(maxSpeed));
}

std::int32_t scaledPositionFromAxis(const double axis,
                                    const std::int32_t maxPositionStep) {
  return static_cast<std::int32_t>(
      clampAxis(axis) * static_cast<double>(maxPositionStep));
}

utl::JoystickStatus joystickOrNeutral(const utl::RobotStatus& status,
                                      const utl::EArm arm) {
  const auto it = status.joystics.find(arm);
  if (it == status.joystics.end()) {
    return {.x = 0.0, .y = 0.0, .btn = false};
  }
  return it->second;
}

void appendAxisIntents(std::vector<IRobotControlPolicy::MotorIntent>& intents,
                       const utl::EMotor m1, const utl::EMotor m2,
                       const utl::JoystickStatus& joystick,
                       const double deadband, const std::int32_t maxSpeed,
                       const std::int32_t maxPositionStep) {
  const auto speedCmd = scaledSpeedFromAxis(joystick.y, maxSpeed);
  const auto direction = joystick.y >= 0.0 ? MotorControlDirection::Forward
                                           : MotorControlDirection::Reverse;
  const auto positionStep = scaledPositionFromAxis(joystick.x, maxPositionStep);

  if (std::abs(clampAxis(joystick.y)) < deadband &&
      std::abs(clampAxis(joystick.x)) < deadband) {
    intents.push_back({.motorId = m1, .stopMovement = true});
    intents.push_back({.motorId = m2, .stopMovement = true});
    return;
  }

  for (const auto motor : {m1, m2}) {
    IRobotControlPolicy::MotorIntent intent;
    intent.motorId = motor;
    if (joystick.btn) {
      intent.mode = MotorControlMode::Position;
      intent.position = positionStep;
      intent.speed = std::max<std::int32_t>(100, maxSpeed / 2);
      intent.startMovement = true;
    } else {
      intent.mode = MotorControlMode::Speed;
      intent.direction = direction;
      intent.speed = std::max<std::int32_t>(50, speedCmd);
      intent.startMovement = true;
    }
    intents.push_back(intent);
  }
}
}  // namespace

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

  constexpr double kDeadband = 0.10;
  constexpr std::int32_t kMaxSpeed = 3000;
  constexpr std::int32_t kMaxPositionStep = 2000;

  std::vector<MotorIntent> motorIntents;
  motorIntents.reserve(6);
  appendAxisIntents(motorIntents, utl::EMotor::XLeft, utl::EMotor::XRight,
                    joystickOrNeutral(robotStatus, utl::EArm::Left), kDeadband,
                    kMaxSpeed, kMaxPositionStep);
  appendAxisIntents(motorIntents, utl::EMotor::YLeft, utl::EMotor::YRight,
                    joystickOrNeutral(robotStatus, utl::EArm::Right), kDeadband,
                    kMaxSpeed, kMaxPositionStep);
  appendAxisIntents(motorIntents, utl::EMotor::ZLeft, utl::EMotor::ZRight,
                    joystickOrNeutral(robotStatus, utl::EArm::Gantry), kDeadband,
                    kMaxSpeed, kMaxPositionStep);

  return {.outputs = std::move(ioOutputs),
          .motorIntents = std::move(motorIntents),
          .setToolChangerErrorBlinking = false};
}
