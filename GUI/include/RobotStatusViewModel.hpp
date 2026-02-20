#pragma once

#include <optional>

#include "CommonDefinitions.hpp"

struct ToolChangerViewModel {
  utl::ELEDState prox{utl::ELEDState::Off};
  utl::ELEDState openSensor{utl::ELEDState::Off};
  utl::ELEDState closedSensor{utl::ELEDState::Off};
  utl::ELEDState openValve{utl::ELEDState::Off};
  utl::ELEDState closedValve{utl::ELEDState::Off};
  bool openButtonEnabled{true};
  bool closeButtonEnabled{true};
};

struct ResetControlsViewModel {
  utl::ELEDState server{utl::ELEDState::Error};
  utl::ELEDState contec{utl::ELEDState::Off};
  utl::ELEDState motor{utl::ELEDState::Off};
  utl::ELEDState controlPanel{utl::ELEDState::Off};
  bool resetContecEnabled{false};
  bool resetMotorEnabled{false};
  bool resetControlPanelEnabled{false};
  bool enableAllMotorsEnabled{false};
  bool disableAllMotorsEnabled{false};
};

class RobotStatusViewModel {
 public:
  static std::optional<ToolChangerViewModel> toolChangerForArm(
      const utl::RobotStatus& status, utl::EArm arm);

  static ResetControlsViewModel resetControlsForStatus(
      const std::optional<utl::RobotStatus>& status);
};
