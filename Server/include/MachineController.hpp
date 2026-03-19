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

using signal_map_t = utl::SignalMap;

class MachineController {
 public:
  struct IoOps {
    std::function<std::optional<signal_map_t>()> readInputs;
    std::function<void(const signal_map_t&)> setOutputs;
    std::function<std::optional<signal_map_t>()> readOutputs;
    std::function<MachineComponent::State()> contecState;
  };

  struct MotorOps {
    std::function<void(utl::EMotor, MotorControlMode)> setMode;
    std::function<void(utl::EMotor, std::int32_t)> setSpeed;
    std::function<void(utl::EMotor, std::int32_t)> setAcceleration;
    std::function<void(utl::EMotor, std::int32_t)> setDeceleration;
    std::function<void(utl::EMotor, std::int32_t)> setPosition;
    std::function<void(utl::EMotor, MotorControlDirection)> setDirection;
    std::function<void(utl::EMotor)> start;
    std::function<void(utl::EMotor)> stop;
    std::function<bool(utl::EMotor)> isConfigured;
  };

  MachineController(IoOps io,
                    MotorOps motorOps,
                    utl::RobotStatus& robotStatus,
                    std::unique_ptr<IRobotControlPolicy> controlPolicy);

  void runControlLoopTasks();
  void handleToolChangerCommand(const cmd::ToolChangerCommand& command) const;

 private:
  IoOps _io;
  MotorOps _motorOps;
  utl::RobotStatus& _robotStatus;
  std::unique_ptr<IRobotControlPolicy> _controlPolicy;
  std::map<utl::EMotor, bool> _missingMotorWarned;
};
