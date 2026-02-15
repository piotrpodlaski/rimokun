#pragma once

#include <CommonDefinitions.hpp>
#include <ModbusClient.hpp>
#include <MotorRegisterMap.hpp>

#include <optional>
#include <string>

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

class Motor {
 public:
  Motor(utl::EMotor id, int slaveAddress, MotorRegisterMap map);

  [[nodiscard]] utl::EMotor id() const { return _id; }
  [[nodiscard]] int slaveAddress() const { return _slaveAddress; }

  void initialize(ModbusClient& bus) const;
  [[nodiscard]] std::uint32_t readU32(ModbusClient& bus, int upperAddr) const;
  void writeInt32(ModbusClient& bus, int upperAddr, std::int32_t value) const;
  [[nodiscard]] std::uint8_t readAlarmCode(ModbusClient& bus) const;
  [[nodiscard]] std::uint8_t readWarningCode(ModbusClient& bus) const;
  [[nodiscard]] std::uint8_t readCommunicationErrorCode(ModbusClient& bus) const;
  [[nodiscard]] MotorCodeDiagnostic diagnoseAlarm(std::uint8_t code) const;
  [[nodiscard]] MotorCodeDiagnostic diagnoseWarning(std::uint8_t code) const;
  [[nodiscard]] MotorCodeDiagnostic diagnoseCommunicationError(
      std::uint8_t code) const;
  void resetAlarm(ModbusClient& bus) const;

  [[nodiscard]] const MotorRegisterMap& map() const { return _map; }

 private:
  void selectSlave(ModbusClient& bus) const;

  utl::EMotor _id;
  int _slaveAddress;
  MotorRegisterMap _map;
};
