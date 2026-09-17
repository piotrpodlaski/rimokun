#include <MotorControl.hpp>
#include <ExceptionUtils.hpp>

#include <ArKd2RegisterMap.hpp>
#include <Config.hpp>
#include <Logger.hpp>
#include <TimingMetrics.hpp>
#include <EnumStrings.hpp>
#include <cstdlib>
#include <chrono>
#include <format>
#include <string_view>
#include <stdexcept>
#include <utility>

namespace {
const Motor& requireMotor(const std::map<utl::EMotor, Motor>& motors,
                          const utl::EMotor motorId) {
  const auto it = motors.find(motorId);
  if (it == motors.end()) {
    utl::throwRuntimeError(
        std::format("Motor {} is not configured in MotorControl",
                    utl::enumToStringView(motorId)));
  }
  return it->second;
}

}  // namespace

template <typename F>
decltype(auto) MotorControl::withBus(const utl::EMotor motorId, F&& fn) {
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    utl::throwRuntimeError(std::format(
        "Runtime state for motor {} is not available",
        utl::enumToStringView(motorId)));
  }
  auto& runtime = rtIt->second;
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  return std::forward<F>(fn)(motor, runtime, *_bus);
}

template <typename F>
decltype(auto) MotorControl::withMotor(const utl::EMotor motorId, F&& fn) {
  const auto& motor = requireMotor(_motors, motorId);
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  return std::forward<F>(fn)(motor, *_bus);
}

template <typename F>
decltype(auto) MotorControl::withMotorHandled(const std::string_view action,
                                              const utl::EMotor motorId,
                                              F&& fn) {
  try {
    return withMotor(motorId, std::forward<F>(fn));
  } catch (const std::exception& ex) {
    handleCommunicationFailure(action, ex);
    throw;
  }
}

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
          utl::enumToStringView(motorId)));
    }
    const auto address = entry["address"].as<int>();
    if (address < 0 || address > 247) {
      utl::throwRuntimeError(std::format(
          "MotorControl.motors.{}.address must be in range 0..247 (got {})",
          utl::enumToStringView(motorId), address));
    }
    std::optional<int> commandAddress;
    if (const auto commandAddressNode = entry["commandAddress"]; commandAddressNode) {
      commandAddress = commandAddressNode.as<int>();
      if (*commandAddress < 1 || *commandAddress > 247) {
        utl::throwRuntimeError(std::format(
            "MotorControl.motors.{}.commandAddress must be in range 1..247 (got {})",
            utl::enumToStringView(motorId), *commandAddress));
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
    validateCurrentRange(runCurrent, "runCurrent", utl::enumToStringView(motorId));
    validateCurrentRange(stopCurrent, "stopCurrent",
                         utl::enumToStringView(motorId));
    if (stopCurrent > runCurrent) {
      SPDLOG_WARN(
          "MotorControl.motors.{}: stopCurrent ({}) > runCurrent ({}). "
          "Verify this is intentional.",
          utl::enumToStringView(motorId), stopCurrent, runCurrent);
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
    const auto motorName = utl::enumToStringView(motorId);
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

  _freeControllableMotors.clear();
  if (const auto freeMotorsNode = motorCfg["freeControllableMotors"];
      freeMotorsNode) {
    if (!freeMotorsNode.IsSequence()) {
      utl::throwRuntimeError(
          "MotorControl.freeControllableMotors must be a list of motor names.");
    }
    for (const auto& motorNode : freeMotorsNode) {
      const auto motorId = motorNode.as<utl::EMotor>();
      if (!_motorConfigs.contains(motorId)) {
        utl::throwRuntimeError(std::format(
            "MotorControl.freeControllableMotors contains {} which is not configured.",
            utl::enumToStringView(motorId)));
      }
      _freeControllableMotors.insert(motorId);
    }
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

bool MotorControl::isGroupMember(const utl::EMotor motorId) const {
  const auto it = _motorConfigs.find(motorId);
  return it != _motorConfigs.end() && it->second.groupId.has_value();
}

bool MotorControl::isFreeControllable(const utl::EMotor motorId) const {
  return _freeControllableMotors.contains(motorId);
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
        applyConfiguredParameters(it->second, motorCfg, *_bus);
        it->second.initialize(*_bus);
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
  withBus(motorId, [this, mode](const Motor& motor, MotorRuntimeState& runtime,
                                ModbusClient& bus) {
    runtime.mode = mode;
    if (mode == MotorControlMode::Speed) {
      prepareSpeedMode(motor, runtime, bus);
    } else {
      preparePositionMode(motor, runtime, bus);
    }
  });
}

void MotorControl::setSpeed(const utl::EMotor motorId, const std::int32_t speed) {
  RIMO_TIMED_SCOPE("MotorControl::setSpeed");
  withBus(motorId, [this, speed](const Motor& motor, MotorRuntimeState& runtime,
                                 ModbusClient& bus) {
    runtime.speed = std::abs(speed);
    if (runtime.mode == MotorControlMode::Speed) {
      if (runtime.speedPairPrepared) {
        motor.updateConstantSpeedBuffered(bus, runtime.speed);
      } else {
        prepareSpeedMode(motor, runtime, bus);
      }
    }
  });
}

void MotorControl::setAcceleration(const utl::EMotor motorId,
                                   const std::int32_t acceleration) {
  RIMO_TIMED_SCOPE("MotorControl::setAcceleration");
  withBus(motorId, [acceleration](const Motor& motor, MotorRuntimeState& runtime,
                                  ModbusClient& bus) {
    if (runtime.acceleration == acceleration) return;
    runtime.acceleration = acceleration;
    if (runtime.speedPairPrepared) {
      motor.setOperationAcceleration(bus, 0, runtime.acceleration);
      motor.setOperationAcceleration(bus, 1, runtime.acceleration);
    }
    if (runtime.positionPrepared) {
      motor.setOperationAcceleration(bus, 2, runtime.acceleration);
    }
  });
}

void MotorControl::setDeceleration(const utl::EMotor motorId,
                                   const std::int32_t deceleration) {
  RIMO_TIMED_SCOPE("MotorControl::setDeceleration");
  withBus(motorId, [deceleration](const Motor& motor, MotorRuntimeState& runtime,
                                  ModbusClient& bus) {
    if (runtime.deceleration == deceleration) return;
    runtime.deceleration = deceleration;
    if (runtime.speedPairPrepared) {
      motor.setOperationDeceleration(bus, 0, runtime.deceleration);
      motor.setOperationDeceleration(bus, 1, runtime.deceleration);
    }
    if (runtime.positionPrepared) {
      motor.setOperationDeceleration(bus, 2, runtime.deceleration);
    }
  });
}

void MotorControl::setPosition(const utl::EMotor motorId,
                               const std::int32_t position) {
  RIMO_TIMED_SCOPE("MotorControl::setPosition");
  withBus(motorId, [this, position](const Motor& motor, MotorRuntimeState& runtime,
                                    ModbusClient& bus) {
    runtime.position = position;
    if (runtime.mode == MotorControlMode::Position) {
      preparePositionMode(motor, runtime, bus);
      motor.setOperationPosition(bus, 2, runtime.position);
    }
  });
}

void MotorControl::setDirection(const utl::EMotor motorId,
                                const MotorControlDirection direction) {
  RIMO_TIMED_SCOPE("MotorControl::setDirection");
  withBus(motorId, [direction](const Motor& motor, MotorRuntimeState& runtime,
                               ModbusClient& bus) {
    runtime.direction = direction;
    if (direction == MotorControlDirection::Forward) {
      motor.setReverse(bus, false);
      motor.setForward(bus, true);
    } else {
      motor.setForward(bus, false);
      motor.setReverse(bus, true);
    }
  });
}

void MotorControl::startMovement(const utl::EMotor motorId) {
  RIMO_TIMED_SCOPE("MotorControl::startMovement");
  withBus(motorId, [this, motorId](const Motor& motor,
                                   MotorRuntimeState& runtime,
                                   ModbusClient& bus) {
    if (!runtime.enabled) {
      SPDLOG_WARN("Ignoring startMovement for disabled motor {}",
                  utl::enumToStringView(motorId));
      return;
    }
    if (runtime.mode == MotorControlMode::Speed) {
      prepareSpeedMode(motor, runtime, bus);
      if (runtime.direction == MotorControlDirection::Forward) {
        motor.setReverse(bus, false);
        motor.setForward(bus, true);
      } else {
        motor.setForward(bus, false);
        motor.setReverse(bus, true);
      }
      return;
    } else {
      preparePositionMode(motor, runtime, bus);
      motor.setOperationPosition(bus, 2, runtime.position);
      motor.setSelectedOperationId(bus, 2);
    }
    motor.pulseStart(bus);
  });
}

void MotorControl::stopMovement(const utl::EMotor motorId) {
  RIMO_TIMED_SCOPE("MotorControl::stopMovement");
  withBus(motorId, [](const Motor& motor, MotorRuntimeState& runtime,
                      ModbusClient& bus) {
    if (runtime.mode == MotorControlMode::Speed) {
      // AR-KD2 speed mode: stop by deasserting direction bits only.
      // Do not clear the whole register because that would overwrite C-ON and other inputs.
      motor.setForward(bus, false);
      motor.setReverse(bus, false);
      return;
    }
    motor.pulseStop(bus);
  });
}

void MotorControl::pulseStart(const utl::EMotor motorId) {
  withBus(motorId, [motorId](const Motor& motor, MotorRuntimeState& runtime,
                             ModbusClient& bus) {
    if (!runtime.enabled) {
      SPDLOG_WARN("Ignoring pulseStart for disabled motor {}",
                  utl::enumToStringView(motorId));
      return;
    }
    motor.pulseStart(bus);
  });
}

void MotorControl::pulseStop(const utl::EMotor motorId) {
  withBus(motorId, [](const Motor& motor, MotorRuntimeState&, ModbusClient& bus) {
    motor.pulseStop(bus);
  });
}

void MotorControl::pulseHome(const utl::EMotor motorId) {
  withBus(motorId, [](const Motor& motor, MotorRuntimeState&, ModbusClient& bus) {
    motor.pulseHome(bus);
  });
}

void MotorControl::setForward(const utl::EMotor motorId, const bool enabled) {
  withBus(motorId, [motorId, enabled](const Motor& motor,
                                      MotorRuntimeState& runtime,
                                      ModbusClient& bus) {
    if (!runtime.enabled) {
      SPDLOG_WARN("Ignoring setForward for disabled motor {}",
                  utl::enumToStringView(motorId));
      return;
    }
    motor.setForward(bus, enabled);
  });
}

void MotorControl::setReverse(const utl::EMotor motorId, const bool enabled) {
  withBus(motorId, [motorId, enabled](const Motor& motor,
                                      MotorRuntimeState& runtime,
                                      ModbusClient& bus) {
    if (!runtime.enabled) {
      SPDLOG_WARN("Ignoring setReverse for disabled motor {}",
                  utl::enumToStringView(motorId));
      return;
    }
    motor.setReverse(bus, enabled);
  });
}

void MotorControl::setJogPlus(const utl::EMotor motorId, const bool enabled) {
  withBus(motorId, [motorId, enabled](const Motor& motor,
                                      MotorRuntimeState& runtime,
                                      ModbusClient& bus) {
    if (!runtime.enabled) {
      SPDLOG_WARN("Ignoring setJogPlus for disabled motor {}",
                  utl::enumToStringView(motorId));
      return;
    }
    motor.setJogPlus(bus, enabled);
  });
}

void MotorControl::setJogMinus(const utl::EMotor motorId, const bool enabled) {
  withBus(motorId, [motorId, enabled](const Motor& motor,
                                      MotorRuntimeState& runtime,
                                      ModbusClient& bus) {
    if (!runtime.enabled) {
      SPDLOG_WARN("Ignoring setJogMinus for disabled motor {}",
                  utl::enumToStringView(motorId));
      return;
    }
    motor.setJogMinus(bus, enabled);
  });
}

void MotorControl::setFree(const utl::EMotor motorId, const bool enabled) {
  if (!isFreeControllable(motorId)) {
    utl::throwRuntimeError(std::format(
        "Motor {} is not allowed to enter or leave FREE mode by configuration.",
        utl::enumToStringView(motorId)));
  }
  withBus(motorId, [enabled](const Motor& motor, MotorRuntimeState&,
                              ModbusClient& bus) {
    motor.setFree(bus, enabled);
  });
}

void MotorControl::setEnabled(const utl::EMotor motorId, const bool enabled) {
  withBus(motorId, [motorId, enabled](const Motor& motor,
                                      MotorRuntimeState& runtime,
                                      ModbusClient& bus) {
    if (!runtime.enableControllable) {
      SPDLOG_WARN(
          "Ignoring enable change for motor {} because C-ON mapping is unavailable",
          utl::enumToStringView(motorId));
      return;
    }
    if (runtime.enabled == enabled) {
      return;
    }
    motor.setEnabled(bus, enabled);
    runtime.enabled = enabled;
  });
}

void MotorControl::setAllEnabled(const bool enabled) {
  std::lock_guard<std::mutex> lock(_busMutex);
  if (!_bus) utl::throwRuntimeError("MotorControl bus is not initialized");
  for (const auto& [motorId, motor] : _motors) {
    auto rtIt = _runtime.find(motorId);
    if (rtIt == _runtime.end()) {
      utl::throwRuntimeError(std::format(
          "Runtime state for motor {} is not available",
          utl::enumToStringView(motorId)));
    }
    if (!rtIt->second.enableControllable) {
      SPDLOG_WARN(
          "Ignoring enable change for motor {} because C-ON mapping is unavailable",
          utl::enumToStringView(motorId));
      continue;
    }
    if (rtIt->second.enabled == enabled) {
      continue;
    }
    motor.setEnabled(*_bus, enabled);
    rtIt->second.enabled = enabled;
  }
}

void MotorControl::onAlarmCleared(const utl::EMotor motorId) {
  const auto& motor = requireMotor(_motors, motorId);
  auto rtIt = _runtime.find(motorId);
  if (rtIt == _runtime.end()) {
    return;
  }
  SPDLOG_INFO("Motor {} alarm cleared — forcing C-ON=0 for safe recovery",
              utl::enumToStringView(motorId));
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
  return withMotorHandled("readSelectedOperationId", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.readSelectedOperationId(bus);
                          });
}

void MotorControl::resetAlarm(const utl::EMotor motorId) {
  withMotorHandled("resetAlarm", motorId,
                   [this, motorId](const Motor& motor, ModbusClient& bus) {
                     motor.resetAlarm(bus);
    const auto cfgIt = _motorConfigs.find(motorId);
    if (cfgIt != _motorConfigs.end()) {
                       applyConfiguredParameters(motor, cfgIt->second, bus);
    }
                   });
}

void MotorControl::setSelectedOperationId(const utl::EMotor motorId,
                                          const std::uint8_t opId) {
  withMotor(motorId, [opId](const Motor& motor, ModbusClient& bus) {
    motor.setSelectedOperationId(bus, opId);
  });
}

void MotorControl::setOperationMode(const utl::EMotor motorId,
                                    const std::uint8_t opId,
                                    const MotorOperationMode mode) {
  withMotor(motorId, [opId, mode](const Motor& motor, ModbusClient& bus) {
    motor.setOperationMode(bus, opId, mode);
  });
}

void MotorControl::setOperationFunction(const utl::EMotor motorId,
                                        const std::uint8_t opId,
                                        const MotorOperationFunction function) {
  withMotor(motorId, [opId, function](const Motor& motor, ModbusClient& bus) {
    motor.setOperationFunction(bus, opId, function);
  });
}

void MotorControl::setOperationPosition(const utl::EMotor motorId,
                                        const std::uint8_t opId,
                                        const std::int32_t position) {
  withMotor(motorId, [opId, position](const Motor& motor, ModbusClient& bus) {
    motor.setOperationPosition(bus, opId, position);
  });
}

void MotorControl::setOperationSpeed(const utl::EMotor motorId,
                                     const std::uint8_t opId,
                                     const std::int32_t speed) {
  withMotor(motorId, [opId, speed](const Motor& motor, ModbusClient& bus) {
    motor.setOperationSpeed(bus, opId, speed);
  });
}

void MotorControl::setOperationAcceleration(const utl::EMotor motorId,
                                            const std::uint8_t opId,
                                            const std::int32_t acceleration) {
  withMotor(motorId,
            [opId, acceleration](const Motor& motor, ModbusClient& bus) {
              motor.setOperationAcceleration(bus, opId, acceleration);
            });
}

void MotorControl::setOperationDeceleration(const utl::EMotor motorId,
                                            const std::uint8_t opId,
                                            const std::int32_t deceleration) {
  withMotor(motorId,
            [opId, deceleration](const Motor& motor, ModbusClient& bus) {
              motor.setOperationDeceleration(bus, opId, deceleration);
            });
}

void MotorControl::setRunCurrent(const utl::EMotor motorId,
                                 const std::int32_t current) {
  validateCurrentRange(current, "runCurrent", utl::enumToStringView(motorId));
  withMotor(motorId, [current](const Motor& motor, ModbusClient& bus) {
    motor.setRunCurrent(bus, current);
  });
}

void MotorControl::setStopCurrent(const utl::EMotor motorId,
                                  const std::int32_t current) {
  validateCurrentRange(current, "stopCurrent", utl::enumToStringView(motorId));
  withMotor(motorId, [current](const Motor& motor, ModbusClient& bus) {
    motor.setStopCurrent(bus, current);
  });
}

void MotorControl::configureHoming(const utl::EMotor motorId,
                                   const std::int32_t mode,
                                   const std::int32_t operatingSpeed,
                                   const std::int32_t acceleration,
                                   const std::int32_t startingSpeed,
                                   const std::int32_t startingDirection) {
  withMotor(motorId, [=](const Motor& motor, ModbusClient& bus) {
    motor.configureHoming(bus, mode, operatingSpeed, acceleration,
                          startingSpeed, startingDirection);
  });
}

void MotorControl::configureConstantSpeedPair(const utl::EMotor motorId,
                                              const std::int32_t speedOp0,
                                              const std::int32_t speedOp1,
                                              const std::int32_t acceleration,
                                              const std::int32_t deceleration) {
  withMotor(motorId, [=](const Motor& motor, ModbusClient& bus) {
    motor.configureConstantSpeedPair(bus, speedOp0, speedOp1, acceleration,
                                     deceleration);
  });
}

void MotorControl::updateConstantSpeedBuffered(const utl::EMotor motorId,
                                               const std::int32_t speed) {
  withMotor(motorId, [speed](const Motor& motor, ModbusClient& bus) {
    motor.updateConstantSpeedBuffered(bus, speed);
  });
}

MotorFlagStatus MotorControl::readInputStatus(const utl::EMotor motorId) {
  return withMotorHandled("readInputStatus", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.decodeDriverInputStatus(
                                motor.readDriverInputCommandRaw(bus));
                          });
}

MotorFlagStatus MotorControl::readOutputStatus(const utl::EMotor motorId) {
  return withMotorHandled("readOutputStatus", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.decodeDriverOutputStatus(
                                motor.readDriverOutputStatusRaw(bus));
                          });
}

MotorDirectIoStatus MotorControl::readDirectIoStatus(const utl::EMotor motorId) {
  return withMotorHandled("readDirectIoStatus", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.decodeDirectIoAndBrakeStatus(
                                motor.readDirectIoAndBrakeStatusRaw(bus));
                          });
}

MotorMonitorSnapshot MotorControl::readMonitorSnapshot(const utl::EMotor motorId) {
  return withMotorHandled("readMonitorSnapshot", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.readMonitorSnapshot(bus);
                          });
}

MotorRemoteIoStatus MotorControl::readRemoteIoStatus(const utl::EMotor motorId) {
  return withMotorHandled("readRemoteIoStatus", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.decodeRemoteIoStatus(
                                motor.readDriverInputCommandRaw(bus),
                                motor.readDriverOutputStatusRaw(bus));
                          });
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
  return withMotorHandled("diagnoseCurrentAlarm", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.diagnoseAlarm(motor.readAlarmCode(bus));
                          });
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentWarning(
    const utl::EMotor motorId) {
  return withMotorHandled("diagnoseCurrentWarning", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.diagnoseWarning(
                                motor.readWarningCode(bus));
                          });
}

MotorCodeDiagnostic MotorControl::diagnoseCurrentCommunicationError(
    const utl::EMotor motorId) {
  return withMotorHandled(
      "diagnoseCurrentCommunicationError", motorId,
      [](const Motor& motor, ModbusClient& bus) {
        return motor.diagnoseCommunicationError(
            motor.readCommunicationErrorCode(bus));
      });
}

std::int32_t MotorControl::readGroupId(const utl::EMotor motorId) {
  return withMotorHandled("readGroupId", motorId,
                          [](const Motor& motor, ModbusClient& bus) {
                            return motor.readGroupId(bus);
                          });
}

void MotorControl::prepareSpeedMode(const Motor& motor,
                                    MotorRuntimeState& runtime,
                                    ModbusClient& bus) {
  if (runtime.speedPairPrepared) {
    return;
  }
  motor.configureConstantSpeedPair(bus, runtime.speed, runtime.speed,
                                   runtime.acceleration, runtime.deceleration);
  runtime.speedPairPrepared = true;
}

void MotorControl::preparePositionMode(const Motor& motor,
                                       MotorRuntimeState& runtime,
                                       ModbusClient& bus) {
  if (runtime.positionPrepared) {
    return;
  }
  motor.setOperationMode(bus, 2, MotorOperationMode::Incremental);
  motor.setOperationFunction(bus, 2, MotorOperationFunction::SingleMotion);
  motor.setOperationSpeed(bus, 2, runtime.speed);
  motor.setOperationAcceleration(bus, 2, runtime.acceleration);
  motor.setOperationDeceleration(bus, 2, runtime.deceleration);
  runtime.positionPrepared = true;
}

void MotorControl::applyConfiguredParameters(const Motor& motor,
                                             const MotorConfig& config,
                                             ModbusClient& bus) const {
  // NET-OUT12 reports FREE state (FREE_R, function code 16). This must be
  // written before Motor::initialize reads assignments for GUI diagnostics.
  motor.setNetOutputFunctionSelection(bus, 12, 16);
  if (config.groupId.has_value()) {
    motor.setGroupId(bus, *config.groupId);
  }
  // Fixed startup policy: STOP input triggers deceleration and current-off.
  // STOP input polarity is also fixed here so the physical wiring is treated
  // as active-high by the driver.
  // Both settings are intentionally hardcoded for all active motors, not
  // config-driven.
  motor.setStopInputAction(bus, 3);
  motor.setStopInputPolarity(bus, 1);
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
  }
  motor.executeConfiguration(bus);
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
