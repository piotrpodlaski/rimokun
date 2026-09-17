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
  const bool operationSuspended = status.cpuOverride.value_or(false);
  vm.openButtonEnabled =
      !operationSuspended && (vm.openValve != utl::ELEDState::On);
  vm.closeButtonEnabled =
      !operationSuspended && (vm.closedValve != utl::ELEDState::On);
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
  const bool operationSuspended = status->cpuOverride.value_or(false);
  if (status->motorPower.has_value()) {
    vm.motorPower =
        *status->motorPower ? utl::ELEDState::On : utl::ELEDState::Off;
  } else {
    vm.motorPower = utl::ELEDState::Off;
  }
  if (status->safetyOn.has_value()) {
    if (*status->safetyOn) {
      vm.safetyOn = utl::ELEDState::On;
    } else if (status->heartBeatRunning.value_or(false)) {
      vm.safetyOn = utl::ELEDState::Warning;
    } else {
      vm.safetyOn = utl::ELEDState::Error;
    }
  } else {
    vm.safetyOn = utl::ELEDState::Off;
  }
  if (operationSuspended) {
    vm.safetyOn = utl::ELEDState::ErrorBlinking;
    vm.warningMessage = utl::kCpuOverrideWarningMessage;
  }
  vm.resetContecEnabled = (vm.contec == utl::ELEDState::Error);
  vm.resetSafetyEnabled =
      !operationSuspended && (vm.safetyOn != utl::ELEDState::On);
  vm.resetMotorEnabled =
      !operationSuspended && (vm.motor == utl::ELEDState::Error);
  vm.resetControlPanelEnabled = (vm.controlPanel == utl::ELEDState::Error);
  vm.enableAllMotorsEnabled =
      !operationSuspended &&
      (vm.motor == utl::ELEDState::On || vm.motor == utl::ELEDState::Warning);
  vm.disableAllMotorsEnabled =
      !operationSuspended &&
      (vm.motor == utl::ELEDState::On || vm.motor == utl::ELEDState::Warning);
  vm.homingEnabled =
      !operationSuspended &&
      (vm.motor == utl::ELEDState::On || vm.motor == utl::ELEDState::Warning);
  return vm;
}
