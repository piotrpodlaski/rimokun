#include "MachineStatusBuilder.hpp"

#include <array>
#include <exception>
#include <format>

#include <Logger.hpp>
#include <Motor.hpp>
#include <MotorControl.hpp>
#include <magic_enum/magic_enum.hpp>

void MachineStatusBuilder::updateAndPublish(
    utl::RobotStatus& status, const ComponentsMap& components,
    const SnapshotFn& readJoystickSnapshot, const ReadSignalsFn& readInputSignals,
    const ReadSignalsFn& readOutputSignals, const PublishFn& publish) const {
  const auto motorControlIt = components.find(utl::ERobotComponent::MotorControl);
  if (motorControlIt != components.end() && motorControlIt->second != nullptr) {
    if (auto* motorControl = dynamic_cast<MotorControl*>(motorControlIt->second);
        motorControl != nullptr) {
      const auto componentState = motorControlIt->second->state();
      bool hasAnyMotorWarningOrAlarm = false;
      const auto configuredMotorIds = motorControl->configuredMotorIds();
      for (const auto motorId : configuredMotorIds) {
        auto& motorStatus = status.motors[motorId];

        if (componentState == MachineComponent::State::Error) {
          motorStatus.state = utl::ELEDState::Error;
          motorStatus.flags[utl::EMotorStatusFlags::Warning] = utl::ELEDState::Off;
          motorStatus.flags[utl::EMotorStatusFlags::Alarm] = utl::ELEDState::Error;
          motorStatus.warningDescription.clear();
          motorStatus.alarmDescription =
              "MotorControl component is in error state";
          hasAnyMotorWarningOrAlarm = true;
          continue;
        }

        if (!motorControl->motors().contains(motorId)) {
          motorStatus.state = utl::ELEDState::Error;
          motorStatus.flags[utl::EMotorStatusFlags::Warning] = utl::ELEDState::Off;
          motorStatus.flags[utl::EMotorStatusFlags::Alarm] = utl::ELEDState::Error;
          motorStatus.warningDescription.clear();
          motorStatus.alarmDescription =
              "Motor is configured but unavailable in runtime";
          hasAnyMotorWarningOrAlarm = true;
          continue;
        }

        try {
          const auto outputStatus = motorControl->readOutputStatus(motorId);
          const bool hasWarning = Motor::isDriverOutputFlagSet(
              outputStatus.raw, MotorOutputFlag::Warning);
          const bool hasAlarm = Motor::isDriverOutputFlagSet(outputStatus.raw,
                                                             MotorOutputFlag::Alarm);
          hasAnyMotorWarningOrAlarm = hasAnyMotorWarningOrAlarm || hasWarning || hasAlarm;

          motorStatus.flags[utl::EMotorStatusFlags::Warning] =
              hasWarning ? utl::ELEDState::Warning : utl::ELEDState::Off;
          motorStatus.flags[utl::EMotorStatusFlags::Alarm] =
              hasAlarm ? utl::ELEDState::Error : utl::ELEDState::Off;
          motorStatus.state = hasAlarm
                                  ? utl::ELEDState::Error
                                  : (hasWarning ? utl::ELEDState::Warning
                                                : utl::ELEDState::On);
          motorStatus.warningDescription.clear();
          motorStatus.alarmDescription.clear();

          if (hasWarning) {
            const auto warning = motorControl->diagnoseCurrentWarning(motorId);
            motorStatus.warningDescription =
                warning.cause.empty()
                    ? warning.type
                    : std::format("{}: {}", warning.type, warning.cause);
          }
          if (hasAlarm) {
            const auto alarm = motorControl->diagnoseCurrentAlarm(motorId);
            motorStatus.alarmDescription =
                alarm.cause.empty()
                    ? alarm.type
                    : std::format("{}: {}", alarm.type, alarm.cause);
          }
        } catch (const std::exception& ex) {
          SPDLOG_WARN("Failed to read status for motor {}: {}",
                      magic_enum::enum_name(motorId), ex.what());
          auto& motorStatus = status.motors[motorId];
          motorStatus.state = utl::ELEDState::Error;
          motorStatus.flags[utl::EMotorStatusFlags::Warning] = utl::ELEDState::Off;
          motorStatus.flags[utl::EMotorStatusFlags::Alarm] = utl::ELEDState::Error;
          motorStatus.warningDescription.clear();
          motorStatus.alarmDescription = ex.what();
          hasAnyMotorWarningOrAlarm = true;
        }
      }
      motorControl->setWarningState(hasAnyMotorWarningOrAlarm);
    }
  }

  for (const auto& [componentType, component] : components) {
    status.robotComponents[componentType] = stateToLed(component->state());
  }

  const auto contecIt = components.find(utl::ERobotComponent::Contec);
  const bool contecInError =
      contecIt != components.end() && contecIt->second != nullptr &&
      contecIt->second->state() == MachineComponent::State::Error;

  const auto joystick = readJoystickSnapshot();
  status.joystics[utl::EArm::Left] = {
      .x = joystick.x[0], .y = joystick.y[0], .btn = joystick.b[0]};
  status.joystics[utl::EArm::Right] = {
      .x = joystick.x[1], .y = joystick.y[1], .btn = joystick.b[1]};
  status.joystics[utl::EArm::Gantry] = {
      .x = joystick.x[2], .y = joystick.y[2], .btn = joystick.b[2]};

  const auto setAllToolChangerFlags = [&status](const utl::ELEDState ledState) {
    constexpr std::array flags = {
        utl::EToolChangerStatusFlags::ProxSen,
        utl::EToolChangerStatusFlags::OpenSen,
        utl::EToolChangerStatusFlags::ClosedSen,
        utl::EToolChangerStatusFlags::OpenValve,
        utl::EToolChangerStatusFlags::ClosedValve,
    };
    for (const auto arm : {utl::EArm::Left, utl::EArm::Right}) {
      for (const auto flag : flags) {
        status.toolChangers[arm].flags[flag] = ledState;
      }
    }
  };
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

  if (contecInError) {
    setAllToolChangerFlags(utl::ELEDState::Error);
    publish(status);
    return;
  }

  auto inputs = readInputSignals();
  auto outputs = readOutputSignals();

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
  switch (state) {
    case MachineComponent::State::Normal:
      return utl::ELEDState::On;
    case MachineComponent::State::Warning:
      return utl::ELEDState::Warning;
    case MachineComponent::State::Error:
      return utl::ELEDState::Error;
  }
  return utl::ELEDState::Error;
}
