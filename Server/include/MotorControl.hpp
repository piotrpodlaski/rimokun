#pragma once

#include <MachineComponent.hpp>
#include <Motor.hpp>

#include <map>
#include <mutex>
#include <optional>

enum class MotorControlMode {
  Speed,
  Position,
};

enum class MotorControlDirection {
  Forward,
  Reverse,
};

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

  void setMode(utl::EMotor motorId, MotorControlMode mode);
  void setSpeed(utl::EMotor motorId, std::int32_t speed);
  void setPosition(utl::EMotor motorId, std::int32_t position);
  void setDirection(utl::EMotor motorId, MotorControlDirection direction);
  void startMovement(utl::EMotor motorId);
  void stopMovement(utl::EMotor motorId);

  void pulseStart(utl::EMotor motorId);
  void pulseStop(utl::EMotor motorId);
  void pulseHome(utl::EMotor motorId);
  void setForward(utl::EMotor motorId, bool enabled);
  void setReverse(utl::EMotor motorId, bool enabled);
  void setJogPlus(utl::EMotor motorId, bool enabled);
  void setJogMinus(utl::EMotor motorId, bool enabled);
  [[nodiscard]] std::uint8_t readSelectedOperationId(utl::EMotor motorId);
  void setSelectedOperationId(utl::EMotor motorId, std::uint8_t opId);
  void setOperationMode(utl::EMotor motorId, std::uint8_t opId,
                        MotorOperationMode mode);
  void setOperationFunction(utl::EMotor motorId, std::uint8_t opId,
                            MotorOperationFunction function);
  void setOperationPosition(utl::EMotor motorId, std::uint8_t opId,
                            std::int32_t position);
  void setOperationSpeed(utl::EMotor motorId, std::uint8_t opId,
                         std::int32_t speed);
  void setOperationAcceleration(utl::EMotor motorId, std::uint8_t opId,
                                std::int32_t acceleration);
  void setOperationDeceleration(utl::EMotor motorId, std::uint8_t opId,
                                std::int32_t deceleration);
  void configureConstantSpeedPair(utl::EMotor motorId, std::int32_t speedOp0,
                                  std::int32_t speedOp1, std::int32_t acceleration,
                                  std::int32_t deceleration);
  void updateConstantSpeedBuffered(utl::EMotor motorId, std::int32_t speed);
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

  struct MotorRuntimeState {
    MotorControlMode mode{MotorControlMode::Speed};
    MotorControlDirection direction{MotorControlDirection::Forward};
    std::int32_t speed{1000};
    std::int32_t position{0};
    std::int32_t acceleration{0x5fff};
    std::int32_t deceleration{0x5fff};
    bool speedPairPrepared{false};
    bool positionPrepared{false};
  };
  std::map<utl::EMotor, MotorRuntimeState> _runtime;

  std::optional<ModbusClient> _bus;
  std::mutex _busMutex;
};
