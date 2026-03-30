#include "MachineController.hpp"

#include <Logger.hpp>
#include <ExceptionUtils.hpp>
#include <TimingMetrics.hpp>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>

MachineController::MachineController(IoOps io,
                                     MotorOps motorOps,
                                     utl::RobotStatus& robotStatus,
                                     std::unique_ptr<IRobotControlPolicy> controlPolicy)
    : _io(std::move(io)),
      _motorOps(std::move(motorOps)),
      _robotStatus(robotStatus),
      _controlPolicy(std::move(controlPolicy)) {
  if (!_controlPolicy) {
    utl::throwRuntimeError("MachineController requires a non-null control policy.");
  }
  if (!_motorOps.isConfigured) {
    utl::throwRuntimeError("MachineController requires motor availability callback.");
  }
}

void MachineController::runControlLoopTasks() {
  RIMO_TIMED_SCOPE("MachineController::runControlLoopTasks");

  if (_motorOps.onAlarmCleared) {
    for (const auto& [motorId, motorStatus] : _robotStatus.motors) {
      const auto alarmIt = motorStatus.flags.find(utl::EMotorStatusFlags::Alarm);
      const bool isInAlarm = alarmIt != motorStatus.flags.end() &&
                             alarmIt->second == utl::ELEDState::Error;
      const bool wasInAlarm = _motorWasInAlarm[motorId];
      if (wasInAlarm && !isInAlarm) {
        _motorOps.onAlarmCleared(motorId);
      }
      _motorWasInAlarm[motorId] = isInAlarm;
    }
  }

  const auto decision =
      _controlPolicy->decide(_io.readInputs(), _io.readOutputs(), _io.contecState(),
                             _robotStatus);

  // Publish arm states and per-motor speed commands so the GUI can display them
  _robotStatus.armStates = decision.armStates;
  for (const auto& [motorId, cmd] : decision.motorSpeedCommands) {
    auto& ms = _robotStatus.motors[motorId];
    ms.speedCommandPercent = cmd.speedCommandPercent;
    ms.modeMaxLinearSpeedMmPerSec = cmd.modeMaxLinearSpeedMmPerSec;
  }

  if (decision.setToolChangerErrorBlinking) {
    return;
  }
  if (decision.outputs) {
    _io.setOutputs(*decision.outputs);
  }
  for (const auto& intent : decision.motorIntents) {
    if (!_motorOps.isConfigured(intent.motorId)) {
      if (!_missingMotorWarned[intent.motorId]) {
        SPDLOG_WARN(
            "Control policy emitted command for motor {} which is not configured. "
            "Ignoring commands for this motor.",
            magic_enum::enum_name(intent.motorId));
        _missingMotorWarned[intent.motorId] = true;
      }
      continue;
    }
    _missingMotorWarned[intent.motorId] = false;
    if (intent.mode && _motorOps.setMode) {
      _motorOps.setMode(intent.motorId, *intent.mode);
    }
    if (intent.direction && _motorOps.setDirection) {
      _motorOps.setDirection(intent.motorId, *intent.direction);
    }
    if (intent.speed && _motorOps.setSpeed) {
      _motorOps.setSpeed(intent.motorId, *intent.speed);
    }
    if (intent.acceleration && _motorOps.setAcceleration) {
      _motorOps.setAcceleration(intent.motorId, *intent.acceleration);
    }
    if (intent.deceleration && _motorOps.setDeceleration) {
      _motorOps.setDeceleration(intent.motorId, *intent.deceleration);
    }
    if (intent.position && _motorOps.setPosition) {
      _motorOps.setPosition(intent.motorId, *intent.position);
    }
    if (intent.stopMovement) {
      if (_motorOps.stop) _motorOps.stop(intent.motorId);
      continue;
    }
    if (intent.startMovement) {
      if (_motorOps.start) _motorOps.start(intent.motorId);
    }
  }
}

void MachineController::handleToolChangerCommand(
    const cmd::ToolChangerCommand& command) const {
  RIMO_TIMED_SCOPE("MachineController::handleToolChangerCommand");
  SPDLOG_INFO("Changing status of '{}' tool changer to '{}'",
              magic_enum::enum_name(command.arm),
              magic_enum::enum_name(command.action));
  if (_io.contecState() == MachineComponent::State::Error) {
    utl::throwRuntimeError(
        "Contec is in error state. Not possible to alter tool changer state!");
  }
  signal_map_t outputSignals;
  if (command.arm == utl::EArm::Left) {
    outputSignals["toolChangerLeft"] =
        (command.action == utl::EToolChangerAction::Open);
  } else if (command.arm == utl::EArm::Right) {
    outputSignals["toolChangerRight"] =
        (command.action == utl::EToolChangerAction::Open);
  }
  _io.setOutputs(outputSignals);
  if (!_io.readOutputs()) {
    utl::throwRuntimeError(
        "Unable to read status of output signals, tool changer status update "
        "failed!");
  }
}
