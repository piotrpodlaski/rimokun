#include "MachineRuntime.hpp"

void MachineRuntime::wireMachine(Machine& machine) {
  if (!machine._loopRunner) {
    machine._loopRunner = std::make_unique<ControlLoopRunner>(
        *machine._clock, machine._loopInterval, machine._updateInterval);
  }
  if (!machine._controller) {
    machine._controller = std::make_unique<MachineController>(
        [&machine]() { return machine.readInputSignals(); },
        [&machine](const signal_map_t& outputs) { machine.setOutputs(outputs); },
        [&machine]() { return machine.readOutputSignals(); },
        [&machine]() { return machine._contec.state(); },
        [&machine](const utl::EMotor motorId, const MotorControlMode mode) {
          machine._motorControl.setMode(motorId, mode);
        },
        [&machine](const utl::EMotor motorId, const std::int32_t speed) {
          machine._motorControl.setSpeed(motorId, speed);
        },
        [&machine](const utl::EMotor motorId, const std::int32_t position) {
          machine._motorControl.setPosition(motorId, position);
        },
        [&machine](const utl::EMotor motorId, const MotorControlDirection direction) {
          machine._motorControl.setDirection(motorId, direction);
        },
        [&machine](const utl::EMotor motorId) {
          machine._motorControl.startMovement(motorId);
        },
        [&machine](const utl::EMotor motorId) {
          machine._motorControl.stopMovement(motorId);
        },
        [&machine](const utl::EMotor motorId) {
          return machine._motorControl.motors().contains(motorId);
        },
        machine._robotStatus, std::make_unique<RimoKunControlPolicy>());
  }
  if (!machine._componentService) {
    machine._componentService =
        std::make_unique<MachineComponentService>(machine._components);
  }
  if (!machine._statusBuilder) {
    machine._statusBuilder = std::make_unique<MachineStatusBuilder>();
  }
  if (!machine._commandProcessor) {
    machine._commandProcessor = std::make_unique<MachineCommandProcessor>();
  }
  if (!machine._commandServer) {
    machine._commandServer = std::make_unique<MachineCommandServer>(
        *machine._commandProcessor, machine._robotServer);
  }
}

void MachineRuntime::initialize() {
  wireMachine(_machine);
  _machine.initialize();
}

void MachineRuntime::shutdown() { _machine.shutdown(); }
