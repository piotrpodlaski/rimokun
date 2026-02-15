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

  void pulseStart(utl::EMotor motorId);
  void pulseStop(utl::EMotor motorId);
  void pulseHome(utl::EMotor motorId);
  void setForward(utl::EMotor motorId, bool enabled);
  void setReverse(utl::EMotor motorId, bool enabled);
  void setJogPlus(utl::EMotor motorId, bool enabled);
  void setJogMinus(utl::EMotor motorId, bool enabled);
  [[nodiscard]] MotorFlagStatus readInputStatus(utl::EMotor motorId);
  [[nodiscard]] MotorFlagStatus readOutputStatus(utl::EMotor motorId);
  [[nodiscard]] MotorDirectIoStatus readDirectIoStatus(utl::EMotor motorId);
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
