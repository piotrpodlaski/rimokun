#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "CommandInterface.hpp"
#include "CommonDefinitions.hpp"
#include "MachineComponent.hpp"
#include "RobotControlPolicy.hpp"

using signal_map_t = IRobotControlPolicy::SignalMap;

class MachineController {
 public:
  using ReadSignalsFn = std::function<std::optional<signal_map_t>()>;
  using SetOutputsFn = std::function<void(const signal_map_t&)>;
  using ComponentStateFn = std::function<MachineComponent::State()>;

  MachineController(ReadSignalsFn readInputs,
                    SetOutputsFn setOutputs,
                    ReadSignalsFn readOutputs,
                    ComponentStateFn contecState,
                    utl::RobotStatus& robotStatus,
                    std::unique_ptr<IRobotControlPolicy> controlPolicy);

  void runControlLoopTasks() const;
  void handleToolChangerCommand(const cmd::ToolChangerCommand& command) const;

 private:
  ReadSignalsFn _readInputs;
  SetOutputsFn _setOutputs;
  ReadSignalsFn _readOutputs;
  ComponentStateFn _contecState;
  utl::RobotStatus& _robotStatus;
  std::unique_ptr<IRobotControlPolicy> _controlPolicy;
};
