#pragma once

#include <CommonDefinitions.hpp>
#include <ModbusClient.hpp>
#include <MotorRegisterMap.hpp>

#include <optional>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

struct MotorRtuConfig {
  std::string device;
  int baud{9600};
  char parity{'N'};
  int dataBits{8};
  int stopBits{1};
  unsigned responseTimeoutMS{1000};
};

enum class MotorDiagnosticDomain {
  Alarm,
  Warning,
  CommunicationError,
};

struct MotorCodeDiagnostic {
  MotorDiagnosticDomain domain;
  std::uint8_t code;
  bool known{false};
  std::string type;
  std::string cause;
  std::string remedialAction;
};

enum class MotorInputFlag : std::uint16_t {
  M0 = 1u << 0,
  M1 = 1u << 1,
  M2 = 1u << 2,
  Start = 1u << 3,
  Home = 1u << 4,
  Stop = 1u << 5,
  Free = 1u << 6,
  Ms0 = 1u << 8,
  Ms1 = 1u << 9,
  Ms2 = 1u << 10,
  SStart = 1u << 11,
  PlusJog = 1u << 12,
  MinusJog = 1u << 13,
  Fwd = 1u << 14,
  Rvs = 1u << 15,
};

enum class MotorOutputFlag : std::uint16_t {
  M0R = 1u << 0,
  M1R = 1u << 1,
  M2R = 1u << 2,
  StartR = 1u << 3,
  HomeP = 1u << 4,
  Ready = 1u << 5,
  Warning = 1u << 6,
  Alarm = 1u << 7,
  SBusy = 1u << 8,
  Area1 = 1u << 9,
  Area2 = 1u << 10,
  Area3 = 1u << 11,
  Tim = 1u << 12,
  Move = 1u << 13,
  End = 1u << 14,
  Tlc = 1u << 15,
};

struct MotorFlagStatus {
  std::uint16_t raw{0};
  std::vector<std::string_view> activeFlags;
};

struct MotorDirectIoStatus {
  std::uint16_t reg00D4{0};
  std::uint16_t reg00D5{0};
  std::vector<std::string_view> activeFlags;
};

enum class MotorOperationMode : std::int32_t {
  Incremental = 0,
  Absolute = 1,
};

enum class MotorOperationFunction : std::int32_t {
  SingleMotion = 0,
  LinkedMotion = 1,
  LinkedMotion2 = 2,
  Push = 3,
};

class Motor {
 public:
  Motor(utl::EMotor id, int slaveAddress, MotorRegisterMap map);

  [[nodiscard]] utl::EMotor id() const { return _id; }
  [[nodiscard]] int slaveAddress() const { return _slaveAddress; }

  void initialize(ModbusClient& bus) const;
  [[nodiscard]] std::uint32_t readU32(ModbusClient& bus, int upperAddr) const;
  void writeInt32(ModbusClient& bus, int upperAddr, std::int32_t value) const;
  [[nodiscard]] std::uint16_t readU16(ModbusClient& bus, int addr) const;
  void writeU16(ModbusClient& bus, int addr, std::uint16_t value) const;
  [[nodiscard]] std::uint8_t readAlarmCode(ModbusClient& bus) const;
  [[nodiscard]] std::uint8_t readWarningCode(ModbusClient& bus) const;
  [[nodiscard]] std::uint8_t readCommunicationErrorCode(ModbusClient& bus) const;
  [[nodiscard]] MotorCodeDiagnostic diagnoseAlarm(std::uint8_t code) const;
  [[nodiscard]] MotorCodeDiagnostic diagnoseWarning(std::uint8_t code) const;
  [[nodiscard]] MotorCodeDiagnostic diagnoseCommunicationError(
      std::uint8_t code) const;

  [[nodiscard]] std::uint16_t readDriverInputCommandRaw(ModbusClient& bus) const;
  [[nodiscard]] std::uint16_t readDriverOutputStatusRaw(ModbusClient& bus) const;
  [[nodiscard]] std::uint32_t readDirectIoAndBrakeStatusRaw(
      ModbusClient& bus) const;
  // AR-KD2 motion command hooks via driver input command (007Dh).
  // START/HOME/STOP are exposed as pulses; direction and JOG as level signals.
  void writeDriverInputCommandRaw(ModbusClient& bus, std::uint16_t raw) const;
  void setDriverInputFlag(ModbusClient& bus, MotorInputFlag flag,
                          bool enabled) const;
  [[nodiscard]] static bool isDriverOutputFlagSet(std::uint16_t raw,
                                                  MotorOutputFlag flag);
  void pulseDriverInputFlag(
      ModbusClient& bus, MotorInputFlag flag,
      std::chrono::milliseconds hold = std::chrono::milliseconds{30}) const;
  void pulseStart(ModbusClient& bus) const;
  void pulseStop(ModbusClient& bus) const;
  void pulseHome(ModbusClient& bus) const;
  void setForward(ModbusClient& bus, bool enabled) const;
  void setReverse(ModbusClient& bus, bool enabled) const;
  void setJogPlus(ModbusClient& bus, bool enabled) const;
  void setJogMinus(ModbusClient& bus, bool enabled) const;
  [[nodiscard]] static std::uint8_t decodeOperationIdFromInputRaw(
      std::uint16_t raw);
  [[nodiscard]] std::uint8_t readSelectedOperationId(ModbusClient& bus) const;
  void setSelectedOperationId(ModbusClient& bus, std::uint8_t opId) const;
  void setOperationMode(ModbusClient& bus, std::uint8_t opId,
                        MotorOperationMode mode) const;
  void setOperationFunction(ModbusClient& bus, std::uint8_t opId,
                            MotorOperationFunction function) const;
  void setOperationPosition(ModbusClient& bus, std::uint8_t opId,
                            std::int32_t position) const;
  void setOperationSpeed(ModbusClient& bus, std::uint8_t opId,
                         std::int32_t speed) const;
  void setOperationAcceleration(ModbusClient& bus, std::uint8_t opId,
                                std::int32_t acceleration) const;
  void setOperationDeceleration(ModbusClient& bus, std::uint8_t opId,
                                std::int32_t deceleration) const;
  void configureConstantSpeedPair(ModbusClient& bus, std::int32_t speedOp0,
                                  std::int32_t speedOp1, std::int32_t acceleration,
                                  std::int32_t deceleration) const;
  // Double-buffer update for constant-speed operation (op0/op1 swap).
  // Writes speed to inactive operation and switches selected opId.
  void updateConstantSpeedBuffered(ModbusClient& bus, std::int32_t speed) const;
  [[nodiscard]] MotorFlagStatus decodeDriverInputStatus(std::uint16_t raw) const;
  [[nodiscard]] MotorFlagStatus decodeDriverOutputStatus(std::uint16_t raw) const;
  [[nodiscard]] MotorDirectIoStatus decodeDirectIoAndBrakeStatus(
      std::uint32_t raw) const;
  void resetAlarm(ModbusClient& bus) const;

  [[nodiscard]] const MotorRegisterMap& map() const { return _map; }

 private:
  void selectSlave(ModbusClient& bus) const;

  utl::EMotor _id;
  int _slaveAddress;
  MotorRegisterMap _map;
};
