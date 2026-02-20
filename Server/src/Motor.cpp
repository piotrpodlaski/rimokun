#include <Motor.hpp>

#include <ArKd2Diagnostics.hpp>
#include <ArKd2FullRegisterMap.hpp>
#include <Logger.hpp>
#include <TimingMetrics.hpp>
#include <magic_enum/magic_enum.hpp>

#include <array>
#include <algorithm>
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

std::string joinFlags(const std::vector<std::string>& flags) {
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

constexpr std::uint16_t kFunctionFwd = 1;
constexpr std::uint16_t kFunctionRvs = 2;
constexpr std::uint16_t kFunctionHome = 3;
constexpr std::uint16_t kFunctionStart = 4;
constexpr std::uint16_t kFunctionStop = 18;
constexpr std::uint16_t kFunctionPlusJog = 6;
constexpr std::uint16_t kFunctionMinusJog = 7;
constexpr std::uint16_t kFunctionM0 = 48;
constexpr std::uint16_t kFunctionM1 = 49;
constexpr std::uint16_t kFunctionM2 = 50;
constexpr std::uint16_t kFunctionMs0 = 8;
constexpr std::uint16_t kFunctionMs1 = 9;
constexpr std::uint16_t kFunctionMs2 = 10;
constexpr std::uint16_t kFunctionCOn = 17;

int operationAddr(const int baseUpperAddr, const std::uint8_t opId) {
  if (opId > 63) {
    throw std::runtime_error(std::format("Invalid operation id {} (allowed 0..63)",
                                         static_cast<int>(opId)));
  }
  return baseUpperAddr + static_cast<int>(opId) * 2;
}

std::string outputFunctionName(const std::uint16_t code) {
  switch (code) {
    case 0: return "No function";
    case 1: return "FWD_R";
    case 2: return "RVS_R";
    case 3: return "HOME_R";
    case 4: return "START_R";
    case 5: return "SSTART_R";
    case 6: return "+JOG_R";
    case 7: return "-JOG_R";
    case 8: return "MS0_R";
    case 9: return "MS1_R";
    case 10: return "MS2_R";
    case 11: return "MS3_R";
    case 12: return "MS4_R";
    case 13: return "MS5_R";
    case 16: return "FREE_R";
    case 17: return "C-ON_R";
    case 18: return "STOP_R";
    case 32: return "R0";
    case 33: return "R1";
    case 34: return "R2";
    case 35: return "R3";
    case 36: return "R4";
    case 37: return "R5";
    case 38: return "R6";
    case 39: return "R7";
    case 40: return "R8";
    case 41: return "R9";
    case 42: return "R10";
    case 43: return "R11";
    case 44: return "R12";
    case 45: return "R13";
    case 46: return "R14";
    case 47: return "R15";
    case 48: return "M0_R";
    case 49: return "M1_R";
    case 50: return "M2_R";
    case 51: return "M3_R";
    case 52: return "M4_R";
    case 53: return "M5_R";
    case 60: return "+LS_R";
    case 61: return "-LS_R";
    case 62: return "HOMES_R";
    case 63: return "SLIT_R";
    case 65: return "ALM";
    case 66: return "WNG";
    case 67: return "READY";
    case 68: return "MOVE";
    case 69: return "END";
    case 70: return "HOME-P";
    case 71: return "TLC";
    case 72: return "TIM";
    case 73: return "AREA1";
    case 74: return "AREA2";
    case 75: return "AREA3";
    case 80: return "S-BSY";
    case 82: return "MPS";
    default: return std::format("Unknown({})", code);
  }
}

std::string inputFunctionName(const std::uint16_t code) {
  switch (code) {
    case 0: return "No function";
    case 1: return "FWD";
    case 2: return "RVS";
    case 3: return "HOME";
    case 4: return "START";
    case 5: return "SSTART";
    case 6: return "+JOG";
    case 7: return "-JOG";
    case 8: return "MS0";
    case 9: return "MS1";
    case 10: return "MS2";
    case 11: return "MS3";
    case 12: return "MS4";
    case 13: return "MS5";
    case 16: return "FREE";
    case 17: return "C-ON";
    case 18: return "STOP";
    case 24: return "ALM-RST";
    case 25: return "P-PRESET";
    case 26: return "P-CLR";
    case 27: return "HMI";
    case 32: return "R0";
    case 33: return "R1";
    case 34: return "R2";
    case 35: return "R3";
    case 36: return "R4";
    case 37: return "R5";
    case 38: return "R6";
    case 39: return "R7";
    case 40: return "R8";
    case 41: return "R9";
    case 42: return "R10";
    case 43: return "R11";
    case 44: return "R12";
    case 45: return "R13";
    case 46: return "R14";
    case 47: return "R15";
    case 48: return "M0";
    case 49: return "M1";
    case 50: return "M2";
    case 51: return "M3";
    case 52: return "M4";
    case 53: return "M5";
    default: return std::format("Unknown({})", code);
  }
}

constexpr std::array<std::uint16_t, 16> kDefaultNetInputFunctionCodes{
    48, 49, 50, 4, 3, 18, 16, 0, 8, 9, 10, 5, 6, 7, 1, 2};

constexpr std::array<std::uint16_t, 16> kDefaultNetOutputFunctionCodes{
    48, 49, 50, 4, 70, 67, 66, 65, 80, 73, 74, 75, 72, 68, 69, 71};
}  // namespace

Motor::Motor(utl::EMotor id, int slaveAddress, MotorRegisterMap map)
    : _id(id), _slaveAddress(slaveAddress), _map(std::move(map)) {}

void Motor::initialize(ModbusClient& bus) const {
  _driverInputCommandRawCache.reset();
  _selectedOperationIdCache.reset();
  _outputFunctionAssignments.reset();
  _inputFunctionAssignments.reset();
  _netOutputFunctionAssignments.reset();
  _netInputFunctionAssignments.reset();
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
  _outputFunctionAssignments = readOutputFunctionAssignments(bus);
  _inputFunctionAssignments = readInputFunctionAssignments(bus);
  _netOutputFunctionAssignments = readNetOutputFunctionAssignments(bus);
  _netInputFunctionAssignments = readNetInputFunctionAssignments(bus);
  const auto ioRaw = readDirectIoAndBrakeStatusRaw(bus);
  const auto remoteInRaw = readDriverInputCommandRaw(bus);
  const auto remoteOutRaw = readDriverOutputStatusRaw(bus);
  const auto inputStatus = decodeDriverInputStatus(inputRaw);
  const auto outputStatus = decodeDriverOutputStatus(outputRaw);
  const auto ioStatus = decodeDirectIoAndBrakeStatus(ioRaw);
  const auto remoteIoStatus = decodeRemoteIoStatus(remoteInRaw, remoteOutRaw);
  SPDLOG_INFO(
      "Motor {} (slave {}) input flags: 0x{:04X} [{}], output flags: 0x{:04X} "
      "[{}], direct IO 00D4=0x{:04X} 00D5=0x{:04X} [{}], remote IO "
      "007D=0x{:04X} 007F=0x{:04X} [{}]",
      magic_enum::enum_name(_id), _slaveAddress, inputStatus.raw,
      joinFlags(inputStatus.activeFlags), outputStatus.raw,
      joinFlags(outputStatus.activeFlags), ioStatus.reg00D4, ioStatus.reg00D5,
      joinFlags(ioStatus.activeFlags), remoteIoStatus.reg007D,
      remoteIoStatus.reg007F, joinFlags(remoteIoStatus.activeFlags));
}

std::uint32_t Motor::readU32(ModbusClient& bus, int upperAddr) const {
  RIMO_TIMED_SCOPE("Motor::readU32");
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
  RIMO_TIMED_SCOPE("Motor::readU16");
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
  RIMO_TIMED_SCOPE("Motor::writeU16");
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
  RIMO_TIMED_SCOPE("Motor::writeInt32");
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
  const auto raw = readU16(bus, _map.driverInputCommandLower);
  _driverInputCommandRawCache = raw;
  _selectedOperationIdCache = decodeOperationIdFromInputRawMapped(raw);
  return raw;
}

std::uint16_t Motor::readDriverOutputStatusRaw(ModbusClient& bus) const {
  return readU16(bus, _map.driverOutputCommandLower);
}

void Motor::writeDriverInputCommandRaw(ModbusClient& bus,
                                       const std::uint16_t raw) const {
  writeU16(bus, _map.driverInputCommandLower, raw);
  _driverInputCommandRawCache = raw;
  _selectedOperationIdCache = decodeOperationIdFromInputRawMapped(raw);
}

void Motor::setDriverInputFlag(ModbusClient& bus, const MotorInputFlag flag,
                               const bool enabled) const {
  auto raw = _driverInputCommandRawCache.has_value()
                 ? *_driverInputCommandRawCache
                 : readDriverInputCommandRaw(bus);
  std::optional<std::uint16_t> bit;
  const auto bitMaskFromMapped = [&](const std::uint16_t functionCode)
      -> std::optional<std::uint16_t> {
    if (const auto mappedBit = netInputBitForFunction(functionCode);
        mappedBit.has_value()) {
      return static_cast<std::uint16_t>(1u << *mappedBit);
    }
    return std::nullopt;
  };
  switch (flag) {
    case MotorInputFlag::Start:
      bit = bitMaskFromMapped(kFunctionStart);
      if (!bit.has_value()) bit = fallbackInputBitMaskForFunction(kFunctionStart);
      break;
    case MotorInputFlag::Home:
      bit = bitMaskFromMapped(kFunctionHome);
      if (!bit.has_value()) bit = fallbackInputBitMaskForFunction(kFunctionHome);
      break;
    case MotorInputFlag::Stop:
      bit = bitMaskFromMapped(kFunctionStop);
      if (!bit.has_value()) bit = fallbackInputBitMaskForFunction(kFunctionStop);
      break;
    case MotorInputFlag::PlusJog:
      bit = bitMaskFromMapped(kFunctionPlusJog);
      if (!bit.has_value()) bit = fallbackInputBitMaskForFunction(kFunctionPlusJog);
      break;
    case MotorInputFlag::MinusJog:
      bit = bitMaskFromMapped(kFunctionMinusJog);
      if (!bit.has_value()) bit = fallbackInputBitMaskForFunction(kFunctionMinusJog);
      break;
    case MotorInputFlag::Fwd:
      bit = bitMaskFromMapped(kFunctionFwd);
      if (!bit.has_value()) bit = fallbackInputBitMaskForFunction(kFunctionFwd);
      break;
    case MotorInputFlag::Rvs:
      bit = bitMaskFromMapped(kFunctionRvs);
      if (!bit.has_value()) bit = fallbackInputBitMaskForFunction(kFunctionRvs);
      break;
    default:
      break;
  }
  if (!bit.has_value()) {
    bit = static_cast<std::uint16_t>(flag);
  }
  raw = enabled ? static_cast<std::uint16_t>(raw | *bit)
                : static_cast<std::uint16_t>(raw & ~(*bit));
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

void Motor::setEnabled(ModbusClient& bus, const bool enabled) const {
  auto raw = _driverInputCommandRawCache.has_value()
                 ? *_driverInputCommandRawCache
                 : readDriverInputCommandRaw(bus);
  const auto mappedBit = netInputBitForFunction(kFunctionCOn);
  if (!mappedBit.has_value()) {
    SPDLOG_WARN(
        "Motor {} (slave {}) C-ON mapping not available (NET-IN function 17). "
        "Enable/disable command is currently a placeholder.",
        magic_enum::enum_name(_id), _slaveAddress);
    return;
  }
  const auto mask = static_cast<std::uint16_t>(1u << *mappedBit);
  raw = enabled ? static_cast<std::uint16_t>(raw | mask)
                : static_cast<std::uint16_t>(raw & ~mask);
  writeDriverInputCommandRaw(bus, raw);
}

std::uint8_t Motor::decodeOperationIdFromInputRaw(const std::uint16_t raw) {
  std::uint8_t opId = 0;
  if ((raw & static_cast<std::uint16_t>(MotorInputFlag::M0)) != 0) opId |= 1u;
  if ((raw & static_cast<std::uint16_t>(MotorInputFlag::M1)) != 0) opId |= 2u;
  if ((raw & static_cast<std::uint16_t>(MotorInputFlag::M2)) != 0) opId |= 4u;
  if ((raw & static_cast<std::uint16_t>(MotorInputFlag::Ms0)) != 0) opId |= 8u;
  if ((raw & static_cast<std::uint16_t>(MotorInputFlag::Ms1)) != 0)
    opId |= 16u;
  if ((raw & static_cast<std::uint16_t>(MotorInputFlag::Ms2)) != 0)
    opId |= 32u;
  return opId;
}

std::uint8_t Motor::readSelectedOperationId(ModbusClient& bus) const {
  if (_selectedOperationIdCache.has_value()) {
    return *_selectedOperationIdCache;
  }
  return decodeOperationIdFromInputRawMapped(readDriverInputCommandRaw(bus));
}

void Motor::setSelectedOperationId(ModbusClient& bus, const std::uint8_t opId) const {
  if (opId > 63) {
    throw std::runtime_error(std::format("Invalid operation id {} (allowed 0..63)",
                                         static_cast<int>(opId)));
  }
  auto raw = _driverInputCommandRawCache.has_value()
                 ? *_driverInputCommandRawCache
                 : readDriverInputCommandRaw(bus);
  raw = static_cast<std::uint16_t>(raw & ~operationIdMask());
  const auto applyOpBit = [&](const std::uint8_t opMask,
                              const std::uint16_t functionCode) {
    if ((opId & opMask) == 0u) {
      return;
    }
    const auto mappedBit = netInputBitForFunction(functionCode);
    if (mappedBit.has_value()) {
      raw = static_cast<std::uint16_t>(raw | static_cast<std::uint16_t>(1u << *mappedBit));
      return;
    }
    if (const auto fallbackMask = fallbackInputBitMaskForFunction(functionCode);
        fallbackMask.has_value()) {
      raw = static_cast<std::uint16_t>(raw | *fallbackMask);
    }
  };
  applyOpBit(1u, kFunctionM0);
  applyOpBit(2u, kFunctionM1);
  applyOpBit(4u, kFunctionM2);
  applyOpBit(8u, kFunctionMs0);
  applyOpBit(16u, kFunctionMs1);
  applyOpBit(32u, kFunctionMs2);
  writeDriverInputCommandRaw(bus, raw);
}

void Motor::setOperationMode(ModbusClient& bus, const std::uint8_t opId,
                             const MotorOperationMode mode) const {
  writeInt32(bus, operationAddr(_map.operationModeNo0, opId),
             static_cast<std::int32_t>(mode));
}

void Motor::setOperationFunction(ModbusClient& bus, const std::uint8_t opId,
                                 const MotorOperationFunction function) const {
  // Operation function starts at 0x0580 and is contiguous.
  writeInt32(bus, operationAddr(_map.operationModeNo0 + 0x80, opId),
             static_cast<std::int32_t>(function));
}

void Motor::setOperationPosition(ModbusClient& bus, const std::uint8_t opId,
                                 const std::int32_t position) const {
  writeInt32(bus, operationAddr(_map.positionNo0, opId), position);
}

void Motor::setOperationSpeed(ModbusClient& bus, const std::uint8_t opId,
                              const std::int32_t speed) const {
  writeInt32(bus, operationAddr(_map.speedNo0, opId), speed);
}

void Motor::setOperationAcceleration(ModbusClient& bus, const std::uint8_t opId,
                                     const std::int32_t acceleration) const {
  writeInt32(bus, operationAddr(_map.accelerationNo0, opId), acceleration);
}

void Motor::setOperationDeceleration(ModbusClient& bus, const std::uint8_t opId,
                                     const std::int32_t deceleration) const {
  writeInt32(bus, operationAddr(_map.decelerationNo0, opId), deceleration);
}

void Motor::setRunCurrent(ModbusClient& bus, const std::int32_t current) const {
  writeInt32(bus, _map.runCurrent, current);
}

void Motor::setStopCurrent(ModbusClient& bus, const std::int32_t current) const {
  writeInt32(bus, _map.stopCurrent, current);
}

void Motor::configureConstantSpeedPair(ModbusClient& bus,
                                       const std::int32_t speedOp0,
                                       const std::int32_t speedOp1,
                                       const std::int32_t acceleration,
                                       const std::int32_t deceleration) const {
  RIMO_TIMED_SCOPE("Motor::configureConstantSpeedPair");
  setOperationMode(bus, 0, MotorOperationMode::Incremental);
  setOperationMode(bus, 1, MotorOperationMode::Incremental);
  setOperationFunction(bus, 0, MotorOperationFunction::SingleMotion);
  setOperationFunction(bus, 1, MotorOperationFunction::SingleMotion);
  setOperationSpeed(bus, 0, speedOp0);
  setOperationSpeed(bus, 1, speedOp1);
  setOperationAcceleration(bus, 0, acceleration);
  setOperationAcceleration(bus, 1, acceleration);
  setOperationDeceleration(bus, 0, deceleration);
  setOperationDeceleration(bus, 1, deceleration);
  setSelectedOperationId(bus, 0);
}

void Motor::updateConstantSpeedBuffered(ModbusClient& bus,
                                        const std::int32_t speed) const {
  RIMO_TIMED_SCOPE("Motor::updateConstantSpeedBuffered");
  const auto active = readSelectedOperationId(bus);
  const std::uint8_t next = (active == 0u) ? 1u : 0u;
  setOperationSpeed(bus, next, speed);
  setSelectedOperationId(bus, next);
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

  const auto outputAssignments = _outputFunctionAssignments.value_or(
      std::array<std::uint16_t, 16>{});
  const bool haveOutputAssignments = _outputFunctionAssignments.has_value();
  // 00D4 mapping from original implementation:
  // bit0..5 -> OUT0..OUT5, bit8 -> MB (fixed)
  // Only 6 configurable output lines are exposed at interface level.
  for (std::size_t i = 0; i <= 5; ++i) {
    const auto code = outputAssignments[i];
    const bool active = (reg00D4 & (1u << i)) != 0;
    const auto functionName =
        haveOutputAssignments ? describeOutputFunctionCode(code)
                              : std::format("OUT{}", i);
    status.outputAssignments.push_back(
        {.channel = std::format("OUT{}", i),
         .function = functionName,
         .functionCode = code,
         .active = active});
    if (active) {
      status.activeFlags.emplace_back(
          std::format("OUT{}({})", i, functionName));
    }
  }
  const bool mbActive = (reg00D4 & (1u << 8)) != 0;
  status.outputAssignments.push_back(
      {.channel = "MB", .function = "MB", .functionCode = 0, .active = mbActive});
  if (mbActive) {
    status.activeFlags.emplace_back("MB");
  }

  const auto inputAssignments =
      _inputFunctionAssignments.value_or(std::array<std::uint16_t, 12>{});
  const bool haveInputAssignments = _inputFunctionAssignments.has_value();
  // 00D5 mapping from original implementation:
  // IN0..IN1 -> bits6..7, IN2..IN7 -> bits8..13.
  // Only 8 configurable input lines are exposed at interface level.
  const std::array<int, 8> inBitsByChannel = {6, 7, 8, 9, 10, 11, 12, 13};
  for (std::size_t i = 0; i < inBitsByChannel.size(); ++i) {
    const auto code = inputAssignments[i];
    const bool active = (reg00D5 & (1u << inBitsByChannel[i])) != 0;
    const auto functionName =
        haveInputAssignments ? describeInputFunctionCode(code)
                             : std::format("IN{}", i);
    status.inputAssignments.push_back(
        {.channel = std::format("IN{}", i),
         .function = functionName,
         .functionCode = code,
         .active = active});
    if (active) {
      status.activeFlags.emplace_back(
          std::format("IN{}({})", i, functionName));
    }
  }

  // Fixed input bits on 00D5 lower nibble.
  const auto addFixedInput = [&](const std::string& name, const int bit) {
    const bool active = (reg00D5 & (1u << bit)) != 0;
    status.inputAssignments.push_back(
        {.channel = name, .function = name, .functionCode = 0, .active = active});
    if (active) {
      status.activeFlags.push_back(name);
    }
  };
  addFixedInput("+LS", 0);
  addFixedInput("-LS", 1);
  addFixedInput("HOMES", 2);
  addFixedInput("SLIT", 3);
  return status;
}

MotorRemoteIoStatus Motor::decodeRemoteIoStatus(const std::uint16_t reg007D,
                                                const std::uint16_t reg007F) const {
  MotorRemoteIoStatus status{.reg007D = reg007D, .reg007F = reg007F};

  const auto inputAssignments =
      _netInputFunctionAssignments.value_or(kDefaultNetInputFunctionCodes);
  const auto outputAssignments =
      _netOutputFunctionAssignments.value_or(kDefaultNetOutputFunctionCodes);
  const bool haveInputAssignments = _netInputFunctionAssignments.has_value();
  const bool haveOutputAssignments = _netOutputFunctionAssignments.has_value();

  for (std::size_t i = 0; i < 16; ++i) {
    const auto inCode = inputAssignments[i];
    const auto inActive = (reg007D & (1u << i)) != 0;
    const auto inFunction =
        haveInputAssignments ? describeInputFunctionCode(inCode)
                             : (i == 7 ? std::string("not used")
                                       : describeInputFunctionCode(inCode));
    status.inputAssignments.push_back(
        {.channel = std::format("NET-IN{}", i),
         .function = inFunction,
         .functionCode = inCode,
         .active = inActive});
    if (inActive) {
      status.activeFlags.emplace_back(
          std::format("NET-IN{}({})", i, inFunction));
    }

    const auto outCode = outputAssignments[i];
    const auto outActive = (reg007F & (1u << i)) != 0;
    const auto outFunction =
        haveOutputAssignments ? describeOutputFunctionCode(outCode)
                              : describeOutputFunctionCode(outCode);
    status.outputAssignments.push_back(
        {.channel = std::format("NET-OUT{}", i),
         .function = outFunction,
         .functionCode = outCode,
         .active = outActive});
    if (outActive) {
      status.activeFlags.emplace_back(
          std::format("NET-OUT{}({})", i, outFunction));
    }
  }
  return status;
}

std::array<std::uint16_t, 16> Motor::readOutputFunctionAssignments(
    ModbusClient& bus) const {
  selectSlave(bus);
  // Function assignment registers are 32-bit values (upper/lower per channel).
  // We keep only the lower 16-bit function code.
  constexpr int kChannels = 6;
  constexpr int kWordsPerChannel = 2;
  auto regs =
      bus.read_holding_registers(_map.outputFunctionSelectBase,
                                 kChannels * kWordsPerChannel);
  if (!regs || regs->size() != static_cast<std::size_t>(kChannels * kWordsPerChannel)) {
    const auto reason = regs ? "Unexpected register count" : regs.error().message;
    throw std::runtime_error(
        std::format("Motor {} (slave {}) read output function assignment failed: {}",
                    magic_enum::enum_name(_id), _slaveAddress, reason));
  }
  std::array<std::uint16_t, 16> out{};
  for (int i = 0; i < kChannels; ++i) {
    const auto lowerWordIndex = i * kWordsPerChannel + 1;
    out[static_cast<std::size_t>(i)] = (*regs)[static_cast<std::size_t>(lowerWordIndex)];
  }
  return out;
}

std::array<std::uint16_t, 16> Motor::readNetOutputFunctionAssignments(
    ModbusClient& bus) const {
  selectSlave(bus);
  constexpr int kChannels = 16;
  constexpr int kWordsPerChannel = 2;
  auto regs =
      bus.read_holding_registers(_map.netOutputFunctionSelectBase,
                                 kChannels * kWordsPerChannel);
  if (!regs || regs->size() != static_cast<std::size_t>(kChannels * kWordsPerChannel)) {
    const auto reason = regs ? "Unexpected register count" : regs.error().message;
    throw std::runtime_error(
        std::format("Motor {} (slave {}) read NET-OUT function assignment failed: {}",
                    magic_enum::enum_name(_id), _slaveAddress, reason));
  }
  std::array<std::uint16_t, 16> out{};
  for (int i = 0; i < kChannels; ++i) {
    out[static_cast<std::size_t>(i)] =
        (*regs)[static_cast<std::size_t>(i * kWordsPerChannel + 1)];
  }
  return out;
}

std::array<std::uint16_t, 16> Motor::readNetInputFunctionAssignments(
    ModbusClient& bus) const {
  selectSlave(bus);
  constexpr int kChannels = 16;
  constexpr int kWordsPerChannel = 2;
  auto regs =
      bus.read_holding_registers(_map.netInputFunctionSelectBase,
                                 kChannels * kWordsPerChannel);
  if (!regs || regs->size() != static_cast<std::size_t>(kChannels * kWordsPerChannel)) {
    const auto reason = regs ? "Unexpected register count" : regs.error().message;
    throw std::runtime_error(
        std::format("Motor {} (slave {}) read NET-IN function assignment failed: {}",
                    magic_enum::enum_name(_id), _slaveAddress, reason));
  }
  std::array<std::uint16_t, 16> out{};
  for (int i = 0; i < kChannels; ++i) {
    out[static_cast<std::size_t>(i)] =
        (*regs)[static_cast<std::size_t>(i * kWordsPerChannel + 1)];
  }
  return out;
}

std::array<std::uint16_t, 12> Motor::readInputFunctionAssignments(
    ModbusClient& bus) const {
  selectSlave(bus);
  // Function assignment registers are 32-bit values (upper/lower per channel).
  // We keep only the lower 16-bit function code.
  constexpr int kChannels = 8;
  constexpr int kWordsPerChannel = 2;
  auto regs = bus.read_holding_registers(_map.inputFunctionSelectBase,
                                         kChannels * kWordsPerChannel);
  if (!regs || regs->size() != static_cast<std::size_t>(kChannels * kWordsPerChannel)) {
    const auto reason = regs ? "Unexpected register count" : regs.error().message;
    throw std::runtime_error(
        std::format("Motor {} (slave {}) read input function assignment failed: {}",
                    magic_enum::enum_name(_id), _slaveAddress, reason));
  }
  std::array<std::uint16_t, 12> out{};
  for (int i = 0; i < kChannels; ++i) {
    const auto lowerWordIndex = i * kWordsPerChannel + 1;
    out[static_cast<std::size_t>(i)] = (*regs)[static_cast<std::size_t>(lowerWordIndex)];
  }
  return out;
}

std::string Motor::describeOutputFunctionCode(const std::uint16_t code) {
  return outputFunctionName(code);
}

std::string Motor::describeInputFunctionCode(const std::uint16_t code) {
  return inputFunctionName(code);
}

std::optional<std::uint8_t> Motor::netInputBitForFunction(
    const std::uint16_t functionCode) const {
  if (!_netInputFunctionAssignments.has_value()) {
    return std::nullopt;
  }
  for (std::size_t i = 0; i < _netInputFunctionAssignments->size(); ++i) {
    if ((*_netInputFunctionAssignments)[i] == functionCode) {
      return static_cast<std::uint8_t>(i);
    }
  }
  return std::nullopt;
}

std::optional<std::uint16_t> Motor::fallbackInputBitMaskForFunction(
    const std::uint16_t functionCode) {
  switch (functionCode) {
    case kFunctionM0: return static_cast<std::uint16_t>(MotorInputFlag::M0);
    case kFunctionM1: return static_cast<std::uint16_t>(MotorInputFlag::M1);
    case kFunctionM2: return static_cast<std::uint16_t>(MotorInputFlag::M2);
    case kFunctionStart: return static_cast<std::uint16_t>(MotorInputFlag::Start);
    case kFunctionHome: return static_cast<std::uint16_t>(MotorInputFlag::Home);
    case kFunctionStop: return static_cast<std::uint16_t>(MotorInputFlag::Stop);
    case kFunctionMs0: return static_cast<std::uint16_t>(MotorInputFlag::Ms0);
    case kFunctionMs1: return static_cast<std::uint16_t>(MotorInputFlag::Ms1);
    case kFunctionMs2: return static_cast<std::uint16_t>(MotorInputFlag::Ms2);
    case kFunctionPlusJog:
      return static_cast<std::uint16_t>(MotorInputFlag::PlusJog);
    case kFunctionMinusJog:
      return static_cast<std::uint16_t>(MotorInputFlag::MinusJog);
    case kFunctionFwd: return static_cast<std::uint16_t>(MotorInputFlag::Fwd);
    case kFunctionRvs: return static_cast<std::uint16_t>(MotorInputFlag::Rvs);
    default: return std::nullopt;
  }
}

std::uint16_t Motor::operationIdMask() const {
  std::uint16_t mask = 0;
  for (const auto code :
       {kFunctionM0, kFunctionM1, kFunctionM2, kFunctionMs0, kFunctionMs1,
        kFunctionMs2}) {
    if (const auto mappedBit = netInputBitForFunction(code); mappedBit.has_value()) {
      mask = static_cast<std::uint16_t>(mask | static_cast<std::uint16_t>(1u << *mappedBit));
      continue;
    }
    if (const auto fallback = fallbackInputBitMaskForFunction(code);
        fallback.has_value()) {
      mask = static_cast<std::uint16_t>(mask | *fallback);
    }
  }
  return mask;
}

std::uint8_t Motor::decodeOperationIdFromInputRawMapped(const std::uint16_t raw) const {
  std::uint8_t opId = 0;
  const auto decodeBit = [&](const std::uint8_t opBit, const std::uint16_t code) {
    if (const auto mapped = netInputBitForFunction(code); mapped.has_value()) {
      if ((raw & static_cast<std::uint16_t>(1u << *mapped)) != 0u) {
        opId = static_cast<std::uint8_t>(opId | opBit);
      }
      return;
    }
    if (const auto fallback = fallbackInputBitMaskForFunction(code); fallback.has_value()) {
      if ((raw & *fallback) != 0u) {
        opId = static_cast<std::uint8_t>(opId | opBit);
      }
    }
  };
  decodeBit(1u, kFunctionM0);
  decodeBit(2u, kFunctionM1);
  decodeBit(4u, kFunctionM2);
  decodeBit(8u, kFunctionMs0);
  decodeBit(16u, kFunctionMs1);
  decodeBit(32u, kFunctionMs2);
  return opId;
}

void Motor::resetAlarm(ModbusClient& bus) const {
  if (readAlarmCode(bus) == 0u) {
    return;
  }

  // Alarm reset is a 0->1 edge on 0x0180.
  writeInt32(bus, _map.alarmResetCommand, 0u);
  writeInt32(bus, _map.alarmResetCommand, 1u);
}

void Motor::selectSlave(ModbusClient& bus) const {
  RIMO_TIMED_SCOPE("Motor::selectSlave");
  if (auto s = bus.set_slave(_slaveAddress); !s) {
    auto msg = std::format("Failed to select slave {} for motor {}: {}",
                           _slaveAddress, magic_enum::enum_name(_id),
                           s.error().message);
    SPDLOG_ERROR(msg);
    throw std::runtime_error(msg);
  }
}
