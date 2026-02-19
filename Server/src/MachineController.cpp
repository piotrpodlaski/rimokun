#include "MachineController.hpp"

#include <Logger.hpp>
#include <TimingMetrics.hpp>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>

MachineController::MachineController(ReadSignalsFn readInputs,
                                     SetOutputsFn setOutputs,
                                     ReadSignalsFn readOutputs,
                                     ComponentStateFn contecState,
                                     SetMotorModeFn setMotorMode,
                                     SetMotorSpeedFn setMotorSpeed,
                                     SetMotorAccelerationFn setMotorAcceleration,
                                     SetMotorDecelerationFn setMotorDeceleration,
                                     SetMotorPositionFn setMotorPosition,
                                     SetMotorDirectionFn setMotorDirection,
                                     MoveMotorFn startMotor,
                                     MoveMotorFn stopMotor,
                                     IsMotorConfiguredFn isMotorConfigured,
                                     utl::RobotStatus& robotStatus,
                                     std::unique_ptr<IRobotControlPolicy> controlPolicy)
    : _readInputs(std::move(readInputs)),
      _setOutputs(std::move(setOutputs)),
      _readOutputs(std::move(readOutputs)),
      _contecState(std::move(contecState)),
      _setMotorMode(std::move(setMotorMode)),
      _setMotorSpeed(std::move(setMotorSpeed)),
      _setMotorAcceleration(std::move(setMotorAcceleration)),
      _setMotorDeceleration(std::move(setMotorDeceleration)),
      _setMotorPosition(std::move(setMotorPosition)),
      _setMotorDirection(std::move(setMotorDirection)),
      _startMotor(std::move(startMotor)),
      _stopMotor(std::move(stopMotor)),
      _isMotorConfigured(std::move(isMotorConfigured)),
      _robotStatus(robotStatus),
      _controlPolicy(std::move(controlPolicy)) {
  if (!_controlPolicy) {
    throw std::runtime_error("MachineController requires a non-null control policy.");
  }
  if (!_isMotorConfigured) {
    throw std::runtime_error("MachineController requires motor availability callback.");
  }
}

void MachineController::runControlLoopTasks() const {
  RIMO_TIMED_SCOPE("MachineController::runControlLoopTasks");
  const auto decision =
      _controlPolicy->decide(_readInputs(), _readOutputs(), _contecState(),
                             _robotStatus);
  if (decision.setToolChangerErrorBlinking) {
    return;
  }
  if (decision.outputs) {
    _setOutputs(*decision.outputs);
  }
  for (const auto& intent : decision.motorIntents) {
    if (!_isMotorConfigured(intent.motorId)) {
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
    if (intent.mode) {
      _setMotorMode(intent.motorId, *intent.mode);
    }
    if (intent.direction) {
      _setMotorDirection(intent.motorId, *intent.direction);
    }
    if (intent.speed) {
      _setMotorSpeed(intent.motorId, *intent.speed);
    }
    if (intent.acceleration) {
      _setMotorAcceleration(intent.motorId, *intent.acceleration);
    }
    if (intent.deceleration) {
      _setMotorDeceleration(intent.motorId, *intent.deceleration);
    }
    if (intent.position) {
      _setMotorPosition(intent.motorId, *intent.position);
    }
    if (intent.stopMovement) {
      _stopMotor(intent.motorId);
      continue;
    }
    if (intent.startMovement) {
      _startMotor(intent.motorId);
    }
  }
}

void MachineController::handleToolChangerCommand(
    const cmd::ToolChangerCommand& command) const {
  RIMO_TIMED_SCOPE("MachineController::handleToolChangerCommand");
  SPDLOG_INFO("Changing status of '{}' tool changer to '{}'",
              magic_enum::enum_name(command.arm),
              magic_enum::enum_name(command.action));
  if (_contecState() == MachineComponent::State::Error) {
    throw std::runtime_error(
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
  _setOutputs(outputSignals);
  if (!_readOutputs()) {
    throw std::runtime_error(
        "Unable to read status of output signals, tool changer status update "
        "failed!");
  }
}
