#include <MotorControl.hpp>

#include <ArKd2RegisterMap.hpp>
#include <Config.hpp>
#include <Logger.hpp>
#include <magic_enum/magic_enum.hpp>
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
  _rtuConfig.device = cfg.getRequired<std::string>("MotorControl", "device");
  _rtuConfig.baud = cfg.getOptional<int>("MotorControl", "baud", 9600);
  const auto parity =
      cfg.getOptional<std::string>("MotorControl", "parity", "N");
  _rtuConfig.parity = parity.empty() ? 'N' : parity.front();
  _rtuConfig.dataBits = cfg.getOptional<int>("MotorControl", "dataBits", 8);
  _rtuConfig.stopBits = cfg.getOptional<int>("MotorControl", "stopBits", 1);
  _rtuConfig.responseTimeoutMS =
      cfg.getOptional<unsigned>("MotorControl", "responseTimeoutMS", 1000u);

  _motorAddresses = cfg.getRequired<std::map<utl::EMotor, int>>(
      "MotorControl", "motorAddresses");
}

void MotorControl::initialize() {
  _motors.clear();
  try {
    auto busRes = ModbusClient::rtu(_rtuConfig.device, _rtuConfig.baud,
                                    _rtuConfig.parity, _rtuConfig.dataBits,
                                    _rtuConfig.stopBits, 1);
    if (!busRes) {
      auto msg = std::format("Failed to create RTU bus client: {}",
                             busRes.error().message);
      throw std::runtime_error(msg);
    }
    _bus.emplace(std::move(*busRes));
    if (auto c = _bus->connect(); !c) {
      auto msg = std::format("Failed to connect RTU bus on '{}': {}",
                             _rtuConfig.device, c.error().message);
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
      {
        std::lock_guard<std::mutex> lock(_busMutex);
        it->second.initialize(*_bus);
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
  setState(State::Error);
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
