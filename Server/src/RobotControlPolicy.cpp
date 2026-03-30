#include "RobotControlPolicy.hpp"

#include <Config.hpp>
#include <ExceptionUtils.hpp>
#include <Logger.hpp>
#include <magic_enum/magic_enum.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>

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

utl::EAxisState cycleAxisState(const utl::EAxisState state) {
  switch (state) {
    case utl::EAxisState::Locked: return utl::EAxisState::Slow;
    case utl::EAxisState::Slow:   return utl::EAxisState::Fast;
    case utl::EAxisState::Fast:   return utl::EAxisState::Locked;
  }
  return utl::EAxisState::Locked;
}
}  // namespace

std::optional<utl::EArm> RimoKunControlPolicy::motorToArm(
    const utl::EMotor motor) {
  switch (motor) {
    case utl::EMotor::XLeft:
    case utl::EMotor::YLeft:
      return utl::EArm::Left;
    case utl::EMotor::XRight:
    case utl::EMotor::YRight:
      return utl::EArm::Right;
    case utl::EMotor::ZLeft:
    case utl::EMotor::ZRight:
      return utl::EArm::Gantry;
  }
  return std::nullopt;
}

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
    if (const auto timeout = motionCfg["autoLockTimeoutMs"]; timeout) {
      _autoLockTimeoutMs = timeout.as<std::int64_t>();
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
      if (const auto stepsPerMm = axisNode["stepsPerMm"]; stepsPerMm) {
        axis.stepsPerMm = stepsPerMm.as<double>();
      }
      // Slow speed/accel
      if (const auto slowSpeed = axisNode["slowMaxLinearSpeedMmPerSec"]; slowSpeed) {
        axis.slowMaxLinearSpeedMmPerSec = slowSpeed.as<double>();
      }
      if (const auto slowAccel = axisNode["slowAcceleration001MsPerKHz"]; slowAccel) {
        axis.slowAcceleration001MsPerKHz = slowAccel.as<std::int32_t>();
      }
      if (const auto slowDecel = axisNode["slowDeceleration001MsPerKHz"]; slowDecel) {
        axis.slowDeceleration001MsPerKHz = slowDecel.as<std::int32_t>();
      }
      // Fast speed/accel
      if (const auto fastSpeed = axisNode["fastMaxLinearSpeedMmPerSec"]; fastSpeed) {
        axis.fastMaxLinearSpeedMmPerSec = fastSpeed.as<double>();
      }
      if (const auto fastAccel = axisNode["fastAcceleration001MsPerKHz"]; fastAccel) {
        axis.fastAcceleration001MsPerKHz = fastAccel.as<std::int32_t>();
      }
      if (const auto fastDecel = axisNode["fastDeceleration001MsPerKHz"]; fastDecel) {
        axis.fastDeceleration001MsPerKHz = fastDecel.as<std::int32_t>();
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
    const utl::RobotStatus& robotStatus) {
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
    return {.outputs = std::nullopt, .setToolChangerErrorBlinking = true,
            .armStates = _armStates};
  }
  setWarningFlag(_warningState.contecUnavailable, false);

  if (!inputs || !inputs->contains("button1") || !inputs->contains("button2")) {
    warnOnce(_warningState.missingInputSignals,
             "Missing required input signals 'button1'/'button2'. "
             "Skipping this control cycle.");
    return {.outputs = std::nullopt, .setToolChangerErrorBlinking = false,
            .armStates = _armStates};
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
            .setToolChangerErrorBlinking = false,
            .armStates = _armStates};
  }

  if (anyMotorInAlarm(robotStatus)) {
    warnOnce(_warningState.motorAlarmActive,
             "At least one motor is in alarm state. Motor commands are disabled.");
    return {.outputs = std::move(ioOutputs),
            .motorIntents = {},
            .setToolChangerErrorBlinking = false,
            .armStates = _armStates};
  }
  setWarningFlag(_warningState.motorAlarmActive, false);

  // Process joystick button presses to cycle arm states (Locked → Slow → Fast → Locked)
  const auto leftJs = joystickOrNeutral(robotStatus, utl::EArm::Left);
  const auto rightJs = joystickOrNeutral(robotStatus, utl::EArm::Right);
  const auto gantryJs = joystickOrNeutral(robotStatus, utl::EArm::Gantry);

  const auto now = std::chrono::steady_clock::now();

  const auto processButton = [&](const utl::EArm arm, const bool btnPressed) {
    const bool wasPressed = _prevButtonState[arm];
    if (btnPressed && !wasPressed) {
      // Rising edge: cycle to next state
      _armStates[arm] = cycleAxisState(_armStates[arm]);
      SPDLOG_INFO("Arm {} state changed to {}",
                  magic_enum::enum_name(arm),
                  magic_enum::enum_name(_armStates[arm]));
    }
    _prevButtonState[arm] = btnPressed;
  };

  processButton(utl::EArm::Left, leftJs.btn);
  processButton(utl::EArm::Right, rightJs.btn);
  processButton(utl::EArm::Gantry, gantryJs.btn);

  // Check auto-lock timeout: lock arms idle for too long
  for (auto arm : {utl::EArm::Left, utl::EArm::Right, utl::EArm::Gantry}) {
    if (_armStates[arm] == utl::EAxisState::Locked) {
      continue;
    }
    const auto lastMoveIt = _lastMovementTime.find(arm);
    if (lastMoveIt == _lastMovementTime.end()) {
      // Arm was just unlocked; set initial timestamp
      _lastMovementTime[arm] = now;
      continue;
    }
    const auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMoveIt->second)
            .count();
    if (elapsedMs >= _autoLockTimeoutMs) {
      SPDLOG_INFO("Arm {} auto-locked after {}ms of inactivity.",
                  magic_enum::enum_name(arm), elapsedMs);
      _armStates[arm] = utl::EAxisState::Locked;
      _lastMovementTime.erase(arm);
    }
  }

  std::vector<MotorIntent> motorIntents;
  motorIntents.reserve(6);
  std::map<utl::EMotor, MotorSpeedCommand> motorSpeedCommands;

  const auto appendSpeedIntent = [&](const utl::EMotor motorId,
                                     const double axisValue,
                                     const AxisConfig& axisCfg,
                                     const utl::EArm arm) {
    const auto armState = _armStates[arm];

    auto& runtime = _motorRuntime[motorId];
    const auto clampedAxis = clampAxis(axisValue);
    const auto active =
        std::abs(clampedAxis) >= axisCfg.neutralAxisActivationThreshold;

    // In Locked state: ignore all movement, emit stop if was active
    if (armState == utl::EAxisState::Locked) {
      motorSpeedCommands[motorId] = {.speedCommandPercent = 0.0,
                                     .modeMaxLinearSpeedMmPerSec = 0.0};
      runtime.hasLastAxis = false;
      runtime.lastDirection.reset();
      if (runtime.wasActive) {
        runtime.wasActive = false;
        MotorIntent intent;
        intent.motorId = motorId;
        intent.stopMovement = true;
        motorIntents.push_back(intent);
      }
      return;
    }

    // Select speed/accel based on arm state
    const double maxSpeed = (armState == utl::EAxisState::Slow)
                                ? axisCfg.slowMaxLinearSpeedMmPerSec
                                : axisCfg.fastMaxLinearSpeedMmPerSec;
    const std::int32_t accel = (armState == utl::EAxisState::Slow)
                                   ? axisCfg.slowAcceleration001MsPerKHz
                                   : axisCfg.fastAcceleration001MsPerKHz;
    const std::int32_t decel = (armState == utl::EAxisState::Slow)
                                   ? axisCfg.slowDeceleration001MsPerKHz
                                   : axisCfg.fastDeceleration001MsPerKHz;

    // Record speed command for GUI display
    motorSpeedCommands[motorId] = {
        .speedCommandPercent = active ? clampedAxis * 100.0 : 0.0,
        .modeMaxLinearSpeedMmPerSec = maxSpeed};

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

    // Movement is happening: update last movement time for auto-lock
    _lastMovementTime[arm] = now;

    const auto desiredDirection =
        clampedAxis >= 0.0 ? MotorControlDirection::Forward
                           : MotorControlDirection::Reverse;
    const auto shouldUpdateSpeed =
        !runtime.hasLastAxis ||
        std::abs(clampedAxis - runtime.lastAxis) >=
            axisCfg.speedUpdateAxisDeltaThreshold;

    if (!runtime.wasActive) {
      intent.mode = MotorControlMode::Speed;
      intent.direction = desiredDirection;
      intent.acceleration = accel;
      intent.deceleration = decel;
      intent.speed = std::max<std::int32_t>(
          1, speedStepsPerSecFromAxis(clampedAxis, maxSpeed, axisCfg.stepsPerMm));
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
          1, speedStepsPerSecFromAxis(clampedAxis, maxSpeed, axisCfg.stepsPerMm));
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
  appendSpeedIntent(utl::EMotor::XLeft, leftJs.x, _motion.leftArmX, utl::EArm::Left);
  appendSpeedIntent(utl::EMotor::YLeft, leftJs.y, _motion.leftArmY, utl::EArm::Left);
  appendSpeedIntent(utl::EMotor::XRight, rightJs.x, _motion.rightArmX, utl::EArm::Right);
  appendSpeedIntent(utl::EMotor::YRight, rightJs.y, _motion.rightArmY, utl::EArm::Right);

  // Gantry Z: single joystick axis drives both Z motors with the same command, through groups
  appendSpeedIntent(utl::EMotor::ZLeft, rightJs.y, _motion.gantryZ, utl::EArm::Gantry);

  return {.outputs = std::move(ioOutputs),
          .motorIntents = std::move(motorIntents),
          .setToolChangerErrorBlinking = false,
          .armStates = _armStates,
          .motorSpeedCommands = std::move(motorSpeedCommands)};
}

void RimoKunControlPolicy::warnOnce(bool& flag, const std::string& message) {
  if (flag) {
    return;
  }
  flag = true;
  SPDLOG_WARN("{}", message);
}

void RimoKunControlPolicy::setWarningFlag(bool& flag, const bool value) {
  flag = value;
}
