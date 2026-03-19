#pragma once

#include <functional>
#include <optional>

#include "CommonDefinitions.hpp"
#include "ControlPanel.hpp"
#include "MachineComponent.hpp"

class MachineStatusBuilder {
 public:
  MachineStatusBuilder();
  using ComponentsMap = std::map<utl::ERobotComponent, MachineComponent*>;
  using SnapshotFn = std::function<ControlPanel::Snapshot()>;
  using ReadSignalsFn = std::function<std::optional<utl::SignalMap>()>;
  using PublishFn = std::function<void(const utl::RobotStatus&)>;

  void updateAndPublish(utl::RobotStatus& status,
                        const ComponentsMap& components,
                        const SnapshotFn& readJoystickSnapshot,
                        const ReadSignalsFn& readInputSignals,
                        const ReadSignalsFn& readOutputSignals,
                        const PublishFn& publish) const;

 private:
  [[nodiscard]] double stepsPerMm(utl::EMotor motorId) const;
  [[nodiscard]] double positionMmFromSteps(utl::EMotor motorId,
                                           std::int32_t steps) const;
  [[nodiscard]] double speedMmPerSecFromRpm(utl::EMotor motorId,
                                            std::int32_t rpm) const;
  static utl::ELEDState stateToLed(MachineComponent::State state);

  double _stepsPerRevolution{1000.0};
  std::map<utl::EMotor, double> _stepsPerMmByMotor;
};
