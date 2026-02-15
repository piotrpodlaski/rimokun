#pragma once

#include <MachineComponent.hpp>
#include <Motor.hpp>

#include <map>
#include <mutex>
#include <optional>

class MotorControl final : public MachineComponent {
 public:
  MotorControl();
  void initialize() override;
  void reset() override;
  [[nodiscard]] utl::ERobotComponent componentType() const override {
    return utl::ERobotComponent::MotorControl;
  }

  [[nodiscard]] const std::map<utl::EMotor, Motor>& motors() const {
    return _motors;
  }
  [[nodiscard]] MotorCodeDiagnostic diagnoseCurrentAlarm(utl::EMotor motorId);
  [[nodiscard]] MotorCodeDiagnostic diagnoseCurrentWarning(utl::EMotor motorId);
  [[nodiscard]] MotorCodeDiagnostic diagnoseCurrentCommunicationError(
      utl::EMotor motorId);

 private:
  MotorRtuConfig _rtuConfig;
  MotorRegisterMap _registerMap;
  std::map<utl::EMotor, int> _motorAddresses;
  std::map<utl::EMotor, Motor> _motors;
  std::optional<ModbusClient> _bus;
  std::mutex _busMutex;
};
