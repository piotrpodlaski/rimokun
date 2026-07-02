#include <Config.hpp>
#include <EnumStrings.hpp>
#include <ExceptionUtils.hpp>
#include <JsonExtensions.hpp>
#include <Machine.hpp>
#include <TimingMetrics.hpp>

#include <chrono>
#include <array>
#include <cmath>
#include <future>
#include <vector>

using namespace std::chrono_literals;

namespace {
std::string_view homingTargetName(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::All: return "All";
    case cmd::HomingTarget::XLeft: return "XLeft";
    case cmd::HomingTarget::YLeft: return "YLeft";
    case cmd::HomingTarget::XRight: return "XRight";
    case cmd::HomingTarget::YRight: return "YRight";
    case cmd::HomingTarget::Z: return "Z";
  }
  return "Unknown";
}

std::optional<cmd::HomingTarget> parseHomingTarget(const std::string_view value) {
  if (value == "All") return cmd::HomingTarget::All;
  if (value == "XLeft") return cmd::HomingTarget::XLeft;
  if (value == "YLeft") return cmd::HomingTarget::YLeft;
  if (value == "XRight") return cmd::HomingTarget::XRight;
  if (value == "YRight") return cmd::HomingTarget::YRight;
  if (value == "Z") return cmd::HomingTarget::Z;
  return std::nullopt;
}

unsigned int requireMappingIndex(
    const std::map<std::string, unsigned int, std::less<>>& mapping,
    const std::string_view key) {
  const auto it = mapping.find(key);
  if (it == mapping.end()) {
    utl::throwRuntimeError(
        std::format("Missing required signal mapping key '{}'", key));
  }
  return it->second;
}

template <typename ESignal>
void validateSignalMappingKeys(
    const std::map<std::string, unsigned int, std::less<>>& mapping,
    std::string_view mappingName) {
  for (const auto& [key, _] : mapping) {
    if (!utl::tryEnumFromString<ESignal>(key).has_value()) {
      utl::throwRuntimeError(std::format(
          "Unknown {} signal '{}' in config. Known values: [{}]",
          mappingName, key, utl::knownEnumNames<ESignal>()));
    }
  }
}

nlohmann::json makeDefaultIoAssignmentResponse(const utl::EMotor motor) {
  nlohmann::json response{
      {"motor", utl::enumToString(motor)},
      {"driverInputRaw", 0u},
      {"driverOutputRaw", 0u},
      {"ioOutputRaw", 0u},
      {"ioInputRaw", 0u},
      {"netInputRaw", 0u},
      {"netOutputRaw", 0u},
      {"inputFlags", nlohmann::json::array()},
      {"outputFlags", nlohmann::json::array()},
      {"ioOutputAssignments", nlohmann::json::array()},
      {"ioInputAssignments", nlohmann::json::array()},
      {"netOutputAssignments", nlohmann::json::array()},
      {"netInputAssignments", nlohmann::json::array()},
      {"alarm", nlohmann::json::object()},
      {"warning", nlohmann::json::object()},
  };

  const auto appendAssignment = [](nlohmann::json& target,
                                   const MotorIoAssignmentDefault assignment) {
    target.push_back({{"channel", std::string(assignment.channel)},
                      {"function", std::string(assignment.function)},
                      {"functionCode", assignment.functionCode},
                      {"active", false}});
  };
  for (const auto assignment : motor_io_defaults::ioOutput) {
    appendAssignment(response["ioOutputAssignments"], assignment);
  }
  for (const auto assignment : motor_io_defaults::ioInput) {
    appendAssignment(response["ioInputAssignments"], assignment);
  }
  for (const auto assignment : motor_io_defaults::netInput) {
    appendAssignment(response["netInputAssignments"], assignment);
  }
  for (const auto assignment : motor_io_defaults::netOutput) {
    appendAssignment(response["netOutputAssignments"], assignment);
  }
  return response;
}

std::vector<std::string> buildContecChannelNames(
    const std::map<std::string, unsigned int, std::less<>>& mapping,
    const std::size_t count) {
  std::vector<std::string> names(count);
  for (const auto& [signal, index] : mapping) {
    if (index < names.size() && names[index].empty()) {
      names[index] = signal;
    }
  }
  return names;
}

std::string_view axisConfigKey(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::XLeft: return "leftArmX";
    case cmd::HomingTarget::YLeft: return "leftArmY";
    case cmd::HomingTarget::XRight: return "rightArmX";
    case cmd::HomingTarget::YRight: return "rightArmY";
    case cmd::HomingTarget::Z: return "gantryZ";
    case cmd::HomingTarget::All: break;
  }
  utl::throwRuntimeError("Homing target 'All' does not map to a single axis.");
}

utl::EMotor motorForHomingTarget(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::XLeft: return utl::EMotor::XLeft;
    case cmd::HomingTarget::YLeft: return utl::EMotor::YLeft;
    case cmd::HomingTarget::XRight: return utl::EMotor::XRight;
    case cmd::HomingTarget::YRight: return utl::EMotor::YRight;
    case cmd::HomingTarget::Z: return utl::EMotor::ZLeft;
    case cmd::HomingTarget::All: break;
  }
  utl::throwRuntimeError("Homing target 'All' does not map to a single motor.");
}

}  // namespace

Machine::~Machine() { shutdown(); }

Machine::Machine() : Machine(std::make_shared<SteadyClockAdapter>()) {}

Machine::Machine(std::shared_ptr<IClock> clock) : _clock(std::move(clock)) {
  if (!_clock) {
    utl::throwRuntimeError("Machine requires a non-null clock instance.");
  }
  auto& cfg = utl::Config::instance();
  const auto inputMapping = cfg.getRequired<std::map<std::string, unsigned int>>(
      "Machine", "inputMapping");
  const auto outputMapping = cfg.getRequired<std::map<std::string, unsigned int>>(
      "Machine", "outputMapping");
  _inputMapping.insert(inputMapping.begin(), inputMapping.end());
  _outputMapping.insert(outputMapping.begin(), outputMapping.end());
  for (const auto& [signal, _] : _inputMapping) {
    _inputSignals.emplace(signal, false);
  }
  for (const auto& [signal, _] : _outputMapping) {
    _outputSignals.emplace(signal, false);
  }
  validateSignalMappingKeys<utl::EInputSignal>(_inputMapping, "input");
  validateSignalMappingKeys<utl::EOutputSignal>(_outputMapping, "output");
  const auto loopIntervalMS = cfg.getOptional<int>(
      "Machine", "loopIntervalMS",
      cfg.getOptional<int>("Machine", "loopSleepTimeMS", 10));
  const auto updateIntervalMS = cfg.getOptional<int>(
      "Machine", "updateIntervalMS",
      cfg.getOptional<int>("Machine", "statusPublishPeriodMS", 50));
  _statusUpdatesEnabled =
      cfg.getOptional<bool>("Machine", "statusUpdatesEnabled", true);
  _homingOrder = loadHomingOrder();
  _homingAxisConfigs = loadHomingAxisConfigs();

  _loopInterval = std::chrono::milliseconds{std::max(1, loopIntervalMS)};
  _updateInterval = std::chrono::milliseconds{std::max(1, updateIntervalMS)};
  const auto commandQueueMaxSize =
      cfg.getOptional<std::size_t>("Machine", "commandQueueMaxSize", 16u);
  _commandQueue.setMaxSize(commandQueueMaxSize);

  _components.emplace(_contec.componentType(), &_contec);
  _components.emplace(_controlPanel.componentType(), &_controlPanel);
  _components.emplace(_motorControl.componentType(), &_motorControl);

  // Validate mappings used by the control loop and command handlers at startup.
  (void)requireMappingIndex(_inputMapping, "button1");
  (void)requireMappingIndex(_inputMapping, "button2");
  (void)requireMappingIndex(_outputMapping, "toolChangerLeft");
  (void)requireMappingIndex(_outputMapping, "toolChangerRight");
  (void)requireMappingIndex(_outputMapping, "light1");
  (void)requireMappingIndex(_outputMapping, "light2");
}

std::optional<utl::SignalMap> Machine::readInputSignals() {
  if (_inputSignalsCache.valid && _inputSignalsCache.cycle == _ioCacheCycle) {
    return _inputSignalsCache.value;
  }
  if (_contec.state() == MachineComponent::State::Error) {
    cacheInputSignals(std::nullopt);
    return std::nullopt;
  }
  try {
    auto inputs = _contec.readInputs();
    for (const auto& [signal, index] : _inputMapping) {
      validateMappedIndex(signal, index, inputs.size(), "input");
      _inputSignals[signal] = inputs[index];
    }
    cacheInputSignals(_inputSignals);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Exception caught in 'readInputSignals'! {}", e.what());
    cacheInputSignals(std::nullopt);
    return std::nullopt;
  }
  return _inputSignals;
}

void Machine::setOutputs(const utl::SignalMap& signals) {
  if (_contec.state() == MachineComponent::State::Error) {
    return;
  }
  try {
    auto outputs = _contec.readOutputs();
    for (const auto& [signal, value] : signals) {
      const auto it = _outputMapping.find(signal);
      if (it == _outputMapping.end()) {
        utl::throwRuntimeError(
            std::format("Unknown output signal '{}'", signal));
      }
      validateMappedIndex(signal, it->second, outputs.size(), "output");
      outputs[it->second] = value;
    }
    _contec.setOutputs(outputs);

    for (const auto& [signal, index] : _outputMapping) {
      validateMappedIndex(signal, index, outputs.size(), "output");
      _outputSignals[signal] = outputs[index];
    }
    cacheOutputSignals(_outputSignals);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Exception caught in 'setOutputs'! {}", e.what());
    cacheOutputSignals(std::nullopt);
  }
}

std::optional<utl::SignalMap> Machine::readOutputSignals() {
  if (_outputSignalsCache.valid && _outputSignalsCache.cycle == _ioCacheCycle) {
    return _outputSignalsCache.value;
  }
  if (_contec.state() == MachineComponent::State::Error) {
    cacheOutputSignals(std::nullopt);
    return std::nullopt;
  }
  try {
    auto output = _contec.readOutputs();
    for (auto& [signal, index] : _outputMapping) {
      validateMappedIndex(signal, index, output.size(), "output");
      _outputSignals[signal] = output[index];
    }
    cacheOutputSignals(_outputSignals);
    return _outputSignals;
  } catch (const std::exception& e) {
    SPDLOG_WARN("Exception caught in 'readOutputSignals'! {}", e.what());
    cacheOutputSignals(std::nullopt);
    return std::nullopt;
  }
}

void Machine::validateMappedIndex(const std::string_view signal,
                                  const unsigned int index,
                                  const std::size_t ioSize,
                                  const std::string_view ioKind) {
  if (index >= ioSize) {
    utl::throwRuntimeError(std::format(
        "{} mapping '{}' points outside Contec {} range (index={}, size={})",
        ioKind == "input" ? "Input" : "Output", signal, ioKind, index, ioSize));
  }
}
void Machine::processThread() {
  SPDLOG_INFO("Processing thread started");
  ControlLoopRunner::State loopState;
  while (_isRunning.load(std::memory_order_acquire)) {
    runOneCycle(loopState);
  }
  SPDLOG_INFO("Processing thread finished!");
}

void Machine::runOneCycle(LoopState& state) {
  RIMO_TIMED_SCOPE("Machine::runOneCycle");
  if (!_loopRunner) {
    utl::throwRuntimeError("Machine is not wired. Call wire() first.");
  }
  const auto resetComponentsToError = [this]() {
    const auto resetIfNeeded = [](MachineComponent& component) {
      if (component.state() != MachineComponent::State::Error) {
        component.reset();
      }
    };
    resetIfNeeded(_motorControl);
    resetIfNeeded(_contec);
    resetIfNeeded(_controlPanel);
  };
  try {
    ++_ioCacheCycle;
    _inputSignalsCache.valid = false;
    _outputSignalsCache.valid = false;
    _loopRunner->runOneCycle(
        [this]() { controlLoopTasks(); },
        [this]() {
          if (auto command = _commandQueue.try_pop()) {
            try {
              std::string responsePayload;
              std::visit(Overloaded{[this](const cmd::ToolChangerCommand& c) {
                                      handleToolChangerCommand(c);
                                    },
                                    [this](const cmd::ReconnectCommand& c) {
                                      handleReconnectCommand(c);
                                    },
                                    [this, &responsePayload](
                                        const cmd::MotorDiagnosticsCommand& c) {
                                      responsePayload =
                                          handleMotorDiagnosticsCommand(c).dump();
                                    },
                                    [this](const cmd::ResetMotorAlarmCommand& c) {
                                      handleResetMotorAlarmCommand(c);
                                    },
                                    [this](const cmd::SetMotorEnabledCommand& c) {
                                      handleSetMotorEnabledCommand(c);
                                    },
                                    [this](
                                        const cmd::SetAllMotorsEnabledCommand& c) {
                                      handleSetAllMotorsEnabledCommand(c);
                                    },
                                    [this](const cmd::HomingCommand& c) {
                                      handleHomingCommand(c);
                                    },
                                    [this, &responsePayload](
                                        const cmd::ContecDiagnosticsCommand& c) {
                                      responsePayload =
                                          handleContecDiagnosticsCommand(c).dump();
                                    },
                                    [this](const cmd::EmergencyStopCommand& c) {
                                      handleEmergencyStopCommand(c);
                                    }},
                         command->payload);
              command->reply.set_value(std::move(responsePayload));
            } catch (const std::exception& e) {
              SPDLOG_WARN("Exception caught in 'std::visit'! {}", e.what());
              command->reply.set_value(e.what());
            } catch (...) {
              SPDLOG_WARN("Unknown exception caught in 'std::visit'!");
              command->reply.set_value("Unknown exception while processing command");
            }
          }
        },
        [this]() {
          if (_statusUpdatesEnabled) {
            updateStatus();
          }
        },
        state);
  } catch (const std::exception& e) {
    SPDLOG_ERROR(
        "Unhandled exception in machine loop cycle: {}. "
        "Forcing components into error state.",
        e.what());
    resetComponentsToError();
  } catch (...) {
    SPDLOG_ERROR(
        "Unhandled unknown exception in machine loop cycle. "
        "Forcing components into error state.");
    resetComponentsToError();
  }
}

void Machine::cacheInputSignals(std::optional<utl::SignalMap> value) {
  _inputSignalsCache.cycle = _ioCacheCycle;
  _inputSignalsCache.valid = true;
  _inputSignalsCache.value = std::move(value);
}

void Machine::cacheOutputSignals(std::optional<utl::SignalMap> value) {
  _outputSignalsCache.cycle = _ioCacheCycle;
  _outputSignalsCache.valid = true;
  _outputSignalsCache.value = std::move(value);
}

bool Machine::submitCommand(cmd::Command command) {
  return _commandQueue.push(std::move(command));
}

std::string Machine::dispatchCommandAndWait(cmd::Command command,
                                            const std::chrono::milliseconds timeout) {
  auto future = command.reply.get_future();
  if (!submitCommand(std::move(command))) {
    if (_isRunning.load(std::memory_order_acquire)) {
      SPDLOG_WARN("Command queue is full. Command rejected.");
      return "Command queue is full";
    }
    return "Machine is shutting down";
  }
  if (future.wait_for(timeout) != std::future_status::ready) {
    return "Command processing timed out";
  }
  return future.get();
}

void Machine::controlLoopTasks() {
  RIMO_TIMED_SCOPE("Machine::controlLoopTasks");
  if (!_controller) {
    utl::throwRuntimeError("Machine controller is not wired.");
  }
  try {
    _controller->runControlLoopTasks();
  } catch (const std::exception& e) {
    SPDLOG_ERROR(
        "Control loop task failed: {}. Putting MotorControl into error state.",
        e.what());
    if (_motorControl.state() != MachineComponent::State::Error) {
      _motorControl.reset();
    }
  } catch (...) {
    SPDLOG_ERROR(
        "Control loop task failed with unknown exception. "
        "Putting MotorControl into error state.");
    if (_motorControl.state() != MachineComponent::State::Error) {
      _motorControl.reset();
    }
  }
}

void Machine::wire() {
  if (!_loopRunner) {
    _loopRunner = std::make_unique<ControlLoopRunner>(
        *_clock, _loopInterval, _updateInterval);
  }
  if (!_controller) {
    _controller = std::make_unique<MachineController>(
        MachineController::IoOps{
            .readInputs = [this]() { return readInputSignals(); },
            .setOutputs = [this](const utl::SignalMap& outputs) { setOutputs(outputs); },
            .readOutputs = [this]() { return readOutputSignals(); },
            .contecState = [this]() { return _contec.state(); },
        },
        MachineController::MotorOps{
            .setMode = [this](utl::EMotor id, MotorControlMode m) { _motorControl.setMode(id, m); },
            .setSpeed = [this](utl::EMotor id, std::int32_t v) { _motorControl.setSpeed(id, v); },
            .setAcceleration = [this](utl::EMotor id, std::int32_t v) { _motorControl.setAcceleration(id, v); },
            .setDeceleration = [this](utl::EMotor id, std::int32_t v) { _motorControl.setDeceleration(id, v); },
            .setPosition = [this](utl::EMotor id, std::int32_t v) { _motorControl.setPosition(id, v); },
            .setDirection = [this](utl::EMotor id, MotorControlDirection d) { _motorControl.setDirection(id, d); },
            .start = [this](utl::EMotor id) { _motorControl.startMovement(id); },
            .stop = [this](utl::EMotor id) { _motorControl.stopMovement(id); },
            .isConfigured = [this](utl::EMotor id) { return _motorControl.motors().contains(id); },
            .onAlarmCleared = [this](utl::EMotor id) { _motorControl.onAlarmCleared(id); },
        },
        _robotStatus, std::make_unique<RimoKunControlPolicy>());
  }
  if (!_statusBuilder) {
    _statusBuilder = std::make_unique<MachineStatusBuilder>();
  }
  if (!_commandServer) {
    _commandServer = std::make_unique<MachineCommandServer>(_robotServer);
  }
}

void Machine::initialize() {
  std::lock_guard<std::mutex> lock(_lifecycleMutex);
  if (!_loopRunner || !_controller || !_statusBuilder || !_commandServer) {
    utl::throwRuntimeError(
        "Machine collaborators are not wired. Call wire() before initialize().");
  }
  bool expected = false;
  if (!_isRunning.compare_exchange_strong(expected, true,
                                          std::memory_order_acq_rel)) {
    utl::throwRuntimeError("Machine is already running.");
  }
  makeDummyStatus();
  initializeComponents();
  try {
    _commandServerThread = std::thread(&Machine::commandServerThread, this);
    _processThread = std::thread(&Machine::processThread, this);
  } catch (...) {
    _isRunning.store(false, std::memory_order_release);
    throw;
  }
}
void Machine::commandServerThread() {
  _commandServer->runLoop(
      _isRunning, [this](cmd::Command c, const std::chrono::milliseconds timeout) {
        return dispatchCommandAndWait(std::move(c), timeout);
      });
}

void Machine::handleToolChangerCommand(const cmd::ToolChangerCommand& c) {
  _controller->handleToolChangerCommand(c);
}
void Machine::handleReconnectCommand(const cmd::ReconnectCommand& c) {
  const auto result = reconnectComponent(c.robotComponent);
  if (!result.empty()) {
    utl::throwRuntimeError(result);
  }
  makeDummyStatus();
}

void Machine::initializeComponents() {
  for (const auto& [componentType, component] : _components) {
    if (!component) {
      continue;
    }
    try {
      component->initialize();
    } catch (const std::exception& e) {
      SPDLOG_ERROR("{} initialization failed: {}",
                   utl::enumToStringView(componentType), e.what());
    }
  }
}

std::string Machine::reconnectComponent(const utl::ERobotComponent component) {
  const auto it = _components.find(component);
  if (it == _components.end() || !it->second) {
    return std::format("Resetting of '{}' is not implemented!",
                       utl::enumToStringView(component));
  }
  auto* comp = it->second;
  SPDLOG_INFO("Reconnecting {}...", utl::enumToStringView(component));
  comp->reset();
  try {
    comp->initialize();
  } catch (const std::exception& e) {
    return std::format("Resetting '{}' failed: {}",
                       utl::enumToStringView(component), e.what());
  }
  if (comp->state() == MachineComponent::State::Error) {
    return std::format("Resetting '{}' was unsuccessful!",
                       utl::enumToStringView(component));
  }
  return "";
}

std::vector<cmd::HomingTarget> Machine::loadHomingOrder() const {
  constexpr std::array defaultOrder{
      cmd::HomingTarget::Z,
      cmd::HomingTarget::XLeft,
      cmd::HomingTarget::XRight,
      cmd::HomingTarget::YLeft,
      cmd::HomingTarget::YRight,
  };

  std::vector<cmd::HomingTarget> order(defaultOrder.begin(), defaultOrder.end());
  try {
    const auto machineCfg = utl::Config::instance().getClassConfig("Machine");
    const auto homingCfg = machineCfg["homing"];
    if (!homingCfg || !homingCfg.IsMap()) {
      return order;
    }
    const auto orderCfg = homingCfg["order"];
    if (!orderCfg || !orderCfg.IsSequence()) {
      return order;
    }
    order.clear();
    for (const auto& node : orderCfg) {
      const auto value = node.as<std::string>();
      const auto parsed = parseHomingTarget(value);
      if (!parsed.has_value()) {
        utl::throwRuntimeError(std::format(
            "Unknown Machine.homing.order target '{}'", value));
      }
      if (*parsed == cmd::HomingTarget::All) {
        utl::throwRuntimeError(
            "Machine.homing.order must list single axes, not 'All'.");
      }
      order.push_back(*parsed);
    }
  } catch (const std::exception& e) {
    SPDLOG_WARN("Using default homing order: {}", e.what());
    order.assign(defaultOrder.begin(), defaultOrder.end());
  }
  return order;
}

std::map<cmd::HomingTarget, Machine::HomingAxisConfig>
Machine::loadHomingAxisConfigs() const {
  std::map<cmd::HomingTarget, HomingAxisConfig> configs;
  try {
    const auto machineCfg = utl::Config::instance().getClassConfig("Machine");
    const auto axesCfg = machineCfg["motion"]["axes"];
    const auto directionCfg = machineCfg["homing"]["startingDirection"];
    if (!axesCfg || !axesCfg.IsMap()) {
      return configs;
    }
    if (!directionCfg || !directionCfg.IsMap()) {
      return configs;
    }

    for (const auto axis : {cmd::HomingTarget::XLeft, cmd::HomingTarget::YLeft,
                           cmd::HomingTarget::XRight, cmd::HomingTarget::YRight,
                           cmd::HomingTarget::Z}) {
      const auto axisKey = axisConfigKey(axis);
      const auto axisCfg = axesCfg[std::string(axisKey)];
      if (!axisCfg || !axisCfg.IsMap()) {
        utl::throwRuntimeError(std::format(
            "Missing Machine.motion.axes.{} config for homing.", axisKey));
      }
      const auto directionNode =
          directionCfg[std::string(homingTargetName(axis))];
      if (!directionNode) {
        utl::throwRuntimeError(std::format(
            "Missing Machine.homing.startingDirection.{} config.",
            homingTargetName(axis)));
      }
      const auto startingDirection = directionNode.as<std::int32_t>();
      if (startingDirection != 0 && startingDirection != 1) {
        utl::throwRuntimeError(std::format(
            "Machine.homing.startingDirection.{} must be 0 or 1.",
            homingTargetName(axis)));
      }
      const auto slowSpeedMmPerSec =
          axisCfg["slowMaxLinearSpeedMmPerSec"].as<double>();
      const auto stepsPerMm = axisCfg["stepsPerMm"].as<double>();
      configs.emplace(
          axis, HomingAxisConfig{
                    .motorId = motorForHomingTarget(axis),
                    .speedStepsPerSec = static_cast<std::int32_t>(
                        std::llround(slowSpeedMmPerSec * stepsPerMm)),
                    .acceleration =
                        axisCfg["slowAcceleration001MsPerKHz"].as<std::int32_t>(),
                    .startingDirection = startingDirection,
                });
    }
  } catch (const std::exception& e) {
    SPDLOG_WARN("Homing axis config cache is incomplete: {}", e.what());
    configs.clear();
  }
  return configs;
}

nlohmann::json Machine::handleMotorDiagnosticsCommand(
    const cmd::MotorDiagnosticsCommand& c) {
  auto response = makeDefaultIoAssignmentResponse(c.motor);
  try {
    const auto inputStatus = _motorControl.readInputStatus(c.motor);
    const auto outputStatus = _motorControl.readOutputStatus(c.motor);
    const auto directIoStatus = _motorControl.readDirectIoStatus(c.motor);
    const auto remoteIoStatus = _motorControl.readRemoteIoStatus(c.motor);
    const auto alarm = _motorControl.diagnoseCurrentAlarm(c.motor);
    const auto warning = _motorControl.diagnoseCurrentWarning(c.motor);

    response["driverInputRaw"] = inputStatus.raw;
    response["driverOutputRaw"] = outputStatus.raw;
    response["ioOutputRaw"] = directIoStatus.reg00D4;
    response["ioInputRaw"] = directIoStatus.reg00D5;
    response["netInputRaw"] = remoteIoStatus.reg007D;
    response["netOutputRaw"] = remoteIoStatus.reg007F;
    response["inputFlags"] = nlohmann::json::array();
    for (const auto flag : inputStatus.activeFlags) {
      response["inputFlags"].push_back(std::string(flag));
    }
    response["outputFlags"] = nlohmann::json::array();
    for (const auto flag : outputStatus.activeFlags) {
      response["outputFlags"].push_back(std::string(flag));
    }
    response["ioOutputAssignments"] = nlohmann::json::array();
    for (const auto& assignment : directIoStatus.outputAssignments) {
      response["ioOutputAssignments"].push_back(nlohmann::json{
          {"channel", assignment.channel},
          {"function", assignment.function},
          {"functionCode", assignment.functionCode},
          {"active", assignment.active},
      });
    }
    response["ioInputAssignments"] = nlohmann::json::array();
    for (const auto& assignment : directIoStatus.inputAssignments) {
      response["ioInputAssignments"].push_back(nlohmann::json{
          {"channel", assignment.channel},
          {"function", assignment.function},
          {"functionCode", assignment.functionCode},
          {"active", assignment.active},
      });
    }
    response["netOutputAssignments"] = nlohmann::json::array();
    for (const auto& assignment : remoteIoStatus.outputAssignments) {
      response["netOutputAssignments"].push_back(nlohmann::json{
          {"channel", assignment.channel},
          {"function", assignment.function},
          {"functionCode", assignment.functionCode},
          {"active", assignment.active},
      });
    }
    response["netInputAssignments"] = nlohmann::json::array();
    for (const auto& assignment : remoteIoStatus.inputAssignments) {
      response["netInputAssignments"].push_back(nlohmann::json{
          {"channel", assignment.channel},
          {"function", assignment.function},
          {"functionCode", assignment.functionCode},
          {"active", assignment.active},
      });
    }
    response["alarm"] = nlohmann::json{
        {"code", alarm.code},
        {"known", alarm.known},
        {"type", alarm.type},
        {"cause", alarm.cause},
        {"remedialAction", alarm.remedialAction},
    };
    response["warning"] = nlohmann::json{
        {"code", warning.code},
        {"known", warning.known},
        {"type", warning.type},
        {"cause", warning.cause},
        {"remedialAction", warning.remedialAction},
    };
  } catch (const std::exception& ex) {
    response["diagnosticsError"] = ex.what();
  }
  return response;
}

void Machine::handleResetMotorAlarmCommand(const cmd::ResetMotorAlarmCommand& c) {
  _motorControl.resetAlarm(c.motor);
}

void Machine::handleSetMotorEnabledCommand(const cmd::SetMotorEnabledCommand& c) {
  _motorControl.setEnabled(c.motor, c.enabled);
}

void Machine::handleSetAllMotorsEnabledCommand(
    const cmd::SetAllMotorsEnabledCommand& c) {
  _motorControl.setAllEnabled(c.enabled);
}

void Machine::handleHomingCommand(const cmd::HomingCommand& c) {
  if (c.target == cmd::HomingTarget::All) {
    homeAxesInOrder(_homingOrder);
    return;
  }
  homeSingleAxis(c.target);
}

void Machine::homeAxesInOrder(const std::vector<cmd::HomingTarget>& axes) {
  for (const auto axis : axes) {
    homeSingleAxis(axis);
  }
}

void Machine::homeSingleAxis(const cmd::HomingTarget axis) {
  const auto cfgIt = _homingAxisConfigs.find(axis);
  if (cfgIt == _homingAxisConfigs.end()) {
    utl::throwRuntimeError(std::format("Missing cached homing config for {}.",
                                       homingTargetName(axis)));
  }
  const auto& cfg = cfgIt->second;

  SPDLOG_INFO("Starting homing for {}: speed={} steps/s, accel={}, direction={}",
              homingTargetName(axis), cfg.speedStepsPerSec, cfg.acceleration,
              cfg.startingDirection);
  _motorControl.configureHoming(cfg.motorId, 0, cfg.speedStepsPerSec,
                                cfg.acceleration, cfg.speedStepsPerSec,
                                cfg.startingDirection);
  _motorControl.pulseHome(cfg.motorId);
  _homingRequested[cfg.motorId] = true;
}

nlohmann::json Machine::handleContecDiagnosticsCommand(
    const cmd::ContecDiagnosticsCommand&) {
  nlohmann::json response{
      {"inputsRaw", nlohmann::json::array()},
      {"outputsRaw", nlohmann::json::array()},
      {"inputs", nlohmann::json::array()},
      {"outputs", nlohmann::json::array()},
  };
  const auto inputCount = static_cast<std::size_t>(_contec.getNInputs());
  const auto outputCount = static_cast<std::size_t>(_contec.getNOutputs());
  const auto inputNames = buildContecChannelNames(_inputMapping, inputCount);
  const auto outputNames = buildContecChannelNames(_outputMapping, outputCount);

  for (std::size_t i = 0; i < inputNames.size(); ++i) {
    response["inputsRaw"].push_back(false);
    response["inputs"].push_back(
        {{"channel", std::format("IN{}", i)},
         {"active", false},
         {"signal", inputNames[i]}});
  }
  for (std::size_t i = 0; i < outputNames.size(); ++i) {
    response["outputsRaw"].push_back(false);
    response["outputs"].push_back(
        {{"channel", std::format("OUT{}", i)},
         {"active", false},
         {"signal", outputNames[i]}});
  }

  try {
    const auto inputs = _contec.readInputs();
    const auto outputs = _contec.readOutputs();
    for (std::size_t i = 0; i < inputCount && i < inputs.size(); ++i) {
      const auto active = static_cast<bool>(inputs[i]);
      response["inputsRaw"][i] = active;
      response["inputs"][i]["active"] = active;
    }
    for (std::size_t i = 0; i < outputCount && i < outputs.size(); ++i) {
      const auto active = static_cast<bool>(outputs[i]);
      response["outputsRaw"][i] = active;
      response["outputs"][i]["active"] = active;
    }
  } catch (const std::exception& ex) {
    response["diagnosticsError"] = ex.what();
  }
  return response;
}

void Machine::handleEmergencyStopCommand(const cmd::EmergencyStopCommand&) {
  SPDLOG_WARN("Emergency stop requested — not yet implemented.");
}

void Machine::shutdown() {
  std::lock_guard<std::mutex> lock(_lifecycleMutex);
  SPDLOG_INFO("Shutting down. Joining Threads...");
  _isRunning.store(false, std::memory_order_release);
  _commandQueue.shutdown();
  while (auto command = _commandQueue.try_pop()) {
    command->reply.set_value("Machine is shutting down");
  }
  if (_processThread.joinable()) _processThread.join();
  if (_commandServerThread.joinable()) _commandServerThread.join();
}

void Machine::makeDummyStatus() {
  using namespace utl;
  _robotStatus.motors.clear();
  const std::vector motors = {EMotor::XLeft,  EMotor::XRight, EMotor::YLeft,
                              EMotor::YRight, EMotor::ZLeft,  EMotor::ZRight};
  for (const auto& motor : motors) {
    _robotStatus.motors[motor] = {
        .currentPosition = 0,
        .targetPosition = 0,
        .speed = 0,
        .torque = 0,
        .state = ELEDState::Off,
        .warningDescription = "",
        .alarmDescription = "",
        .flags = {{EMotorStatusFlags::BrakeApplied, ELEDState::Off},
                  {EMotorStatusFlags::Enabled, ELEDState::Off},
                  {EMotorStatusFlags::Warning, ELEDState::Off},
                  {EMotorStatusFlags::Alarm, ELEDState::Off},
                  {EMotorStatusFlags::Homing, ELEDState::Off}}};
  }
  _robotStatus.toolChangers[EArm::Left] = {
      .flags = {{EToolChangerStatusFlags::ProxSen, ELEDState::Off},
                {EToolChangerStatusFlags::ClosedSen, ELEDState::Off},
                {EToolChangerStatusFlags::OpenSen, ELEDState::Off},
                {EToolChangerStatusFlags::OpenValve, ELEDState::Off},
                {EToolChangerStatusFlags::ClosedValve, ELEDState::Off}}};
  _robotStatus.toolChangers[EArm::Right] = {
      .flags = {{EToolChangerStatusFlags::ProxSen, ELEDState::Off},
                {EToolChangerStatusFlags::ClosedSen, ELEDState::Off},
                {EToolChangerStatusFlags::OpenSen, ELEDState::Off},
                {EToolChangerStatusFlags::OpenValve, ELEDState::Off},
                {EToolChangerStatusFlags::ClosedValve, ELEDState::Off}}};
}

void Machine::updateStatus() {
  RIMO_TIMED_SCOPE("Machine::updateStatus");
  _statusBuilder->updateAndPublish(
      _robotStatus, _components, &_motorControl,
      [this]() { return _controlPanel.getSnapshot(); },
      [this]() { return readInputSignals(); },
      [this]() { return readOutputSignals(); },
      [this](const utl::RobotStatus&) {
        for (auto& [motorId, requested] : _homingRequested) {
          auto motorIt = _robotStatus.motors.find(motorId);
          if (motorIt == _robotStatus.motors.end()) {
            continue;
          }
          auto& homingLed =
              motorIt->second.flags[utl::EMotorStatusFlags::Homing];
          if (homingLed == utl::ELEDState::On) {
            requested = false;
          } else if (requested) {
            homingLed = utl::ELEDState::Warning;
          }
        }
        _robotServer.publish(_robotStatus);
      });
}
