#include "RobotStatusViewModel.hpp"

namespace {
utl::ELEDState toolChangerFlagOrOff(
    const utl::ToolChangerStatus& tc,
    const utl::EToolChangerStatusFlags flag) {
  const auto it = tc.flags.find(flag);
  if (it == tc.flags.end()) {
    return utl::ELEDState::Off;
  }
  return it->second;
}
}  // namespace

std::optional<ToolChangerViewModel> RobotStatusViewModel::toolChangerForArm(
    const utl::RobotStatus& status, const utl::EArm arm) {
  const auto it = status.toolChangers.find(arm);
  if (it == status.toolChangers.end()) {
    return std::nullopt;
  }
  const auto& tc = it->second;
  ToolChangerViewModel vm;
  vm.prox = toolChangerFlagOrOff(tc, utl::EToolChangerStatusFlags::ProxSen);
  vm.openSensor = toolChangerFlagOrOff(tc, utl::EToolChangerStatusFlags::OpenSen);
  vm.closedSensor =
      toolChangerFlagOrOff(tc, utl::EToolChangerStatusFlags::ClosedSen);
  vm.openValve =
      toolChangerFlagOrOff(tc, utl::EToolChangerStatusFlags::OpenValve);
  vm.closedValve =
      toolChangerFlagOrOff(tc, utl::EToolChangerStatusFlags::ClosedValve);
  vm.openButtonEnabled = (vm.openValve != utl::ELEDState::On);
  vm.closeButtonEnabled = (vm.closedValve != utl::ELEDState::On);
  return vm;
}

ResetControlsViewModel RobotStatusViewModel::resetControlsForStatus(
    const std::optional<utl::RobotStatus>& status) {
  ResetControlsViewModel vm;
  if (!status.has_value()) {
    return vm;
  }
  vm.server = utl::ELEDState::On;

  const auto getComponent = [&](const utl::ERobotComponent component) {
    const auto it = status->robotComponents.find(component);
    if (it == status->robotComponents.end()) {
      return utl::ELEDState::Off;
    }
    return it->second;
  };
  vm.contec = getComponent(utl::ERobotComponent::Contec);
  vm.motor = getComponent(utl::ERobotComponent::MotorControl);
  vm.controlPanel = getComponent(utl::ERobotComponent::ControlPanel);
  vm.resetContecEnabled = (vm.contec == utl::ELEDState::Error);
  vm.resetMotorEnabled = (vm.motor == utl::ELEDState::Error);
  vm.resetControlPanelEnabled = (vm.controlPanel == utl::ELEDState::Error);
  vm.enableAllMotorsEnabled =
      vm.motor == utl::ELEDState::On || vm.motor == utl::ELEDState::Warning;
  vm.disableAllMotorsEnabled =
      vm.motor == utl::ELEDState::On || vm.motor == utl::ELEDState::Warning;
  return vm;
}
