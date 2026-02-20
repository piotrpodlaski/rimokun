#include <Config.hpp>
#include <JsonExtensions.hpp>
#include <Machine.hpp>
#include <TimingMetrics.hpp>

#include <chrono>
#include <array>
#include <future>

using namespace std::chrono_literals;

namespace {
unsigned int requireMappingIndex(
    const std::map<std::string, unsigned int>& mapping,
    const std::string_view key) {
  const auto it = mapping.find(std::string(key));
  if (it == mapping.end()) {
    throw std::runtime_error(
        std::format("Missing required signal mapping key '{}'", key));
  }
  return it->second;
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

  response["ioOutputAssignments"].push_back(
      {{"channel", "OUT0"}, {"function", "HOME-P"}, {"functionCode", 70}, {"active", false}});
  response["ioOutputAssignments"].push_back(
      {{"channel", "OUT1"}, {"function", "END"}, {"functionCode", 69}, {"active", false}});
  response["ioOutputAssignments"].push_back(
      {{"channel", "OUT2"}, {"function", "AREA1"}, {"functionCode", 73}, {"active", false}});
  response["ioOutputAssignments"].push_back(
      {{"channel", "OUT3"}, {"function", "READY"}, {"functionCode", 67}, {"active", false}});
  response["ioOutputAssignments"].push_back(
      {{"channel", "OUT4"}, {"function", "WNG"}, {"functionCode", 66}, {"active", false}});
  response["ioOutputAssignments"].push_back(
      {{"channel", "OUT5"}, {"function", "ALM"}, {"functionCode", 65}, {"active", false}});
  response["ioOutputAssignments"].push_back(
      {{"channel", "MB"}, {"function", "MB"}, {"functionCode", 0}, {"active", false}});

  response["ioInputAssignments"].push_back(
      {{"channel", "IN0"}, {"function", "HOME"}, {"functionCode", 3}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "IN1"}, {"function", "START"}, {"functionCode", 4}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "IN2"}, {"function", "M0"}, {"functionCode", 48}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "IN3"}, {"function", "M1"}, {"functionCode", 49}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "IN4"}, {"function", "M2"}, {"functionCode", 50}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "IN5"}, {"function", "FREE"}, {"functionCode", 16}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "IN6"}, {"function", "STOP"}, {"functionCode", 18}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "IN7"}, {"function", "ALM-RST"}, {"functionCode", 24}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "+LS"}, {"function", "+LS"}, {"functionCode", 0}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "-LS"}, {"function", "-LS"}, {"functionCode", 0}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "HOMES"}, {"function", "HOMES"}, {"functionCode", 0}, {"active", false}});
  response["ioInputAssignments"].push_back(
      {{"channel", "SLIT"}, {"function", "SLIT"}, {"functionCode", 0}, {"active", false}});

  const std::array<std::pair<const char*, unsigned int>, 16> defaultNetIn{
      {{"M0", 48},       {"M1", 49},      {"M2", 50},      {"START", 4},
       {"HOME", 3},      {"STOP", 18},    {"FREE", 16},    {"not used", 0},
       {"MS0", 8},       {"MS1", 9},      {"MS2", 10},     {"SSTART", 5},
       {"+JOG", 6},      {"-JOG", 7},     {"FWD", 1},      {"RVS", 2}}};
  const std::array<std::pair<const char*, unsigned int>, 16> defaultNetOut{
      {{"M0_R", 48},     {"M1_R", 49},    {"M2_R", 50},    {"START_R", 4},
       {"HOME-P", 70},   {"READY", 67},   {"WNG", 66},     {"ALM", 65},
       {"S-BSY", 80},    {"AREA1", 73},   {"AREA2", 74},   {"AREA3", 75},
       {"TIM", 72},      {"MOVE", 68},    {"END", 69},     {"TLC", 71}}};
  for (int i = 0; i < 16; ++i) {
    response["netInputAssignments"].push_back(
        {{"channel", std::format("NET-IN{}", i)},
         {"function", defaultNetIn[static_cast<std::size_t>(i)].first},
         {"functionCode", defaultNetIn[static_cast<std::size_t>(i)].second},
         {"active", false}});
    response["netOutputAssignments"].push_back(
        {{"channel", std::format("NET-OUT{}", i)},
         {"function", defaultNetOut[static_cast<std::size_t>(i)].first},
         {"functionCode", defaultNetOut[static_cast<std::size_t>(i)].second},
         {"active", false}});
  }
  return response;
}

std::array<std::string, 8> buildContecChannelNames(
    const std::map<std::string, unsigned int>& mapping) {
  std::array<std::string, 8> names{};
  for (const auto& [signal, index] : mapping) {
    if (index < names.size() && names[index].empty()) {
      names[index] = signal;
    }
  }
  return names;
}

}  // namespace

Machine::Machine() : Machine(std::make_shared<SteadyClockAdapter>()) {}

Machine::Machine(std::shared_ptr<IClock> clock) : _clock(std::move(clock)) {
  if (!_clock) {
    throw std::runtime_error("Machine requires a non-null clock instance.");
  }
  auto& cfg = utl::Config::instance();
  _inputMapping = cfg.getRequired<std::map<std::string, unsigned int>>(
      "Machine", "inputMapping");
  _outputMapping = cfg.getRequired<std::map<std::string, unsigned int>>(
      "Machine", "outputMapping");
  const auto loopIntervalMS = cfg.getOptional<int>(
      "Machine", "loopIntervalMS",
      cfg.getOptional<int>("Machine", "loopSleepTimeMS", 10));
  const auto updateIntervalMS = cfg.getOptional<int>(
      "Machine", "updateIntervalMS",
      cfg.getOptional<int>("Machine", "statusPublishPeriodMS", 50));

  _loopInterval = std::chrono::milliseconds{std::max(1, loopIntervalMS)};
  _updateInterval = std::chrono::milliseconds{std::max(1, updateIntervalMS)};

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

std::optional<signalMap_t> Machine::readInputSignals() {
  if (_inputSignalsCache.valid && _inputSignalsCache.cycle == _ioCacheCycle) {
    return _inputSignalsCache.value;
  }
  if (_contec.state() == MachineComponent::State::Error) {
    cacheInputSignals(std::nullopt);
    return std::nullopt;
  }
  signalMap_t inputSignals;
  try {
    auto inputs = _contec.readInputs();
    for (const auto& [signal, index] : _inputMapping) {
      validateMappedIndex(signal, index, inputs.size(), "input");
      inputSignals[signal] = inputs[index];
    }
    cacheInputSignals(inputSignals);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Exception caught in 'readInputSignals'! {}", e.what());
    cacheInputSignals(std::nullopt);
    return std::nullopt;
  }
  return inputSignals;
}

void Machine::setOutputs(const signalMap_t& signals) {
  if (_contec.state() == MachineComponent::State::Error) {
    return;
  }
  try {
    auto outputs = _contec.readOutputs();
    for (const auto& [signal, value] : signals) {
      const auto it = _outputMapping.find(signal);
      if (it == _outputMapping.end()) {
        throw std::runtime_error(
            std::format("Unknown output signal '{}'", signal));
      }
      validateMappedIndex(signal, it->second, outputs.size(), "output");
      outputs[it->second] = value;
    }
    _contec.setOutputs(outputs);

    signalMap_t outputSignals;
    for (const auto& [signal, index] : _outputMapping) {
      validateMappedIndex(signal, index, outputs.size(), "output");
      outputSignals[signal] = outputs[index];
    }
    cacheOutputSignals(std::move(outputSignals));
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Exception caught in 'setOutputs'! {}", e.what());
    cacheOutputSignals(std::nullopt);
  }
}

std::optional<signalMap_t> Machine::readOutputSignals() {
  if (_outputSignalsCache.valid && _outputSignalsCache.cycle == _ioCacheCycle) {
    return _outputSignalsCache.value;
  }
  if (_contec.state() == MachineComponent::State::Error) {
    cacheOutputSignals(std::nullopt);
    return std::nullopt;
  }
  try {
    auto output = _contec.readOutputs();
    signalMap_t outputSignals;
    for (auto& [signal, index] : _outputMapping) {
      validateMappedIndex(signal, index, output.size(), "output");
      outputSignals[signal] = output[index];
    }
    cacheOutputSignals(outputSignals);
    return outputSignals;
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
    throw std::runtime_error(std::format(
        "{} mapping '{}' points outside Contec {} range (index={}, size={})",
        ioKind == "input" ? "Input" : "Output", signal, ioKind, index, ioSize));
  }
}
void Machine::processThread() {
  SPDLOG_INFO("Processing thread started");
  auto loopState = makeInitialLoopState();
  while (_isRunning.load(std::memory_order_acquire)) {
    runOneCycle(loopState);
  }
  SPDLOG_INFO("Processing thread finished!");
}

Machine::LoopState Machine::makeInitialLoopState() const {
  if (!_loopRunner) {
    throw std::runtime_error("Machine is not wired. Call MachineRuntime::wireMachine first.");
  }
  return _loopRunner->makeInitialState();
}

void Machine::runOneCycle(LoopState& state) {
  RIMO_TIMED_SCOPE("Machine::runOneCycle");
  if (!_loopRunner) {
    throw std::runtime_error("Machine is not wired. Call MachineRuntime::wireMachine first.");
  }
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
                                    [this, &responsePayload](
                                        const cmd::ContecDiagnosticsCommand& c) {
                                      responsePayload =
                                          handleContecDiagnosticsCommand(c).dump();
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
        [this]() { updateStatus(); }, state);
  } catch (const std::exception& e) {
    SPDLOG_ERROR(
        "Unhandled exception in machine loop cycle: {}. "
        "Forcing components into error state.",
        e.what());
    if (_motorControl.state() != MachineComponent::State::Error) {
      _motorControl.reset();
    }
    if (_contec.state() != MachineComponent::State::Error) {
      _contec.reset();
    }
    if (_controlPanel.state() != MachineComponent::State::Error) {
      _controlPanel.reset();
    }
  } catch (...) {
    SPDLOG_ERROR(
        "Unhandled unknown exception in machine loop cycle. "
        "Forcing components into error state.");
    if (_motorControl.state() != MachineComponent::State::Error) {
      _motorControl.reset();
    }
    if (_contec.state() != MachineComponent::State::Error) {
      _contec.reset();
    }
    if (_controlPanel.state() != MachineComponent::State::Error) {
      _controlPanel.reset();
    }
  }
}

void Machine::cacheInputSignals(std::optional<signalMap_t> value) {
  _inputSignalsCache.cycle = _ioCacheCycle;
  _inputSignalsCache.valid = true;
  _inputSignalsCache.value = std::move(value);
}

void Machine::cacheOutputSignals(std::optional<signalMap_t> value) {
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
    throw std::runtime_error("Machine controller is not wired.");
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

void Machine::initialize() {
  std::lock_guard<std::mutex> lock(_lifecycleMutex);
  if (!_loopRunner || !_controller || !_componentService || !_statusBuilder ||
      !_commandProcessor || !_commandServer) {
    throw std::runtime_error(
        "Machine collaborators are not wired. Use MachineRuntime::wireMachine "
        "before initialize().");
  }
  bool expected = false;
  if (!_isRunning.compare_exchange_strong(expected, true,
                                          std::memory_order_acq_rel)) {
    throw std::runtime_error("Machine is already running.");
  }
  makeDummyStatus();
  _componentService->initializeAll();
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
  const auto result = _componentService->reconnect(c.robotComponent);
  if (!result.empty()) {
    throw std::runtime_error(result);
  }
  makeDummyStatus();
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

nlohmann::json Machine::handleContecDiagnosticsCommand(
    const cmd::ContecDiagnosticsCommand&) {
  nlohmann::json response{
      {"inputsRaw", nlohmann::json::array()},
      {"outputsRaw", nlohmann::json::array()},
      {"inputs", nlohmann::json::array()},
      {"outputs", nlohmann::json::array()},
  };
  const auto inputNames = buildContecChannelNames(_inputMapping);
  const auto outputNames = buildContecChannelNames(_outputMapping);

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
    for (std::size_t i = 0; i < 8 && i < inputs.size(); ++i) {
      const auto active = static_cast<bool>(inputs[i]);
      response["inputsRaw"][i] = active;
      response["inputs"][i]["active"] = active;
    }
    for (std::size_t i = 0; i < 8 && i < outputs.size(); ++i) {
      const auto active = static_cast<bool>(outputs[i]);
      response["outputsRaw"][i] = active;
      response["outputs"][i]["active"] = active;
    }
  } catch (const std::exception& ex) {
    response["diagnosticsError"] = ex.what();
  }
  return response;
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
                  {EMotorStatusFlags::Alarm, ELEDState::Off}}};
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
      _robotStatus, _components,
      [this]() { return _controlPanel.getSnapshot(); },
      [this]() { return readInputSignals(); },
      [this]() { return readOutputSignals(); },
      [this](const utl::RobotStatus& status) { _robotServer.publish(status); });
}
