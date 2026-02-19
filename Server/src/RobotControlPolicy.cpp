#include "RobotControlPolicy.hpp"

#include <Config.hpp>
#include <Logger.hpp>

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

std::optional<utl::ELEDState> componentStateFromStatus(
    const utl::RobotStatus& status, const utl::ERobotComponent component) {
  const auto it = status.robotComponents.find(component);
  if (it == status.robotComponents.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool isComponentOperational(const std::optional<utl::ELEDState>& state) {
  return state.has_value() &&
         (*state == utl::ELEDState::On || *state == utl::ELEDState::Warning);
}

bool isMotorInAlarm(const utl::SingleMotorStatus& motorStatus) {
  if (motorStatus.state == utl::ELEDState::Error) {
    return true;
  }
  const auto alarmIt = motorStatus.flags.find(utl::EMotorStatusFlags::Alarm);
  return alarmIt != motorStatus.flags.end() &&
         alarmIt->second == utl::ELEDState::Error;
}

bool anyMotorInAlarm(const utl::RobotStatus& status) {
  return std::any_of(status.motors.begin(), status.motors.end(),
                     [](const auto& kv) { return isMotorInAlarm(kv.second); });
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
    if (const auto commonDelta = motionCfg["speedUpdateAxisDeltaThreshold"];
        commonDelta) {
      const auto value = commonDelta.as<double>();
      _motion.leftArmX.speedUpdateAxisDeltaThreshold = value;
      _motion.leftArmY.speedUpdateAxisDeltaThreshold = value;
      _motion.rightArmX.speedUpdateAxisDeltaThreshold = value;
      _motion.rightArmY.speedUpdateAxisDeltaThreshold = value;
      _motion.gantryZ.speedUpdateAxisDeltaThreshold = value;
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
      if (const auto deltaThreshold = axisNode["speedUpdateAxisDeltaThreshold"];
          deltaThreshold) {
        axis.speedUpdateAxisDeltaThreshold = deltaThreshold.as<double>();
      }
      if (const auto maxSpeed = axisNode["maxLinearSpeedMmPerSec"]; maxSpeed) {
        axis.maxLinearSpeedMmPerSec = maxSpeed.as<double>();
      }
      if (const auto stepsPerMm = axisNode["stepsPerMm"]; stepsPerMm) {
        axis.stepsPerMm = stepsPerMm.as<double>();
      }
      if (const auto acceleration = axisNode["acceleration001MsPerKHz"];
          acceleration) {
        axis.acceleration001MsPerKHz = acceleration.as<std::int32_t>();
      }
      if (const auto deceleration = axisNode["deceleration001MsPerKHz"];
          deceleration) {
        axis.deceleration001MsPerKHz = deceleration.as<std::int32_t>();
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

  const auto contecStatus =
      componentStateFromStatus(robotStatus, utl::ERobotComponent::Contec);
  const auto motorControlStatus =
      componentStateFromStatus(robotStatus, utl::ERobotComponent::MotorControl);
  const auto controlPanelStatus =
      componentStateFromStatus(robotStatus, utl::ERobotComponent::ControlPanel);

  if (!contecStatus.has_value()) {
    warnOnce(_warningState.missingContecStatus,
             "Robot status does not include Contec state yet. "
             "Control policy is waiting for component initialization.");
  } else {
    setWarningFlag(_warningState.missingContecStatus, false);
  }
  if (!motorControlStatus.has_value()) {
    warnOnce(_warningState.missingMotorControlStatus,
             "Robot status does not include MotorControl state yet. "
             "Motor commands are suspended.");
  } else {
    setWarningFlag(_warningState.missingMotorControlStatus, false);
  }
  if (!controlPanelStatus.has_value()) {
    warnOnce(_warningState.missingControlPanelStatus,
             "Robot status does not include ControlPanel state yet. "
             "Joystick-driven control is suspended.");
  } else {
    setWarningFlag(_warningState.missingControlPanelStatus, false);
  }

  const bool contecAvailable =
      contecState != MachineComponent::State::Error &&
      isComponentOperational(contecStatus);
  if (!contecAvailable) {
    warnOnce(_warningState.contecUnavailable,
             "Contec is unavailable. Tool changer and light outputs are disabled.");
    return {.outputs = std::nullopt, .setToolChangerErrorBlinking = true};
  }
  setWarningFlag(_warningState.contecUnavailable, false);

  if (!inputs || !inputs->contains("button1") || !inputs->contains("button2")) {
    warnOnce(_warningState.missingInputSignals,
             "Missing required input signals 'button1'/'button2'. "
             "Skipping this control cycle.");
    return {.outputs = std::nullopt, .setToolChangerErrorBlinking = false};
  }
  setWarningFlag(_warningState.missingInputSignals, false);

  SignalMap ioOutputs;
  ioOutputs["light1"] = inputs->at("button1");
  ioOutputs["light2"] = inputs->at("button2");

  const bool motorControlAvailable = isComponentOperational(motorControlStatus);
  const bool controlPanelAvailable = isComponentOperational(controlPanelStatus);

  if (!motorControlAvailable) {
    warnOnce(_warningState.motorControlUnavailable,
             "MotorControl is unavailable. Motor commands are suspended.");
  } else {
    setWarningFlag(_warningState.motorControlUnavailable, false);
  }
  if (!controlPanelAvailable) {
    warnOnce(_warningState.controlPanelUnavailable,
             "ControlPanel is unavailable. Joystick-driven control is suspended.");
  } else {
    setWarningFlag(_warningState.controlPanelUnavailable, false);
  }

  if (!motorControlAvailable || !controlPanelAvailable) {
    return {.outputs = std::move(ioOutputs),
            .motorIntents = {},
            .setToolChangerErrorBlinking = false};
  }

  if (anyMotorInAlarm(robotStatus)) {
    warnOnce(_warningState.motorAlarmActive,
             "At least one motor is in alarm state. Motor commands are disabled.");
    return {.outputs = std::move(ioOutputs),
            .motorIntents = {},
            .setToolChangerErrorBlinking = false};
  }
  setWarningFlag(_warningState.motorAlarmActive, false);

  std::vector<MotorIntent> motorIntents;
  motorIntents.reserve(6);
  const auto leftJs = joystickOrNeutral(robotStatus, utl::EArm::Left);
  const auto rightJs = joystickOrNeutral(robotStatus, utl::EArm::Right);
  const auto gantryJs = joystickOrNeutral(robotStatus, utl::EArm::Gantry);

  const auto appendSpeedIntent = [&](const utl::EMotor motorId,
                                     const double axisValue,
                                     const AxisConfig& axisCfg) {
    std::lock_guard<std::mutex> runtimeLock(_runtimeMutex);
    auto& runtime = _motorRuntime[motorId];
    const auto clampedAxis = clampAxis(axisValue);
    const auto active =
        std::abs(clampedAxis) >= axisCfg.neutralAxisActivationThreshold;

    MotorIntent intent;
    intent.motorId = motorId;
    if (!active) {
      runtime.hasLastAxis = false;
      runtime.lastDirection.reset();
      if (!runtime.wasActive) {
        return;
      }
      runtime.wasActive = false;
      intent.stopMovement = true;
      motorIntents.push_back(intent);
      return;
    }

    const auto desiredDirection =
        clampedAxis >= 0.0 ? MotorControlDirection::Forward
                           : MotorControlDirection::Reverse;
    const auto shouldUpdateSpeed =
        !runtime.hasLastAxis ||
        std::abs(clampedAxis - runtime.lastAxis) >=
            axisCfg.speedUpdateAxisDeltaThreshold;

    if (!runtime.wasActive) {
      intent.mode = MotorControlMode::Speed;  // default operating mode
      intent.direction = desiredDirection;
      intent.acceleration = axisCfg.acceleration001MsPerKHz;
      intent.deceleration = axisCfg.deceleration001MsPerKHz;
      intent.speed = std::max<std::int32_t>(
          1, speedStepsPerSecFromAxis(clampedAxis, axisCfg.maxLinearSpeedMmPerSec,
                                    axisCfg.stepsPerMm));
      intent.startMovement = true;
      runtime.wasActive = true;
      runtime.hasLastAxis = true;
      runtime.lastAxis = clampedAxis;
      runtime.lastDirection = desiredDirection;
      motorIntents.push_back(intent);
      return;
    }

    if (!runtime.lastDirection.has_value() ||
        runtime.lastDirection != desiredDirection) {
      intent.direction = desiredDirection;
      runtime.lastDirection = desiredDirection;
    }
    if (shouldUpdateSpeed) {
      intent.speed = std::max<std::int32_t>(
          1, speedStepsPerSecFromAxis(clampedAxis, axisCfg.maxLinearSpeedMmPerSec,
                                      axisCfg.stepsPerMm));
      runtime.hasLastAxis = true;
      runtime.lastAxis = clampedAxis;
    }
    runtime.wasActive = true;

    if (intent.direction || intent.speed || intent.startMovement || intent.stopMovement ||
        intent.mode || intent.position) {
      motorIntents.push_back(intent);
    }
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

void RimoKunControlPolicy::warnOnce(bool& flag, const std::string& message) const {
  std::lock_guard<std::mutex> lock(_warningMutex);
  if (flag) {
    return;
  }
  flag = true;
  SPDLOG_WARN("{}", message);
}

void RimoKunControlPolicy::setWarningFlag(bool& flag, const bool value) const {
  std::lock_guard<std::mutex> lock(_warningMutex);
  flag = value;
}
