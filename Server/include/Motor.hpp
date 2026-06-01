#pragma once

#include <CommonDefinitions.hpp>
#include <ModbusClient.hpp>
#include <MotorRegisterMap.hpp>

#include <optional>
#include <array>
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
  unsigned connectTimeoutMS{1000};
  unsigned responseTimeoutMS{1000};
  unsigned interRequestDelayMS{0};
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
  struct AssignedSignal {
    std::string channel;
    std::string function;
    std::uint16_t functionCode{0};
    bool active{false};
  };

  std::uint16_t reg00D4{0};
  std::uint16_t reg00D5{0};
  std::vector<std::string> activeFlags;
  std::vector<AssignedSignal> outputAssignments;
  std::vector<AssignedSignal> inputAssignments;
};

struct MotorRemoteIoStatus {
  struct AssignedSignal {
    std::string channel;
    std::string function;
    std::uint16_t functionCode{0};
    bool active{false};
  };

  std::uint16_t reg007D{0};
  std::uint16_t reg007F{0};
  std::vector<std::string> activeFlags;
  std::vector<AssignedSignal> outputAssignments;
  std::vector<AssignedSignal> inputAssignments;
};

struct MotorIoAssignmentDefault {
  std::string_view channel;
  std::string_view function;
  std::uint16_t functionCode{0};
};

namespace motor_io_defaults {
inline constexpr std::array<MotorIoAssignmentDefault, 7> ioOutput{{
    {"OUT0", "HOME-P", 70},
    {"OUT1", "END", 69},
    {"OUT2", "AREA1", 73},
    {"OUT3", "READY", 67},
    {"OUT4", "WNG", 66},
    {"OUT5", "ALM", 65},
    {"MB", "MB", 0},
}};

inline constexpr std::array<MotorIoAssignmentDefault, 12> ioInput{{
    {"IN0", "HOME", 3},
    {"IN1", "START", 4},
    {"IN2", "M0", 48},
    {"IN3", "M1", 49},
    {"IN4", "M2", 50},
    {"IN5", "FREE", 16},
    {"IN6", "STOP", 18},
    {"IN7", "ALM-RST", 24},
    {"+LS", "+LS", 0},
    {"-LS", "-LS", 0},
    {"HOMES", "HOMES", 0},
    {"SLIT", "SLIT", 0},
}};

inline constexpr std::array<MotorIoAssignmentDefault, 16> netInput{{
    {"NET-IN0", "M0", 48},
    {"NET-IN1", "M1", 49},
    {"NET-IN2", "M2", 50},
    {"NET-IN3", "START", 4},
    {"NET-IN4", "HOME", 3},
    {"NET-IN5", "STOP", 18},
    {"NET-IN6", "FREE", 16},
    {"NET-IN7", "not used", 0},
    {"NET-IN8", "MS0", 8},
    {"NET-IN9", "MS1", 9},
    {"NET-IN10", "MS2", 10},
    {"NET-IN11", "SSTART", 5},
    {"NET-IN12", "+JOG", 6},
    {"NET-IN13", "-JOG", 7},
    {"NET-IN14", "FWD", 1},
    {"NET-IN15", "RVS", 2},
}};

inline constexpr std::array<MotorIoAssignmentDefault, 16> netOutput{{
    {"NET-OUT0", "M0_R", 48},
    {"NET-OUT1", "M1_R", 49},
    {"NET-OUT2", "M2_R", 50},
    {"NET-OUT3", "START_R", 4},
    {"NET-OUT4", "HOME-P", 70},
    {"NET-OUT5", "READY", 67},
    {"NET-OUT6", "WNG", 66},
    {"NET-OUT7", "ALM", 65},
    {"NET-OUT8", "S-BSY", 80},
    {"NET-OUT9", "AREA1", 73},
    {"NET-OUT10", "AREA2", 74},
    {"NET-OUT11", "AREA3", 75},
    {"NET-OUT12", "TIM", 72},
    {"NET-OUT13", "MOVE", 68},
    {"NET-OUT14", "END", 69},
    {"NET-OUT15", "TLC", 71},
}};
}  // namespace motor_io_defaults

struct MotorMonitorSnapshot {
  std::int32_t commandPosition{0};
  std::int32_t actualSpeed{0};
  std::int32_t actualPosition{0};
  std::uint16_t reg00D4{0};
  std::uint16_t reg00D5{0};
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
  Motor(utl::EMotor id, int slaveAddress, MotorRegisterMap map,
        int commandSlaveAddress = -1,
        bool forceFunction10ForSingleRegisterWrites = false);

  [[nodiscard]] utl::EMotor id() const { return _id; }
  [[nodiscard]] int slaveAddress() const { return _slaveAddress; }
  [[nodiscard]] int commandSlaveAddress() const { return _commandSlaveAddress; }

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
  [[nodiscard]] MotorMonitorSnapshot readMonitorSnapshot(ModbusClient& bus) const;
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
  void setEnabled(ModbusClient& bus, bool enabled) const;
  void invalidateDriverInputCommandCache() const noexcept;
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
  void setRunCurrent(ModbusClient& bus, std::int32_t current) const;
  void setStopCurrent(ModbusClient& bus, std::int32_t current) const;
  void setStopInputAction(ModbusClient& bus, std::int32_t value) const;
  void setStartingSpeed(ModbusClient& bus, std::int32_t speed) const;
  void configureHoming(ModbusClient& bus, std::int32_t mode,
                       std::int32_t operatingSpeed,
                       std::int32_t acceleration,
                       std::int32_t startingSpeed,
                       std::int32_t startingDirection) const;
  void setOverloadAlarm(ModbusClient& bus, std::int32_t value) const;
  void setExcessivePositionDeviationAlarm(ModbusClient& bus,
                                          std::int32_t value) const;
  void setOverloadWarning(ModbusClient& bus, std::int32_t value) const;
  void setExcessivePositionDeviationWarning(ModbusClient& bus,
                                            std::int32_t value) const;
  void setMotorRotationDirection(ModbusClient& bus, std::int32_t value) const;
  void executeConfiguration(ModbusClient& bus) const;
  [[nodiscard]] std::int32_t readGroupId(ModbusClient& bus) const;
  void setGroupId(ModbusClient& bus, std::int32_t value) const;
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
  [[nodiscard]] MotorRemoteIoStatus decodeRemoteIoStatus(
      std::uint16_t reg007D, std::uint16_t reg007F) const;
  void resetAlarm(ModbusClient& bus) const;

  [[nodiscard]] const MotorRegisterMap& map() const { return _map; }

 private:
  enum class SlaveTarget {
    Device,
    Command,
  };
  [[nodiscard]] std::array<std::uint16_t, 16> readOutputFunctionAssignments(
      ModbusClient& bus) const;
  [[nodiscard]] std::array<std::uint16_t, 12> readInputFunctionAssignments(
      ModbusClient& bus) const;
  [[nodiscard]] std::array<std::uint16_t, 16> readNetOutputFunctionAssignments(
      ModbusClient& bus) const;
  [[nodiscard]] std::array<std::uint16_t, 16> readNetInputFunctionAssignments(
      ModbusClient& bus) const;
  [[nodiscard]] static std::string describeOutputFunctionCode(std::uint16_t code);
  [[nodiscard]] static std::string describeInputFunctionCode(std::uint16_t code);
  [[nodiscard]] std::uint16_t operationIdMask() const;
  [[nodiscard]] std::optional<std::uint8_t> netInputBitForFunction(
      std::uint16_t functionCode) const;
  [[nodiscard]] static std::optional<std::uint16_t> fallbackInputBitMaskForFunction(
      std::uint16_t functionCode);
  [[nodiscard]] std::uint8_t decodeOperationIdFromInputRawMapped(
      std::uint16_t raw) const;
  [[nodiscard]] std::uint16_t readDriverInputCommandRawCommandTarget(
      ModbusClient& bus) const;

  void selectSlave(ModbusClient& bus, SlaveTarget target) const;
  void writeInt32(ModbusClient& bus, int upperAddr, std::int32_t value,
                  SlaveTarget target) const;

  utl::EMotor _id;
  int _slaveAddress;
  int _commandSlaveAddress;
  MotorRegisterMap _map;
  bool _forceFunction10ForSingleRegisterWrites{false};
  mutable std::optional<std::uint16_t> _driverInputCommandRawCache;
  mutable std::optional<std::uint8_t> _selectedOperationIdCache;
  mutable std::optional<std::array<std::uint16_t, 16>> _outputFunctionAssignments;
  mutable std::optional<std::array<std::uint16_t, 12>> _inputFunctionAssignments;
  mutable std::optional<std::array<std::uint16_t, 16>> _netOutputFunctionAssignments;
  mutable std::optional<std::array<std::uint16_t, 16>> _netInputFunctionAssignments;
};
