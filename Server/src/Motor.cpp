#include <Motor.hpp>

#include <ArKd2Diagnostics.hpp>
#include <ArKd2FullRegisterMap.hpp>
#include <Logger.hpp>
#include <magic_enum/magic_enum.hpp>

#include <array>
#include <format>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {
std::string registerLabel(int upperAddr) {
  const auto upper = static_cast<std::uint16_t>(upperAddr);
  const auto lower = static_cast<std::uint16_t>(upperAddr + 1);
  const auto upperName = arKd2RegisterName(upper);
  const auto lowerName = arKd2RegisterName(lower);

  if (upperName && lowerName) {
    return std::format("0x{:04X}/0x{:04X} ({} / {})", upper, lower, *upperName,
                       *lowerName);
  }
  if (upperName) {
    return std::format("0x{:04X} ({})", upper, *upperName);
  }
  return std::format("0x{:04X}", upper);
}

MotorCodeDiagnostic asDiagnostic(const MotorDiagnosticDomain domain,
                                 const std::uint8_t code,
                                 const std::optional<ArKd2CodeInfo>& info) {
  MotorCodeDiagnostic out{.domain = domain, .code = code};
  if (info.has_value()) {
    out.known = true;
    out.type = std::string(info->type);
    out.cause = std::string(info->cause);
    out.remedialAction = std::string(info->remedialAction);
  } else {
    out.known = false;
    out.type = "Unknown";
    out.cause = "Code is not present in current AR-KD2 diagnostics table.";
    out.remedialAction = "Check HM-60506E alarm/warning/error lists.";
  }
  return out;
}

constexpr std::array<std::pair<MotorInputFlag, std::string_view>, 15>
    kInputFlags{{
        {MotorInputFlag::M0, "M0"},
        {MotorInputFlag::M1, "M1"},
        {MotorInputFlag::M2, "M2"},
        {MotorInputFlag::Start, "START"},
        {MotorInputFlag::Home, "HOME"},
        {MotorInputFlag::Stop, "STOP"},
        {MotorInputFlag::Free, "FREE"},
        {MotorInputFlag::Ms0, "MS0"},
        {MotorInputFlag::Ms1, "MS1"},
        {MotorInputFlag::Ms2, "MS2"},
        {MotorInputFlag::SStart, "SSTART"},
        {MotorInputFlag::PlusJog, "+JOG"},
        {MotorInputFlag::MinusJog, "-JOG"},
        {MotorInputFlag::Fwd, "FWD"},
        {MotorInputFlag::Rvs, "RVS"},
    }};

constexpr std::array<std::pair<MotorOutputFlag, std::string_view>, 16>
    kOutputFlags{{
        {MotorOutputFlag::M0R, "M0_R"},
        {MotorOutputFlag::M1R, "M1_R"},
        {MotorOutputFlag::M2R, "M2_R"},
        {MotorOutputFlag::StartR, "START_R"},
        {MotorOutputFlag::HomeP, "HOME-P"},
        {MotorOutputFlag::Ready, "READY"},
        {MotorOutputFlag::Warning, "WNG"},
        {MotorOutputFlag::Alarm, "ALM"},
        {MotorOutputFlag::SBusy, "S-BSY"},
        {MotorOutputFlag::Area1, "AREA1"},
        {MotorOutputFlag::Area2, "AREA2"},
        {MotorOutputFlag::Area3, "AREA3"},
        {MotorOutputFlag::Tim, "TIM"},
        {MotorOutputFlag::Move, "MOVE"},
        {MotorOutputFlag::End, "END"},
        {MotorOutputFlag::Tlc, "TLC"},
    }};

std::string joinFlags(const std::vector<std::string_view>& flags) {
  if (flags.empty()) {
    return "none";
  }
  std::string out;
  for (std::size_t i = 0; i < flags.size(); ++i) {
    if (i != 0) {
      out += ", ";
    }
    out += flags[i];
  }
  return out;
}
}  // namespace

Motor::Motor(utl::EMotor id, int slaveAddress, MotorRegisterMap map)
    : _id(id), _slaveAddress(slaveAddress), _map(std::move(map)) {}

void Motor::initialize(ModbusClient& bus) const {
  selectSlave(bus);

  // AR-KD2 sanity check: read present alarm register pair.
  auto alarm = bus.read_holding_registers(_map.presentAlarm, 2);
  if (!alarm) {
    auto msg = std::format("Motor {} (slave {}) RTU test failed while reading "
                           "presentAlarm {}: {}",
                           magic_enum::enum_name(_id), _slaveAddress,
                           registerLabel(_map.presentAlarm),
                           alarm.error().message);
    SPDLOG_ERROR(msg);
    throw std::runtime_error(msg);
  }

  const auto alarmCode = readAlarmCode(bus);
  if (alarmCode != 0) {
    const auto diag = diagnoseAlarm(alarmCode);
    SPDLOG_WARN(
        "Motor {} (slave {}) startup alarm 0x{:02X}: {} | cause: {} | action: "
        "{}",
        magic_enum::enum_name(_id), _slaveAddress, alarmCode, diag.type,
        diag.cause, diag.remedialAction);
  }

  const auto warningCode = readWarningCode(bus);
  if (warningCode != 0) {
    const auto diag = diagnoseWarning(warningCode);
    SPDLOG_WARN(
        "Motor {} (slave {}) startup warning 0x{:02X}: {} | cause: {} | "
        "action: {}",
        magic_enum::enum_name(_id), _slaveAddress, warningCode, diag.type,
        diag.cause, diag.remedialAction);
  }

  const auto commErrCode = readCommunicationErrorCode(bus);
  if (commErrCode != 0) {
    const auto diag = diagnoseCommunicationError(commErrCode);
    SPDLOG_WARN(
        "Motor {} (slave {}) startup communication error 0x{:02X}: {} | "
        "cause: {} | action: {}",
        magic_enum::enum_name(_id), _slaveAddress, commErrCode, diag.type,
        diag.cause, diag.remedialAction);
  }

  const auto inputRaw = readDriverInputCommandRaw(bus);
  const auto outputRaw = readDriverOutputStatusRaw(bus);
  const auto ioRaw = readDirectIoAndBrakeStatusRaw(bus);
  const auto inputStatus = decodeDriverInputStatus(inputRaw);
  const auto outputStatus = decodeDriverOutputStatus(outputRaw);
  const auto ioStatus = decodeDirectIoAndBrakeStatus(ioRaw);
  SPDLOG_INFO(
      "Motor {} (slave {}) input flags: 0x{:04X} [{}], output flags: 0x{:04X} "
      "[{}], direct IO 00D4=0x{:04X} 00D5=0x{:04X} [{}]",
      magic_enum::enum_name(_id), _slaveAddress, inputStatus.raw,
      joinFlags(inputStatus.activeFlags), outputStatus.raw,
      joinFlags(outputStatus.activeFlags), ioStatus.reg00D4, ioStatus.reg00D5,
      joinFlags(ioStatus.activeFlags));
}

std::uint32_t Motor::readU32(ModbusClient& bus, int upperAddr) const {
  selectSlave(bus);
  auto regs = bus.read_holding_registers(upperAddr, 2);
  if (!regs || regs->size() != 2) {
    auto reason = regs ? "Unexpected register count" : regs.error().message;
    auto msg = std::format("Motor {} (slave {}) readU32({}) failed: {}",
                           magic_enum::enum_name(_id), _slaveAddress,
                           registerLabel(upperAddr), reason);
    throw std::runtime_error(msg);
  }
  return (static_cast<std::uint32_t>((*regs)[0]) << 16) |
         static_cast<std::uint32_t>((*regs)[1]);
}

std::uint16_t Motor::readU16(ModbusClient& bus, const int addr) const {
  selectSlave(bus);
  auto regs = bus.read_holding_registers(addr, 1);
  if (!regs || regs->size() != 1) {
    auto reason = regs ? "Unexpected register count" : regs.error().message;
    auto msg = std::format("Motor {} (slave {}) readU16(0x{:04X}) failed: {}",
                           magic_enum::enum_name(_id), _slaveAddress, addr,
                           reason);
    throw std::runtime_error(msg);
  }
  return (*regs)[0];
}

void Motor::writeU16(ModbusClient& bus, const int addr,
                     const std::uint16_t value) const {
  selectSlave(bus);
  auto wr = bus.write_single_register(addr, value);
  if (!wr) {
    auto msg = std::format(
        "Motor {} (slave {}) writeU16(0x{:04X}, 0x{:04X}) failed: {}",
        magic_enum::enum_name(_id), _slaveAddress, addr, value,
        wr.error().message);
    throw std::runtime_error(msg);
  }
}

void Motor::writeInt32(ModbusClient& bus, int upperAddr, std::int32_t value) const {
  selectSlave(bus);
  std::uint32_t raw = static_cast<std::uint32_t>(value);
  std::vector<std::uint16_t> words = {
      static_cast<std::uint16_t>((raw >> 16) & 0xFFFFu),
      static_cast<std::uint16_t>(raw & 0xFFFFu)};
  auto wr = bus.write_multiple_registers(upperAddr, words);
  if (!wr) {
    auto msg =
        std::format("Motor {} (slave {}) writeInt32({}, {}) failed: {}",
                    magic_enum::enum_name(_id), _slaveAddress,
                    registerLabel(upperAddr), value, wr.error().message);
    throw std::runtime_error(msg);
  }
}

std::uint8_t Motor::readAlarmCode(ModbusClient& bus) const {
  const auto alarm = readU32(bus, _map.presentAlarm);
  return static_cast<std::uint8_t>(alarm & 0xFFu);
}

std::uint8_t Motor::readWarningCode(ModbusClient& bus) const {
  const auto warning = readU32(bus, _map.presentWarning);
  return static_cast<std::uint8_t>(warning & 0xFFu);
}

std::uint8_t Motor::readCommunicationErrorCode(ModbusClient& bus) const {
  const auto commErr = readU32(bus, _map.communicationErrorCode);
  return static_cast<std::uint8_t>(commErr & 0xFFu);
}

MotorCodeDiagnostic Motor::diagnoseAlarm(const std::uint8_t code) const {
  return asDiagnostic(MotorDiagnosticDomain::Alarm, code, arKd2FindAlarm(code));
}

MotorCodeDiagnostic Motor::diagnoseWarning(const std::uint8_t code) const {
  return asDiagnostic(MotorDiagnosticDomain::Warning, code,
                      arKd2FindWarning(code));
}

MotorCodeDiagnostic Motor::diagnoseCommunicationError(
    const std::uint8_t code) const {
  return asDiagnostic(MotorDiagnosticDomain::CommunicationError, code,
                      arKd2FindCommunicationError(code));
}

std::uint16_t Motor::readDriverInputCommandRaw(ModbusClient& bus) const {
  return readU16(bus, _map.driverInputCommandLower);
}

std::uint16_t Motor::readDriverOutputStatusRaw(ModbusClient& bus) const {
  return readU16(bus, _map.driverOutputCommandLower);
}

void Motor::writeDriverInputCommandRaw(ModbusClient& bus,
                                       const std::uint16_t raw) const {
  writeU16(bus, _map.driverInputCommandLower, raw);
}

void Motor::setDriverInputFlag(ModbusClient& bus, const MotorInputFlag flag,
                               const bool enabled) const {
  auto raw = readDriverInputCommandRaw(bus);
  const auto bit = static_cast<std::uint16_t>(flag);
  raw = enabled ? static_cast<std::uint16_t>(raw | bit)
                : static_cast<std::uint16_t>(raw & ~bit);
  writeDriverInputCommandRaw(bus, raw);
}

bool Motor::isDriverOutputFlagSet(const std::uint16_t raw,
                                  const MotorOutputFlag flag) {
  return (raw & static_cast<std::uint16_t>(flag)) != 0;
}

void Motor::pulseDriverInputFlag(ModbusClient& bus, const MotorInputFlag flag,
                                 const std::chrono::milliseconds hold) const {
  setDriverInputFlag(bus, flag, true);
  std::this_thread::sleep_for(hold);
  setDriverInputFlag(bus, flag, false);
}

void Motor::pulseStart(ModbusClient& bus) const {
  pulseDriverInputFlag(bus, MotorInputFlag::Start);
}

void Motor::pulseStop(ModbusClient& bus) const {
  pulseDriverInputFlag(bus, MotorInputFlag::Stop);
}

void Motor::pulseHome(ModbusClient& bus) const {
  pulseDriverInputFlag(bus, MotorInputFlag::Home);
}

void Motor::setForward(ModbusClient& bus, const bool enabled) const {
  setDriverInputFlag(bus, MotorInputFlag::Fwd, enabled);
}

void Motor::setReverse(ModbusClient& bus, const bool enabled) const {
  setDriverInputFlag(bus, MotorInputFlag::Rvs, enabled);
}

void Motor::setJogPlus(ModbusClient& bus, const bool enabled) const {
  setDriverInputFlag(bus, MotorInputFlag::PlusJog, enabled);
}

void Motor::setJogMinus(ModbusClient& bus, const bool enabled) const {
  setDriverInputFlag(bus, MotorInputFlag::MinusJog, enabled);
}

MotorFlagStatus Motor::decodeDriverInputStatus(const std::uint16_t raw) const {
  MotorFlagStatus status{.raw = raw};
  for (const auto& [bit, name] : kInputFlags) {
    if ((raw & static_cast<std::uint16_t>(bit)) != 0) {
      status.activeFlags.emplace_back(name);
    }
  }
  return status;
}

MotorFlagStatus Motor::decodeDriverOutputStatus(const std::uint16_t raw) const {
  MotorFlagStatus status{.raw = raw};
  for (const auto& [bit, name] : kOutputFlags) {
    if (isDriverOutputFlagSet(raw, bit)) {
      status.activeFlags.emplace_back(name);
    }
  }
  return status;
}

std::uint32_t Motor::readDirectIoAndBrakeStatusRaw(ModbusClient& bus) const {
  return readU32(bus, _map.directIoAndBrakeStatus);
}

MotorDirectIoStatus Motor::decodeDirectIoAndBrakeStatus(
    const std::uint32_t raw) const {
  const auto reg00D4 = static_cast<std::uint16_t>((raw >> 16) & 0xFFFFu);
  const auto reg00D5 = static_cast<std::uint16_t>(raw & 0xFFFFu);
  MotorDirectIoStatus status{.reg00D4 = reg00D4, .reg00D5 = reg00D5};

  // 00D4h lower byte
  if ((reg00D4 & (1u << 0)) != 0) status.activeFlags.emplace_back("OUT0");
  if ((reg00D4 & (1u << 1)) != 0) status.activeFlags.emplace_back("OUT1");
  if ((reg00D4 & (1u << 2)) != 0) status.activeFlags.emplace_back("OUT2");
  if ((reg00D4 & (1u << 3)) != 0) status.activeFlags.emplace_back("OUT3");
  if ((reg00D4 & (1u << 4)) != 0) status.activeFlags.emplace_back("OUT4");
  if ((reg00D4 & (1u << 5)) != 0) status.activeFlags.emplace_back("OUT5");
  // 00D4h upper byte bit0 (absolute bit8)
  if ((reg00D4 & (1u << 8)) != 0) status.activeFlags.emplace_back("MB");

  // 00D5h upper byte
  if ((reg00D5 & (1u << 13)) != 0) status.activeFlags.emplace_back("IN7");
  if ((reg00D5 & (1u << 12)) != 0) status.activeFlags.emplace_back("IN6");
  if ((reg00D5 & (1u << 11)) != 0) status.activeFlags.emplace_back("IN5");
  if ((reg00D5 & (1u << 10)) != 0) status.activeFlags.emplace_back("IN4");
  if ((reg00D5 & (1u << 9)) != 0) status.activeFlags.emplace_back("IN3");
  if ((reg00D5 & (1u << 8)) != 0) status.activeFlags.emplace_back("IN2");
  // 00D5h lower byte
  if ((reg00D5 & (1u << 7)) != 0) status.activeFlags.emplace_back("IN1");
  if ((reg00D5 & (1u << 6)) != 0) status.activeFlags.emplace_back("IN0");
  if ((reg00D5 & (1u << 3)) != 0) status.activeFlags.emplace_back("SLIT");
  if ((reg00D5 & (1u << 2)) != 0) status.activeFlags.emplace_back("HOMES");
  if ((reg00D5 & (1u << 1)) != 0) status.activeFlags.emplace_back("-LS");
  if ((reg00D5 & (1u << 0)) != 0) status.activeFlags.emplace_back("+LS");
  return status;
}

void Motor::resetAlarm(ModbusClient& bus) const {
  writeInt32(bus, _map.alarmResetCommand, 0);
  writeInt32(bus, _map.alarmResetCommand, 1);
}

void Motor::selectSlave(ModbusClient& bus) const {
  if (auto s = bus.set_slave(_slaveAddress); !s) {
    auto msg = std::format("Failed to select slave {} for motor {}: {}",
                           _slaveAddress, magic_enum::enum_name(_id),
                           s.error().message);
    SPDLOG_ERROR(msg);
    throw std::runtime_error(msg);
  }
}
