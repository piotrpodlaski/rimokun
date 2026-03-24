#include <MotorControl.hpp>
#include <ExceptionUtils.hpp>

#include <ArKd2RegisterMap.hpp>
#include <Config.hpp>
#include <Logger.hpp>
#include <TimingMetrics.hpp>
#include <magic_enum/magic_enum.hpp>
#include <cstdlib>
#include <chrono>
#include <format>
#include <string_view>
#include <stdexcept>

namespace {
const Motor& requireMotor(const std::map<utl::EMotor, Motor>& motors,
                          const utl::EMotor motorId) {
  const auto it = motors.find(motorId);
  if (it == motors.end()) {
    utl::throwRuntimeError(
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
    utl::throwRuntimeError(std::format(
        "MotorControl.motors.{}.{} must be in range {}..{} (got {})",
        motorName, field, kMinCurrent, kMaxCurrent, current));
  }
}

bool isTemporaryCommunicationFailure(const std::string_view message) {
  return message.find("Resource temporarily unavailable") != std::string_view::npos;
}
}  // namespace

MotorControl::MotorControl() {
  auto& cfg = utl::Config::instance();
  const auto model =
      cfg.getOptional<std::string>("MotorControl", "model", "AR-KD2");
  if (model == "AR-KD2") {
    _registerMap = makeArKd2RegisterMap();
  } else {
    utl::throwRuntimeError(
        std::format("Unsupported MotorControl model '{}'", model));
  }
  _rtuConfig.device.clear();
  _rtuConfig.baud = 9600;
  _rtuConfig.parity = 'N';
  _rtuConfig.dataBits = 8;
  _rtuConfig.stopBits = 1;
  _rtuConfig.responseTimeoutMS =
      cfg.getOptional<unsigned>("MotorControl", "responseTimeoutMS", 1000u);
  _rtuConfig.connectTimeoutMS =
      cfg.getOptional<unsigned>("MotorControl", "connectTimeoutMS", 1000u);
  _rtuConfig.interRequestDelayMS =
      cfg.getOptional<unsigned>("MotorControl", "interRequestDelayMS", 0u);
  const auto globalForceFunction10ForSingleRegisterWrites =
      cfg.getOptional<bool>("MotorControl",
                            "forceFunction10ForSingleRegisterWrites", false);

  const auto motorCfg = cfg.getClassConfig("MotorControl");
  const auto transportCfg = motorCfg["transport"];
  if (!transportCfg || !transportCfg.IsMap()) {
    utl::throwRuntimeError(
        "MotorControl.transport map is required (type + tcp/serial settings).");
  }
  const auto type = transportCfg["type"].as<std::string>("rawTcpRtu");
  if (type == "rawTcpRtu") {
    _transportType = TransportType::RawTcpRtu;
    const auto tcpCfg = transportCfg["tcp"];
    if (!tcpCfg || !tcpCfg.IsMap()) {
      utl::throwRuntimeError(
          "MotorControl.transport.tcp is required for rawTcpRtu transport.");
    }
    _rawTcpConfig.host = tcpCfg["host"].as<std::string>();
    _rawTcpConfig.port = tcpCfg["port"].as<int>();
  } else if (type == "serialRtu") {
    _transportType = TransportType::SerialRtu;
    const auto serialCfg = transportCfg["serial"];
    if (!serialCfg || !serialCfg.IsMap()) {
      utl::throwRuntimeError(
          "MotorControl.transport.serial is required for serialRtu transport.");
    }
    _rtuConfig.device = serialCfg["device"].as<std::string>();
    _rtuConfig.baud = serialCfg["baud"].as<int>(9600);
    const auto parity = serialCfg["parity"].as<std::string>("N");
    _rtuConfig.parity = parity.empty() ? 'N' : parity.front();
    _rtuConfig.dataBits = serialCfg["dataBits"].as<int>(8);
    _rtuConfig.stopBits = serialCfg["stopBits"].as<int>(1);
  } else {
    utl::throwRuntimeError(
        std::format("Unsupported MotorControl transport type '{}'", type));
  }

  _motorConfigs.clear();
  const auto motorsNode = motorCfg["motors"];
  if (!motorsNode || !motorsNode.IsMap()) {
    utl::throwRuntimeError(
        "MotorControl.motors map is required (per motor: address, optional commandAddress, optional groupId, runCurrent, stopCurrent, and optional startup thresholds).");
  }
  for (const auto& kv : motorsNode) {
    const auto motorId = kv.first.as<utl::EMotor>();
    const auto entry = kv.second;
    if (!entry || !entry.IsMap()) {
      utl::throwRuntimeError(std::format(
          "MotorControl.motors.{} must be a map with at least 'address'.",
          magic_enum::enum_name(motorId)));
    }
    const auto address = entry["address"].as<int>();
    if (address < 0 || address > 247) {
      utl::throwRuntimeError(std::format(
          "MotorControl.motors.{}.address must be in range 0..247 (got {})",
          magic_enum::enum_name(motorId), address));
    }
    std::optional<int> commandAddress;
    if (const auto commandAddressNode = entry["commandAddress"]; commandAddressNode) {
      commandAddress = commandAddressNode.as<int>();
      if (*commandAddress < 1 || *commandAddress > 247) {
        utl::throwRuntimeError(std::format(
            "MotorControl.motors.{}.commandAddress must be in range 1..247 (got {})",
            magic_enum::enum_name(motorId), *commandAddress));
      }
    }
    const auto groupIdNode = entry["groupId"];
    const auto runCurrent =
        entry["runCurrent"].as<std::int32_t>(kDefaultRunCurrent);
    const auto stopCurrent =
        entry["stopCurrent"].as<std::int32_t>(kDefaultStopCurrent);
    const auto startingSpeedNode = entry["startingSpeed"];
    const auto overloadWarningNode = entry["overloadWarning"];
    const auto overloadAlarmNode = entry["overloadAlarm"];
    const auto excessivePositionDeviationWarningNode =
        entry["excessivePositionDeviationWarning"];
    const auto excessivePositionDeviationAlarmNode =
        entry["excessivePositionDeviationAlarm"];
    const auto motorRotationDirectionNode = entry["motorRotationDirection"];
    validateCurrentRange(runCurrent, "runCurrent", magic_enum::enum_name(motorId));
    validateCurrentRange(stopCurrent, "stopCurrent",
                         magic_enum::enum_name(motorId));
    if (stopCurrent > runCurrent) {
      SPDLOG_WARN(
          "MotorControl.motors.{}: stopCurrent ({}) > runCurrent ({}). "
          "Verify this is intentional.",
          magic_enum::enum_name(motorId), stopCurrent, runCurrent);
    }
    std::optional<std::int32_t> groupId;
    std::optional<bool> forceFunction10ForSingleRegisterWrites;
    if (groupIdNode) {
      groupId = groupIdNode.as<std::int32_t>();
    }
    if (const auto forceNode = entry["forceFunction10ForSingleRegisterWrites"];
        forceNode) {
      forceFunction10ForSingleRegisterWrites = forceNode.as<bool>();
    }
    std::optional<std::int32_t> startingSpeed;
    std::optional<std::int32_t> overloadWarning;
    std::optional<std::int32_t> overloadAlarm;
    std::optional<std::int32_t> excessivePositionDeviationWarning;
    std::optional<std::int32_t> excessivePositionDeviationAlarm;
    std::optional<std::int32_t> motorRotationDirection;
    const auto motorName = magic_enum::enum_name(motorId);
    if (startingSpeedNode) {
      startingSpeed = startingSpeedNode.as<std::int32_t>();
      if (*startingSpeed < 0) {
        utl::throwRuntimeError(std::format(
            "MotorControl.motors.{}.startingSpeed must be >= 0 (got {})",
            motorName, *startingSpeed));
      }
    }
    if (overloadWarningNode) {
      overloadWarning = overloadWarningNode.as<std::int32_t>();
      if (*overloadWarning < 0) {
        utl::throwRuntimeError(std::format(
            "MotorControl.motors.{}.overloadWarning must be >= 0 (got {})",
            motorName, *overloadWarning));
      }
    }
    if (overloadAlarmNode) {
      overloadAlarm = overloadAlarmNode.as<std::int32_t>();
      if (*overloadAlarm < 0) {
        utl::throwRuntimeError(std::format(
            "MotorControl.motors.{}.overloadAlarm must be >= 0 (got {})",
            motorName, *overloadAlarm));
      }
    }
    if (overloadWarning && overloadAlarm && *overloadAlarm < *overloadWarning) {
      utl::throwRuntimeError(std::format(
          "MotorControl.motors.{}: overloadAlarm ({}) must be >= overloadWarning ({})",
          motorName, *overloadAlarm, *overloadWarning));
    }
    if (excessivePositionDeviationWarningNode) {
      excessivePositionDeviationWarning =
          excessivePositionDeviationWarningNode.as<std::int32_t>();
      if (*excessivePositionDeviationWarning < 0) {
        utl::throwRuntimeError(std::format(
            "MotorControl.motors.{}.excessivePositionDeviationWarning must be >= 0 (got {})",
            motorName, *excessivePositionDeviationWarning));
      }
    }
    if (excessivePositionDeviationAlarmNode) {
      excessivePositionDeviationAlarm =
          excessivePositionDeviationAlarmNode.as<std::int32_t>();
      if (*excessivePositionDeviationAlarm < 0) {
        utl::throwRuntimeError(std::format(
            "MotorControl.motors.{}.excessivePositionDeviationAlarm must be >= 0 (got {})",
            motorName, *excessivePositionDeviationAlarm));
      }
    }
    if (excessivePositionDeviationWarning && excessivePositionDeviationAlarm &&
        *excessivePositionDeviationAlarm < *excessivePositionDeviationWarning) {
      utl::throwRuntimeError(std::format(
          "MotorControl.motors.{}: excessivePositionDeviationAlarm ({}) must be >= "
          "excessivePositionDeviationWarning ({})",
          motorName, *excessivePositionDeviationAlarm,
          *excessivePositionDeviationWarning));
    }
    if (motorRotationDirectionNode) {
      motorRotationDirection = motorRotationDirectionNode.as<std::int32_t>();
    }
    _motorConfigs[motorId] = MotorConfig{
        .address = address,
        .commandAddress = commandAddress,
        .groupId = groupId,
        .forceFunction10ForSingleRegisterWrites =
            forceFunction10ForSingleRegisterWrites
                .value_or(globalForceFunction10ForSingleRegisterWrites),
        .runCurrent = runCurrent,
        .stopCurrent = stopCurrent,
        .startingSpeed = startingSpeed,
        .overloadWarning = overloadWarning,
        .overloadAlarm = overloadAlarm,
        .excessivePositionDeviationWarning = excessivePositionDeviationWarning,
        .excessivePositionDeviationAlarm = excessivePositionDeviationAlarm,
        .motorRotationDirection = motorRotationDirection};
  }
}

std::vector<utl::EMotor> MotorControl::configuredMotorIds() const {
  std::vector<utl::EMotor> ids;
  ids.reserve(_motorConfigs.size());
  for (const auto& [motorId, _] : _motorConfigs) {
    ids.push_back(motorId);
  }
  return ids;
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
      utl::throwRuntimeError(msg);
    }
    _bus.emplace(std::move(*busRes));
    if (auto ct = _bus->set_connect_timeout(
            std::chrono::milliseconds{_rtuConfig.connectTimeoutMS});
        !ct) {
      auto msg = std::format("Failed to set motor bus connect timeout: {}",
                             ct.error().message);
      _bus.reset();
      utl::throwRuntimeError(msg);
    }
    if (auto c = _bus->connect(); !c) {
      const auto endpoint =
          _transportType == TransportType::SerialRtu
              ? _rtuConfig.device
              : std::format("{}:{}", _rawTcpConfig.host, _rawTcpConfig.port);
      auto msg = std::format("Failed to connect motor bus on '{}': {}",
                             endpoint, c.error().message);
      _bus.reset();
      utl::throwRuntimeError(msg);
    }
    if (auto t = _bus->set_response_timeout(
            std::chrono::milliseconds{_rtuConfig.responseTimeoutMS});
        !t) {
      auto msg = std::format("Failed to set RTU bus timeout: {}",
                             t.error().message);
      _bus->close();
      _bus.reset();
      utl::throwRuntimeError(msg);
    }
    if (auto d = _bus->set_inter_request_delay(
            std::chrono::milliseconds{_rtuConfig.interRequestDelayMS});
        !d) {
      auto msg = std::format("Failed to set inter-request delay: {}",
                             d.error().message);
      _bus->close();
      _bus.reset();
      utl::throwRuntimeError(msg);
    }

    for (const auto& [motorId, motorCfg] : _motorConfigs) {
      auto [it, inserted] = _motors.emplace(
          motorId,
          Motor(motorId, motorCfg.address, _registerMap,
                motorCfg.commandAddress.value_or(motorCfg.address),
                motorCfg.forceFunction10ForSingleRegisterWrites.value_or(false)));
      (void)inserted;
      _runtime.emplace(motorId, MotorRuntimeState{});
      {
        std::lock_guard<std::mutex> lock(_busMutex);
        it->second.initialize(*_bus);
        applyConfiguredParameters(it->second, motorCfg, *_bus);
        const auto inputRaw = it->second.readDriverInputCommandRaw(*_bus);
        const auto outputRaw = it->second.readDriverOutputStatusRaw(*_bus);
        const bool hasAlarm =
            Motor::isDriverOutputFlagSet(outputRaw, MotorOutputFlag::Alarm);
        const auto remoteIoStatus =
            it->second.decodeRemoteIoStatus(inputRaw, outputRaw);
        const auto cOnIt = std::find_if(
            remoteIoStatus.inputAssignments.begin(),
            remoteIoStatus.inputAssignments.end(),
            [](const auto& assignment) { return assignment.functionCode == 17u; });
        const bool cOnDefined =
            cOnIt != remoteIoStatus.inputAssignments.end();
        const bool cOnActive = cOnDefined ? cOnIt->active : true;
        auto runtimeIt = _runtime.find(motorId);
        runtimeIt->second.enableControllable = cOnDefined;
        // Initialize runtime enabled state from actual startup C-ON + alarm state.
        runtimeIt->second.enabled = !hasAlarm && cOnActive;
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
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");

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
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  runtime.speed = std::abs(speed);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");

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

void MotorControl::setAcceleration(const utl::EMotor motorId,
                                   const std::int32_t acceleration) {
  RIMO_TIMED_SCOPE("MotorControl::setAcceleration");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  if (runtime.acceleration == acceleration) {
    return;
  }
  runtime.acceleration = acceleration;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");

  if (runtime.speedPairPrepared) {
    motor.setOperationAcceleration(*_bus, 0, runtime.acceleration);
    motor.setOperationAcceleration(*_bus, 1, runtime.acceleration);
  }
  if (runtime.positionPrepared) {
    motor.setOperationAcceleration(*_bus, 2, runtime.acceleration);
  }
}

void MotorControl::setDeceleration(const utl::EMotor motorId,
                                   const std::int32_t deceleration) {
  RIMO_TIMED_SCOPE("MotorControl::setDeceleration");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  if (runtime.deceleration == deceleration) {
    return;
  }
  runtime.deceleration = deceleration;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");

  if (runtime.speedPairPrepared) {
    motor.setOperationDeceleration(*_bus, 0, runtime.deceleration);
    motor.setOperationDeceleration(*_bus, 1, runtime.deceleration);
  }
  if (runtime.positionPrepared) {
    motor.setOperationDeceleration(*_bus, 2, runtime.deceleration);
  }
}

void MotorControl::setPosition(const utl::EMotor motorId,
                               const std::int32_t position) {
  RIMO_TIMED_SCOPE("MotorControl::setPosition");
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  runtime.position = position;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");

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
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  runtime.direction = direction;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");

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
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  auto& runtime = rtIt->second;
  if (!runtime.enabled) {
    SPDLOG_WARN("Ignoring startMovement for disabled motor {}",
                magic_enum::enum_name(motorId));
    return;
  }
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");

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
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  const auto& runtime = rtIt->second;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  if (runtime.mode == MotorControlMode::Speed) {
    // AR-KD2 speed mode: stop by deasserting direction bits only.
    // Do not clear the whole register because that would overwrite C-ON and other inputs.
    motor.setForward(*_bus, false);
    motor.setReverse(*_bus, false);
    return;
  }
  motor.pulseStop(*_bus);
}

void MotorControl::pulseStart(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  const auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  if (!rtIt->second.enabled) {
    SPDLOG_WARN("Ignoring pulseStart for disabled motor {}",
                magic_enum::enum_name(motorId));
    return;
  }
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.pulseStart(*_bus);
}

void MotorControl::pulseStop(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.pulseStop(*_bus);
}

void MotorControl::pulseHome(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.pulseHome(*_bus);
}

void MotorControl::setForward(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  const auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  if (!rtIt->second.enabled) {
    SPDLOG_WARN("Ignoring setForward for disabled motor {}",
                magic_enum::enum_name(motorId));
    return;
  }
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setForward(*_bus, enabled);
}

void MotorControl::setReverse(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  const auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  if (!rtIt->second.enabled) {
    SPDLOG_WARN("Ignoring setReverse for disabled motor {}",
                magic_enum::enum_name(motorId));
    return;
  }
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setReverse(*_bus, enabled);
}

void MotorControl::setJogPlus(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  const auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  if (!rtIt->second.enabled) {
    SPDLOG_WARN("Ignoring setJogPlus for disabled motor {}",
                magic_enum::enum_name(motorId));
    return;
  }
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setJogPlus(*_bus, enabled);
}

void MotorControl::setJogMinus(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  const auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  if (!rtIt->second.enabled) {
    SPDLOG_WARN("Ignoring setJogMinus for disabled motor {}",
                magic_enum::enum_name(motorId));
    return;
  }
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setJogMinus(*_bus, enabled);
}

void MotorControl::setEnabled(const utl::EMotor motorId, const bool enabled) {
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        magic_enum::enum_name(motorId)));
  }
  if (!rtIt->second.enableControllable) {
    SPDLOG_WARN(
        "Ignoring enable change for motor {} because C-ON mapping is unavailable",
        magic_enum::enum_name(motorId));
    return;
  }
  if (rtIt->second.enabled == enabled) {
    return;
  }
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setEnabled(*_bus, enabled);
  rtIt->second.enabled = enabled;
}

void MotorControl::setAllEnabled(const bool enabled) {
  for (const auto& [motorId, _] : _motors) {
    setEnabled(motorId, enabled);
  }
}

void MotorControl::onAlarmCleared(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    return;
  }
  SPDLOG_INFO("Motor {} alarm cleared — forcing C-ON=0 for safe recovery",
              magic_enum::enum_name(motorId));
  motor.invalidateDriverInputCommandCache();
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) {
    rtIt->second.enabled = false;
    return;
  }
  motor.setEnabled(*_bus, false);
  rtIt->second.enabled = false;
}

bool MotorControl::isEnabled(const utl::EMotor motorId) const {
  const auto it = _runtime.find(motorId);
  if (it == _runtime.end()) {
    return false;
  }
  return it->second.enabled;
}

bool MotorControl::isEnableControllable(const utl::EMotor motorId) const {
  const auto it = _runtime.find(motorId);
  if (it == _runtime.end()) {
    return false;
  }
  return it->second.enableControllable;
}

std::uint8_t MotorControl::readSelectedOperationId(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
    return motor.readSelectedOperationId(*_bus);
  } catch (const std::exception& ex) {
    handleCommunicationFailure("readSelectedOperationId", ex);
    throw;
  }
}

void MotorControl::resetAlarm(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
    motor.resetAlarm(*_bus);
    const auto cfgIt = _motorConfigs.find(motorId);
    if (cfgIt != _motorConfigs.end()) {
      applyConfiguredParameters(motor, cfgIt->second, *_bus);
    }
  } catch (const std::exception& ex) {
    handleCommunicationFailure("resetAlarm", ex);
    throw;
  }
}

void MotorControl::setSelectedOperationId(const utl::EMotor motorId,
                                          const std::uint8_t opId) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setSelectedOperationId(*_bus, opId);
}

void MotorControl::setOperationMode(const utl::EMotor motorId,
                                    const std::uint8_t opId,
                                    const MotorOperationMode mode) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setOperationMode(*_bus, opId, mode);
}

void MotorControl::setOperationFunction(const utl::EMotor motorId,
                                        const std::uint8_t opId,
                                        const MotorOperationFunction function) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setOperationFunction(*_bus, opId, function);
}

void MotorControl::setOperationPosition(const utl::EMotor motorId,
                                        const std::uint8_t opId,
                                        const std::int32_t position) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setOperationPosition(*_bus, opId, position);
}

void MotorControl::setOperationSpeed(const utl::EMotor motorId,
                                     const std::uint8_t opId,
                                     const std::int32_t speed) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setOperationSpeed(*_bus, opId, speed);
}

void MotorControl::setOperationAcceleration(const utl::EMotor motorId,
                                            const std::uint8_t opId,
                                            const std::int32_t acceleration) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setOperationAcceleration(*_bus, opId, acceleration);
}

void MotorControl::setOperationDeceleration(const utl::EMotor motorId,
                                            const std::uint8_t opId,
                                            const std::int32_t deceleration) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setOperationDeceleration(*_bus, opId, deceleration);
}

void MotorControl::setRunCurrent(const utl::EMotor motorId,
                                 const std::int32_t current) {
  validateCurrentRange(current, "runCurrent", magic_enum::enum_name(motorId));
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setRunCurrent(*_bus, current);
}

void MotorControl::setStopCurrent(const utl::EMotor motorId,
                                  const std::int32_t current) {
  validateCurrentRange(current, "stopCurrent", magic_enum::enum_name(motorId));
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.setStopCurrent(*_bus, current);
}

void MotorControl::configureConstantSpeedPair(const utl::EMotor motorId,
                                              const std::int32_t speedOp0,
                                              const std::int32_t speedOp1,
                                              const std::int32_t acceleration,
                                              const std::int32_t deceleration) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.configureConstantSpeedPair(*_bus, speedOp0, speedOp1, acceleration,
                                   deceleration);
}

void MotorControl::updateConstantSpeedBuffered(const utl::EMotor motorId,
                                               const std::int32_t speed) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  motor.updateConstantSpeedBuffered(*_bus, speed);
}

MotorFlagStatus MotorControl::readInputStatus(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
    return motor.decodeDriverInputStatus(motor.readDriverInputCommandRaw(*_bus));
  } catch (const std::exception& ex) {
    handleCommunicationFailure("readInputStatus", ex);
    throw;
  }
}

MotorFlagStatus MotorControl::readOutputStatus(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
    return motor.decodeDriverOutputStatus(motor.readDriverOutputStatusRaw(*_bus));
  } catch (const std::exception& ex) {
    handleCommunicationFailure("readOutputStatus", ex);
    throw;
  }
}

MotorDirectIoStatus MotorControl::readDirectIoStatus(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
    return motor.decodeDirectIoAndBrakeStatus(
        motor.readDirectIoAndBrakeStatusRaw(*_bus));
  } catch (const std::exception& ex) {
    handleCommunicationFailure("readDirectIoStatus", ex);
    throw;
  }
}

MotorMonitorSnapshot MotorControl::readMonitorSnapshot(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
    return motor.readMonitorSnapshot(*_bus);
  } catch (const std::exception& ex) {
    handleCommunicationFailure("readMonitorSnapshot", ex);
    throw;
  }
}

MotorRemoteIoStatus MotorControl::readRemoteIoStatus(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
    return motor.decodeRemoteIoStatus(motor.readDriverInputCommandRaw(*_bus),
                                      motor.readDriverOutputStatusRaw(*_bus));
  } catch (const std::exception& ex) {
    handleCommunicationFailure("readRemoteIoStatus", ex);
    throw;
  }
}

bool MotorControl::hasAnyWarningOrAlarm() {
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) {
      utl::throwRuntimeError("MotorControl bus is not initialized");
    }
    for (const auto& [_, motor] : _motors) {
      const auto raw = motor.readDriverOutputStatusRaw(*_bus);
      if (Motor::isDriverOutputFlagSet(raw, MotorOutputFlag::Warning) ||
          Motor::isDriverOutputFlagSet(raw, MotorOutputFlag::Alarm)) {
        return true;
      }
    }
    return false;
  } catch (const std::exception& ex) {
    handleCommunicationFailure("hasAnyWarningOrAlarm", ex);
    throw;
  }
}

void MotorControl::setWarningState(const bool warningActive) {
  if (state() == State::Error) {
    return;
  }
  setState(warningActive ? State::Warning : State::Normal);
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentAlarm(
    const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) {
      utl::throwRuntimeError("MotorControl bus is not initialized");
    }
    return motor.diagnoseAlarm(motor.readAlarmCode(*_bus));
  } catch (const std::exception& ex) {
    handleCommunicationFailure("diagnoseCurrentAlarm", ex);
    throw;
  }
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentWarning(
    const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) {
      utl::throwRuntimeError("MotorControl bus is not initialized");
    }
    return motor.diagnoseWarning(motor.readWarningCode(*_bus));
  } catch (const std::exception& ex) {
    handleCommunicationFailure("diagnoseCurrentWarning", ex);
    throw;
  }
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentCommunicationError(
    const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) {
      utl::throwRuntimeError("MotorControl bus is not initialized");
    }
    return motor.diagnoseCommunicationError(
        motor.readCommunicationErrorCode(*_bus));
  } catch (const std::exception& ex) {
    handleCommunicationFailure("diagnoseCurrentCommunicationError", ex);
    throw;
  }
}

std::int32_t MotorControl::readGroupId(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  try {
    std::lock_guard<std::mutex> lock(_busMutex);
    if (!_bus) {
      utl::throwRuntimeError("MotorControl bus is not initialized");
    }
    return motor.readGroupId(*_bus);
  } catch (const std::exception& ex) {
    handleCommunicationFailure("readGroupId", ex);
    throw;
  }
}

void MotorControl::applyConfiguredParameters(const Motor& motor,
                                             const MotorConfig& config,
                                             ModbusClient& bus) const {
  if (config.groupId.has_value()) {
    motor.setGroupId(bus, *config.groupId);
  }
  // Fixed startup policy: STOP input triggers deceleration and current-off.
  // This is intentionally hardcoded for all active motors, not config-driven.
  motor.setStopInputAction(bus, 3);
  motor.setRunCurrent(bus, config.runCurrent);
  motor.setStopCurrent(bus, config.stopCurrent);
  if (config.startingSpeed.has_value()) {
    motor.setStartingSpeed(bus, *config.startingSpeed);
  }
  if (config.overloadWarning.has_value()) {
    motor.setOverloadWarning(bus, *config.overloadWarning);
  }
  if (config.overloadAlarm.has_value()) {
    motor.setOverloadAlarm(bus, *config.overloadAlarm);
  }
  if (config.excessivePositionDeviationWarning.has_value()) {
    motor.setExcessivePositionDeviationWarning(
        bus, *config.excessivePositionDeviationWarning);
  }
  if (config.excessivePositionDeviationAlarm.has_value()) {
    motor.setExcessivePositionDeviationAlarm(
        bus, *config.excessivePositionDeviationAlarm);
  }
  if (config.motorRotationDirection.has_value()) {
    motor.setMotorRotationDirection(bus, *config.motorRotationDirection);
    motor.executeConfiguration(bus);
  }
}

void MotorControl::handleCommunicationFailure(const std::string_view action,
                                              const std::exception& ex) {
  if (!isTemporaryCommunicationFailure(ex.what())) {
    return;
  }
  SPDLOG_ERROR("MotorControl communication failure during {}: {}", action,
               ex.what());
  if (_bus.has_value()) {
    _bus->close();
    _bus.reset();
  }
  setState(State::Error);
}
