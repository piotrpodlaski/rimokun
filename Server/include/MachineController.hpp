#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "CommandInterface.hpp"
#include "CommonDefinitions.hpp"
#include "MachineComponent.hpp"
#include "MotorControl.hpp"
#include "RobotControlPolicy.hpp"

using signal_map_t = IRobotControlPolicy::SignalMap;

class MachineController {
 public:
  using ReadSignalsFn = std::function<std::optional<signal_map_t>()>;
  using SetOutputsFn = std::function<void(const signal_map_t&)>;
  using ComponentStateFn = std::function<MachineComponent::State()>;
  using SetMotorModeFn = std::function<void(utl::EMotor, MotorControlMode)>;
  using SetMotorSpeedFn = std::function<void(utl::EMotor, std::int32_t)>;
  using SetMotorAccelerationFn = std::function<void(utl::EMotor, std::int32_t)>;
  using SetMotorDecelerationFn = std::function<void(utl::EMotor, std::int32_t)>;
  using SetMotorPositionFn = std::function<void(utl::EMotor, std::int32_t)>;
  using SetMotorDirectionFn =
      std::function<void(utl::EMotor, MotorControlDirection)>;
  using MoveMotorFn = std::function<void(utl::EMotor)>;
  using IsMotorConfiguredFn = std::function<bool(utl::EMotor)>;

  MachineController(ReadSignalsFn readInputs,
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
                    std::unique_ptr<IRobotControlPolicy> controlPolicy);

  void runControlLoopTasks() const;
  void handleToolChangerCommand(const cmd::ToolChangerCommand& command) const;

 private:
  ReadSignalsFn _readInputs;
  SetOutputsFn _setOutputs;
  ReadSignalsFn _readOutputs;
  ComponentStateFn _contecState;
  SetMotorModeFn _setMotorMode;
  SetMotorSpeedFn _setMotorSpeed;
  SetMotorAccelerationFn _setMotorAcceleration;
  SetMotorDecelerationFn _setMotorDeceleration;
  SetMotorPositionFn _setMotorPosition;
  SetMotorDirectionFn _setMotorDirection;
  MoveMotorFn _startMotor;
  MoveMotorFn _stopMotor;
  IsMotorConfiguredFn _isMotorConfigured;
  utl::RobotStatus& _robotStatus;
  std::unique_ptr<IRobotControlPolicy> _controlPolicy;
  mutable std::map<utl::EMotor, bool> _missingMotorWarned;
};
