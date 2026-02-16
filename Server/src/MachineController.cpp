#include "MachineController.hpp"

#include <Logger.hpp>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>

MachineController::MachineController(ReadSignalsFn readInputs,
                                     SetOutputsFn setOutputs,
                                     ReadSignalsFn readOutputs,
                                     ComponentStateFn contecState,
                                     utl::RobotStatus& robotStatus,
                                     std::unique_ptr<IRobotControlPolicy> controlPolicy)
    : _readInputs(std::move(readInputs)),
      _setOutputs(std::move(setOutputs)),
      _readOutputs(std::move(readOutputs)),
      _contecState(std::move(contecState)),
      _robotStatus(robotStatus),
      _controlPolicy(std::move(controlPolicy)) {
  if (!_controlPolicy) {
    throw std::runtime_error("MachineController requires a non-null control policy.");
  }
}

void MachineController::runControlLoopTasks() const {
  const auto decision =
      _controlPolicy->decide(_readInputs(), _contecState(), _robotStatus);
  if (decision.setToolChangerErrorBlinking) {
    for (auto& [_, tc] : _robotStatus.toolChangers) {
      for (auto& [__, flag] : tc.flags) {
        flag = utl::ELEDState::ErrorBlinking;
      }
    }
    return;
  }
  if (decision.outputs) {
    _setOutputs(*decision.outputs);
  }
}

void MachineController::handleToolChangerCommand(
    const cmd::ToolChangerCommand& command) const {
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
