#include <Config.hpp>
#include <Machine.hpp>
#include <TimingMetrics.hpp>

#include <chrono>
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
  if (_contec.state() == MachineComponent::State::Error) {
    return std::nullopt;
  }
  signalMap_t inputSignals;
  try {
    auto inputs = _contec.readInputs();
    for (const auto& [signal, index] : _inputMapping) {
      validateMappedIndex(signal, index, inputs.size(), "input");
      inputSignals[signal] = inputs[index];
    }
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Exception caught in 'readInputSignals'! {}", e.what());
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
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Exception caught in 'setOutputs'! {}", e.what());
  }
}

std::optional<signalMap_t> Machine::readOutputSignals() {
  if (_contec.state() == MachineComponent::State::Error) {
    return std::nullopt;
  }
  try {
    auto output = _contec.readOutputs();
    signalMap_t outputSignals;
    for (auto& [signal, index] : _outputMapping) {
      validateMappedIndex(signal, index, output.size(), "output");
      outputSignals[signal] = output[index];
    }
    return outputSignals;
  } catch (const std::exception& e) {
    SPDLOG_WARN("Exception caught in 'readOutputSignals'! {}", e.what());
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
    _loopRunner->runOneCycle(
        [this]() { controlLoopTasks(); },
        [this]() {
          if (auto command = _commandQueue.try_pop()) {
            try {
              std::visit(Overloaded{[this](const cmd::ToolChangerCommand& c) {
                                      handleToolChangerCommand(c);
                                    },
                                    [this](const cmd::ReconnectCommand& c) {
                                      handleReconnectCommand(c);
                                    }},
                         command->payload);
              command->reply.set_value("");
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
  const std::vector motors = {EMotor::XLeft,  EMotor::XRight, EMotor::YLeft,
                              EMotor::YRight, EMotor::ZLeft,  EMotor::ZRight};
  for (const auto& motor : motors) {
    _robotStatus.motors[motor] = {
        .currentPosition = 0, .targetPosition = 0, .speed = 0, .torque = 0};
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
