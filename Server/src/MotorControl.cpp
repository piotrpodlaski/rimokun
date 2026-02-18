#include <MotorControl.hpp>

#include <ArKd2RegisterMap.hpp>
#include <Config.hpp>
#include <Logger.hpp>
#include <TimingMetrics.hpp>
#include <magic_enum/magic_enum.hpp>
#include <cstdlib>
#include <chrono>
#include <format>
#include <stdexcept>

namespace {
const Motor& requireMotor(const std::map<utl::EMotor, Motor>& motors,
                          const utl::EMotor motorId) {
  const auto it = motors.find(motorId);
  if (it == motors.end()) {
    throw std::runtime_error(
        std::format("Motor {} is not configured in MotorControl",
                    magic_enum::enum_name(motorId)));
  }
  return it->second;
}

}  // namespace

namespace {
constexpr std::int32_t kDefaultRunCurrent = 1000;
constexpr std::int32_t kDefaultStopCurrent = 500;
constexpr std::int32_t kMinCurrent = 0;
constexpr std::int32_t kMaxCurrent = 1000;

void validateCurrentRange(const std::int32_t current, const std::string_view field,
                          const std::string_view motorName) {
  if (current < kMinCurrent || current > kMaxCurrent) {
    throw std::runtime_error(std::format(
        "MotorControl.motorCurrents.{}.{} must be in range {}..{} (got {})",
        motorName, field, kMinCurrent, kMaxCurrent, current));
  }
}
}  // namespace

MotorControl::MotorControl() {
  auto& cfg = utl::Config::instance();
  const auto model =
      cfg.getOptional<std::string>("MotorControl", "model", "AR-KD2");
  if (model == "AR-KD2") {
    _registerMap = makeArKd2RegisterMap();
  } else {
    throw std::runtime_error(
        std::format("Unsupported MotorControl model '{}'", model));
  }
  _rtuConfig.device = cfg.getOptional<std::string>("MotorControl", "device", "");
  _rtuConfig.baud = cfg.getOptional<int>("MotorControl", "baud", 9600);
  const auto parity =
      cfg.getOptional<std::string>("MotorControl", "parity", "N");
  _rtuConfig.parity = parity.empty() ? 'N' : parity.front();
  _rtuConfig.dataBits = cfg.getOptional<int>("MotorControl", "dataBits", 8);
  _rtuConfig.stopBits = cfg.getOptional<int>("MotorControl", "stopBits", 1);
  _rtuConfig.responseTimeoutMS =
      cfg.getOptional<unsigned>("MotorControl", "responseTimeoutMS", 1000u);
  _rtuConfig.connectTimeoutMS =
      cfg.getOptional<unsigned>("MotorControl", "connectTimeoutMS", 1000u);

  const auto motorCfg = cfg.getClassConfig("MotorControl");
  const auto transportCfg = motorCfg["transport"];
  if (transportCfg && transportCfg.IsMap()) {
    const auto type = transportCfg["type"].as<std::string>("rawTcpRtu");
    if (type == "rawTcpRtu" || type == "rtuTcpRaw" || type == "tcpRawRtu") {
      _transportType = TransportType::RawTcpRtu;
      const auto tcpCfg = transportCfg["tcp"];
      if (!tcpCfg || !tcpCfg.IsMap()) {
        throw std::runtime_error(
            "MotorControl.transport.tcp is required for rawTcpRtu transport.");
      }
      _rawTcpConfig.host = tcpCfg["host"].as<std::string>();
      _rawTcpConfig.port = tcpCfg["port"].as<int>();
    } else if (type == "serialRtu" || type == "serial" || type == "rtuSerial") {
      _transportType = TransportType::SerialRtu;
      const auto serialCfg = transportCfg["serial"];
      if (!serialCfg || !serialCfg.IsMap()) {
        throw std::runtime_error(
            "MotorControl.transport.serial is required for serialRtu transport.");
      }
      _rtuConfig.device = serialCfg["device"].as<std::string>();
      _rtuConfig.baud = serialCfg["baud"].as<int>(9600);
      const auto parity = serialCfg["parity"].as<std::string>("N");
      _rtuConfig.parity = parity.empty() ? 'N' : parity.front();
      _rtuConfig.dataBits = serialCfg["dataBits"].as<int>(8);
      _rtuConfig.stopBits = serialCfg["stopBits"].as<int>(1);
    } else {
      throw std::runtime_error(
          std::format("Unsupported MotorControl transport type '{}'", type));
    }
  } else {
    // Legacy config path: keep existing serial keys behavior.
    _transportType = TransportType::SerialRtu;
    if (_rtuConfig.device.empty()) {
      throw std::runtime_error(
          "MotorControl.device is required when using legacy serial config.");
    }
  }

  _motorAddresses = cfg.getRequired<std::map<utl::EMotor, int>>(
      "MotorControl", "motorAddresses");

  for (const auto& [motorId, _] : _motorAddresses) {
    _motorCurrents[motorId] = MotorCurrentConfig{
        .runCurrent = kDefaultRunCurrent, .stopCurrent = kDefaultStopCurrent};
  }

  const auto motorCurrentsNode = motorCfg["motorCurrents"];
  if (motorCurrentsNode && motorCurrentsNode.IsMap()) {
    for (const auto& kv : motorCurrentsNode) {
      auto motorId = kv.first.as<utl::EMotor>();
      const auto entry = kv.second;
      if (!entry || !entry.IsMap()) {
        throw std::runtime_error(std::format(
            "MotorControl.motorCurrents.{} must be a map with runCurrent/stopCurrent",
            magic_enum::enum_name(motorId)));
      }
      auto runCurrent = entry["runCurrent"].as<std::int32_t>(kDefaultRunCurrent);
      auto stopCurrent =
          entry["stopCurrent"].as<std::int32_t>(kDefaultStopCurrent);
      validateCurrentRange(runCurrent, "runCurrent", magic_enum::enum_name(motorId));
      validateCurrentRange(stopCurrent, "stopCurrent",
                           magic_enum::enum_name(motorId));
      _motorCurrents[motorId] = MotorCurrentConfig{
          .runCurrent = runCurrent, .stopCurrent = stopCurrent};
    }
  }
}

void MotorControl::initialize() {
  _motors.clear();
  _runtime.clear();
  try {
    ModbusResult<ModbusClient> busRes =
        std::unexpected(ModbusError{EINVAL, "Motor transport is not configured"});
    if (_transportType == TransportType::SerialRtu) {
      busRes = ModbusClient::rtu(_rtuConfig.device, _rtuConfig.baud,
                                 _rtuConfig.parity, _rtuConfig.dataBits,
                                 _rtuConfig.stopBits, 1);
    } else {
      busRes = ModbusClient::rtu_over_tcp(_rawTcpConfig.host, _rawTcpConfig.port,
                                          1);
    }
    if (!busRes) {
      auto msg = std::format("Failed to create motor bus client: {}",
                             busRes.error().message);
      throw std::runtime_error(msg);
    }
    _bus.emplace(std::move(*busRes));
    if (auto ct = _bus->set_connect_timeout(
            std::chrono::milliseconds{_rtuConfig.connectTimeoutMS});
        !ct) {
      auto msg = std::format("Failed to set motor bus connect timeout: {}",
                             ct.error().message);
      _bus.reset();
      throw std::runtime_error(msg);
    }
    if (auto c = _bus->connect(); !c) {
      const auto endpoint =
          _transportType == TransportType::SerialRtu
              ? _rtuConfig.device
              : std::format("{}:{}", _rawTcpConfig.host, _rawTcpConfig.port);
      auto msg = std::format("Failed to connect motor bus on '{}': {}",
                             endpoint, c.error().message);
      _bus.reset();
      throw std::runtime_error(msg);
    }
    if (auto t = _bus->set_response_timeout(
            std::chrono::milliseconds{_rtuConfig.responseTimeoutMS});
        !t) {
      auto msg = std::format("Failed to set RTU bus timeout: {}",
                             t.error().message);
      _bus->close();
      _bus.reset();
      throw std::runtime_error(msg);
    }

    for (const auto& [motorId, address] : _motorAddresses) {
      if (address < 0 || address > 247) {
        auto msg = std::format("Invalid Modbus slave address {} for motor {}",
                               address, magic_enum::enum_name(motorId));
        SPDLOG_ERROR(msg);
        throw std::runtime_error(msg);
      }
      auto [it, inserted] =
          _motors.emplace(motorId, Motor(motorId, address, _registerMap));
      (void)inserted;
      _runtime.emplace(motorId, MotorRuntimeState{});
      {
        std::lock_guard<std::mutex> lock(_busMutex);
        it->second.initialize(*_bus);
        const auto currentIt = _motorCurrents.find(motorId);
        if (currentIt != _motorCurrents.end()) {
          it->second.setRunCurrent(*_bus, currentIt->second.runCurrent);
          it->second.setStopCurrent(*_bus, currentIt->second.stopCurrent);
        }
      }
    }
    setState(State::Normal);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("MotorControl initialize failed: {}", e.what());
    if (_bus) {
      _bus->close();
      _bus.reset();
    }
    _motors.clear();
    _runtime.clear();
    setState(State::Error);
    throw;
  }
}

void MotorControl::reset() {
  SPDLOG_INFO("Resetting MotorControl component.");
  if (_bus) {
    _bus->close();
    _bus.reset();
  }
  _motors.clear();
  _runtime.clear();
  setState(State::Error);
}

void MotorControl::setMode(const utl::EMotor motorId, const MotorControlMode mode) {
  RIMO_TIMED_SCOPE("MotorControl::setMode");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    throw std::runtime_error(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");

  runtime.mode = mode;
  if (mode == MotorControlMode::Speed) {
    if (!runtime.speedPairPrepared) {
      motor.configureConstantSpeedPair(*_bus, runtime.speed, runtime.speed,
                                       runtime.acceleration,
                                       runtime.deceleration);
      runtime.speedPairPrepared = true;
    }
  } else {
    if (!runtime.positionPrepared) {
      motor.setOperationMode(*_bus, 2, MotorOperationMode::Incremental);
      motor.setOperationFunction(*_bus, 2, MotorOperationFunction::SingleMotion);
      motor.setOperationSpeed(*_bus, 2, runtime.speed);
      motor.setOperationAcceleration(*_bus, 2, runtime.acceleration);
      motor.setOperationDeceleration(*_bus, 2, runtime.deceleration);
      runtime.positionPrepared = true;
    }
  }
}

void MotorControl::setSpeed(const utl::EMotor motorId, const std::int32_t speed) {
  RIMO_TIMED_SCOPE("MotorControl::setSpeed");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    throw std::runtime_error(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  runtime.speed = std::abs(speed);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");

  if (runtime.mode == MotorControlMode::Speed) {
    if (!runtime.speedPairPrepared) {
      motor.configureConstantSpeedPair(*_bus, runtime.speed, runtime.speed,
                                       runtime.acceleration,
                                       runtime.deceleration);
      runtime.speedPairPrepared = true;
    } else {
      motor.updateConstantSpeedBuffered(*_bus, runtime.speed);
    }
  }
}

void MotorControl::setPosition(const utl::EMotor motorId,
                               const std::int32_t position) {
  RIMO_TIMED_SCOPE("MotorControl::setPosition");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    throw std::runtime_error(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  runtime.position = position;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");

  if (runtime.mode == MotorControlMode::Position) {
    if (!runtime.positionPrepared) {
      motor.setOperationMode(*_bus, 2, MotorOperationMode::Incremental);
      motor.setOperationFunction(*_bus, 2, MotorOperationFunction::SingleMotion);
      motor.setOperationSpeed(*_bus, 2, runtime.speed);
      motor.setOperationAcceleration(*_bus, 2, runtime.acceleration);
      motor.setOperationDeceleration(*_bus, 2, runtime.deceleration);
      runtime.positionPrepared = true;
    }
    motor.setOperationPosition(*_bus, 2, runtime.position);
  }
}

void MotorControl::setDirection(const utl::EMotor motorId,
                                const MotorControlDirection direction) {
  RIMO_TIMED_SCOPE("MotorControl::setDirection");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    throw std::runtime_error(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  runtime.direction = direction;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");

  if (direction == MotorControlDirection::Forward) {
    motor.setReverse(*_bus, false);
    motor.setForward(*_bus, true);
  } else {
    motor.setForward(*_bus, false);
    motor.setReverse(*_bus, true);
  }
}

void MotorControl::startMovement(const utl::EMotor motorId) {
  RIMO_TIMED_SCOPE("MotorControl::startMovement");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    throw std::runtime_error(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");

  if (runtime.mode == MotorControlMode::Speed) {
    if (!runtime.speedPairPrepared) {
      motor.configureConstantSpeedPair(*_bus, runtime.speed, runtime.speed,
                                       runtime.acceleration,
                                       runtime.deceleration);
      runtime.speedPairPrepared = true;
    }
    if (runtime.direction == MotorControlDirection::Forward) {
      motor.setReverse(*_bus, false);
      motor.setForward(*_bus, true);
    } else {
      motor.setForward(*_bus, false);
      motor.setReverse(*_bus, true);
    }
    return;
  } else {
    if (!runtime.positionPrepared) {
      motor.setOperationMode(*_bus, 2, MotorOperationMode::Incremental);
      motor.setOperationFunction(*_bus, 2, MotorOperationFunction::SingleMotion);
      motor.setOperationSpeed(*_bus, 2, runtime.speed);
      motor.setOperationAcceleration(*_bus, 2, runtime.acceleration);
      motor.setOperationDeceleration(*_bus, 2, runtime.deceleration);
      runtime.positionPrepared = true;
    }
    motor.setOperationPosition(*_bus, 2, runtime.position);
    motor.setSelectedOperationId(*_bus, 2);
  }
  motor.pulseStart(*_bus);
}

void MotorControl::stopMovement(const utl::EMotor motorId) {
  RIMO_TIMED_SCOPE("MotorControl::stopMovement");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    throw std::runtime_error(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  const auto& runtime = rtIt->second;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  if (runtime.mode == MotorControlMode::Speed) {
    // AR-KD2 speed mode: motion is level-controlled through FWD/RVS bits.
    // Stop by clearing the whole driver input command register (0x007D).
    motor.writeDriverInputCommandRaw(*_bus, 0);
    return;
  }
  motor.pulseStop(*_bus);
}

void MotorControl::pulseStart(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.pulseStart(*_bus);
}

void MotorControl::pulseStop(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.pulseStop(*_bus);
}

void MotorControl::pulseHome(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.pulseHome(*_bus);
}

void MotorControl::setForward(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setForward(*_bus, enabled);
}

void MotorControl::setReverse(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setReverse(*_bus, enabled);
}

void MotorControl::setJogPlus(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setJogPlus(*_bus, enabled);
}

void MotorControl::setJogMinus(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setJogMinus(*_bus, enabled);
}

std::uint8_t MotorControl::readSelectedOperationId(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  return motor.readSelectedOperationId(*_bus);
}

void MotorControl::setSelectedOperationId(const utl::EMotor motorId,
                                          const std::uint8_t opId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setSelectedOperationId(*_bus, opId);
}

void MotorControl::setOperationMode(const utl::EMotor motorId,
                                    const std::uint8_t opId,
                                    const MotorOperationMode mode) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setOperationMode(*_bus, opId, mode);
}

void MotorControl::setOperationFunction(const utl::EMotor motorId,
                                        const std::uint8_t opId,
                                        const MotorOperationFunction function) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setOperationFunction(*_bus, opId, function);
}

void MotorControl::setOperationPosition(const utl::EMotor motorId,
                                        const std::uint8_t opId,
                                        const std::int32_t position) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setOperationPosition(*_bus, opId, position);
}

void MotorControl::setOperationSpeed(const utl::EMotor motorId,
                                     const std::uint8_t opId,
                                     const std::int32_t speed) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setOperationSpeed(*_bus, opId, speed);
}

void MotorControl::setOperationAcceleration(const utl::EMotor motorId,
                                            const std::uint8_t opId,
                                            const std::int32_t acceleration) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setOperationAcceleration(*_bus, opId, acceleration);
}

void MotorControl::setOperationDeceleration(const utl::EMotor motorId,
                                            const std::uint8_t opId,
                                            const std::int32_t deceleration) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setOperationDeceleration(*_bus, opId, deceleration);
}

void MotorControl::setRunCurrent(const utl::EMotor motorId,
                                 const std::int32_t current) {
  validateCurrentRange(current, "runCurrent", magic_enum::enum_name(motorId));
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setRunCurrent(*_bus, current);
}

void MotorControl::setStopCurrent(const utl::EMotor motorId,
                                  const std::int32_t current) {
  validateCurrentRange(current, "stopCurrent", magic_enum::enum_name(motorId));
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.setStopCurrent(*_bus, current);
}

void MotorControl::configureConstantSpeedPair(const utl::EMotor motorId,
                                              const std::int32_t speedOp0,
                                              const std::int32_t speedOp1,
                                              const std::int32_t acceleration,
                                              const std::int32_t deceleration) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.configureConstantSpeedPair(*_bus, speedOp0, speedOp1, acceleration,
                                   deceleration);
}

void MotorControl::updateConstantSpeedBuffered(const utl::EMotor motorId,
                                               const std::int32_t speed) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  motor.updateConstantSpeedBuffered(*_bus, speed);
}

MotorFlagStatus MotorControl::readInputStatus(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  return motor.decodeDriverInputStatus(motor.readDriverInputCommandRaw(*_bus));
}

MotorFlagStatus MotorControl::readOutputStatus(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  return motor.decodeDriverOutputStatus(motor.readDriverOutputStatusRaw(*_bus));
}

MotorDirectIoStatus MotorControl::readDirectIoStatus(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) throw std::runtime_error("MotorControl bus is not initialized");
  return motor.decodeDirectIoAndBrakeStatus(
      motor.readDirectIoAndBrakeStatusRaw(*_bus));
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentAlarm(
    const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) {
    throw std::runtime_error("MotorControl bus is not initialized");
  }
  return motor.diagnoseAlarm(motor.readAlarmCode(*_bus));
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentWarning(
    const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) {
    throw std::runtime_error("MotorControl bus is not initialized");
  }
  return motor.diagnoseWarning(motor.readWarningCode(*_bus));
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentCommunicationError(
    const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) {
    throw std::runtime_error("MotorControl bus is not initialized");
  }
  return motor.diagnoseCommunicationError(
      motor.readCommunicationErrorCode(*_bus));
}
