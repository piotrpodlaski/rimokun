#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>

#include "CommonDefinitions.hpp"
#include "ControlPanel.hpp"
#include "MachineComponent.hpp"

using signal_map_t = std::map<std::string, bool>;

class MachineStatusBuilder {
 public:
  using ComponentsMap = std::map<utl::ERobotComponent, MachineComponent*>;
  using SnapshotFn = std::function<ControlPanel::Snapshot()>;
  using ReadSignalsFn = std::function<std::optional<signal_map_t>()>;
  using PublishFn = std::function<void(const utl::RobotStatus&)>;

  void updateAndPublish(utl::RobotStatus& status,
                        const ComponentsMap& components,
                        const SnapshotFn& readJoystickSnapshot,
                        const ReadSignalsFn& readInputSignals,
                        const ReadSignalsFn& readOutputSignals,
                        const PublishFn& publish) const;

 private:
  static utl::ELEDState stateToLed(MachineComponent::State state);
};
