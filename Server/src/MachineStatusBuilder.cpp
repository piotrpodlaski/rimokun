#include "MachineStatusBuilder.hpp"

void MachineStatusBuilder::updateAndPublish(
    utl::RobotStatus& status, const ComponentsMap& components,
    const SnapshotFn& readJoystickSnapshot, const ReadSignalsFn& readInputSignals,
    const ReadSignalsFn& readOutputSignals, const PublishFn& publish) const {
  for (const auto& [componentType, component] : components) {
    status.robotComponents[componentType] = stateToLed(component->state());
  }

  const auto joystick = readJoystickSnapshot();
  status.joystics[utl::EArm::Left] = {
      .x = joystick.x[0], .y = joystick.y[0], .btn = joystick.b[0]};
  status.joystics[utl::EArm::Right] = {
      .x = joystick.x[1], .y = joystick.y[1], .btn = joystick.b[1]};
  status.joystics[utl::EArm::Gantry] = {
      .x = joystick.x[2], .y = joystick.y[2], .btn = joystick.b[2]};

  auto inputs = readInputSignals();
  auto outputs = readOutputSignals();
  const auto setProxUnknown = [&status]() {
    status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
        utl::ELEDState::Error;
    status.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::Error;
  };
  const auto setValveUnknown = [&status]() {
    status.toolChangers[utl::EArm::Left]
        .flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::Error;
    status.toolChangers[utl::EArm::Left]
        .flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::Error;
    status.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::Error;
    status.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::Error;
  };

  if (inputs && inputs->contains("button1") && inputs->contains("button2")) {
    status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
        inputs->at("button1") ? utl::ELEDState::On : utl::ELEDState::Off;
    status.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::ProxSen] =
        inputs->at("button2") ? utl::ELEDState::On : utl::ELEDState::Off;
  } else {
    setProxUnknown();
  }

  if (outputs && outputs->contains("toolChangerLeft") &&
      outputs->contains("toolChangerRight")) {
    const auto tclStatus = outputs->at("toolChangerLeft");
    const auto tcrStatus = outputs->at("toolChangerRight");
    status.toolChangers[utl::EArm::Left]
        .flags[utl::EToolChangerStatusFlags::ClosedValve] =
        tclStatus ? utl::ELEDState::Off : utl::ELEDState::On;
    status.toolChangers[utl::EArm::Left]
        .flags[utl::EToolChangerStatusFlags::OpenValve] =
        tclStatus ? utl::ELEDState::On : utl::ELEDState::Off;
    status.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::ClosedValve] =
        tcrStatus ? utl::ELEDState::Off : utl::ELEDState::On;
    status.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::OpenValve] =
        tcrStatus ? utl::ELEDState::On : utl::ELEDState::Off;
  } else {
    setValveUnknown();
  }

  publish(status);
}

utl::ELEDState MachineStatusBuilder::stateToLed(MachineComponent::State state) {
  return state == MachineComponent::State::Normal ? utl::ELEDState::On
                                                   : utl::ELEDState::Error;
}
