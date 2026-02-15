#include <Config.hpp>
#include <Machine.hpp>
#include <future>

using namespace std::chrono_literals;

Machine::Machine() {
  auto& cfg = utl::Config::instance();
  _inputMapping = cfg.getRequired<std::map<std::string, unsigned int>>(
      "Machine", "inputMapping");
  _outputMapping = cfg.getRequired<std::map<std::string, unsigned int>>(
      "Machine", "outputMapping");
  _loopSleepTime = 1ms * cfg.getRequired<double>("Machine", "loopSleepTimeMS");

  _components.emplace(_contec.componentType(), &_contec);
  _components.emplace(_controlPanel.componentType(), &_controlPanel);
  _components.emplace(_motorControl.componentType(), &_motorControl);
}

std::optional<signalMap_t> Machine::readInputSignals() {
  if (_contec.state() == MachineComponent::State::Error) {
    return std::nullopt;
  }
  signalMap_t inputSignals;
  try {
    auto inputs = _contec.readInputs();
    for (const auto& [signal, index] : _inputMapping) {
      inputSignals[signal] = inputs[index];
    }
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Exception caught in 'readInputSignals'! {}", e.what());
    return std::nullopt;
  }
  return inputSignals;
}

void Machine::setOutputs(signalMap_t signals) {
  if (_contec.state() == MachineComponent::State::Error) {
    return;
  }
  try {
    auto outputs = _contec.readOutputs();
    for (const auto& [signal, value] : signals) {
      outputs[_outputMapping[signal]] = value;
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
      outputSignals[signal] = output[index];
    }
    return outputSignals;
  } catch (const std::exception& e) {
    SPDLOG_WARN("Exception caught in 'readOutputSignals'! {}", e.what());
    return std::nullopt;
  }
}
void Machine::processThread() {
  SPDLOG_INFO("Processing thread started");
  while (_isRunning) {
    // regular control loop stuff
    controlLoopTasks();
    // next we handle commands
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
      } catch (const std::runtime_error& e) {
        SPDLOG_WARN("Exception caught in 'std::visit'! {}", e.what());
        command->reply.set_value(e.what());
      }
    }
    updateStatus();
    std::this_thread::sleep_for(_loopSleepTime);
  }
  SPDLOG_INFO("Processing thread finished!");
}

void Machine::controlLoopTasks() {
  // CONTEC
  auto inputs = readInputSignals();
  if (!inputs || _contec.state() == MachineComponent::State::Error) {
    // unable to access Contec for whatever reason, producing errors!
    for (auto& [_, tc] : _robotStatus.toolChangers) {
      for (auto& [_, flag] : tc.flags) {
        flag = utl::ELEDState::ErrorBlinking;
      }
    }
    return;
  } else {
    signalMap_t outputSignals;
    outputSignals["light1"] = inputs->at("button1");
    outputSignals["light2"] = inputs->at("button2");
    setOutputs(outputSignals);
  }
}

void Machine::initialize() {
  makeDummyStatus();
  for (const auto& [componentType, component] : _components) {
    try {
      component->initialize();
    } catch (const std::exception& e) {
      SPDLOG_ERROR("{} initialization failed: {}",
                   magic_enum::enum_name(componentType), e.what());
    }
  }
  _isRunning = true;
  _commandServerThread = std::thread(&Machine::commandServerThread, this);
  _processThread = std::thread(&Machine::processThread, this);
}
void Machine::commandServerThread() {
  SPDLOG_INFO("Command Server Thread Started!");
  while (_isRunning) {
    auto command = _robotServer.receiveCommand();
    if (!command) continue;
    SPDLOG_INFO("Received command:\n{}", YAML::Dump(*command));
    YAML::Node response;
    response["status"] = "OK";
    response["message"] = "";
    if (!command->IsMap()) {
      std::string msg = "Command must be a map! Ignoring!";
      response["status"] = "Error";
      response["message"] = msg;
      SPDLOG_ERROR(msg);
      _robotServer.sendResponse(response);
      continue;
    }
    const auto typeNode = (*command)["type"];
    if (!typeNode || !typeNode.IsScalar()) {
      std::string msg = "Command lacks a valid 'type' entry! Ignoring!";
      response["status"] = "Error";
      response["message"] = msg;
      SPDLOG_ERROR(msg);
      _robotServer.sendResponse(response);
      continue;
    }
    const auto type = typeNode.as<std::string>();
    if (type == "toolChanger") {
      if ((*command)["position"] && (*command)["action"]) {
        const auto action = (*command)["action"].as<utl::EToolChangerAction>();
        const auto position = (*command)["position"].as<utl::EArm>();
        cmd::Command c;
        c.payload = cmd::ToolChangerCommand(position, action);
        auto fut = c.reply.get_future();
        _commandQueue.push(std::move(c));
        std::string reply = fut.get();
        response["message"] = "";
        if (reply.empty()) {  // everything OK
          response["status"] = "OK";
        } else {  // there was a problem
          response["status"] = "Error";
          response["message"] = reply;
        }
      }
    } else if (type == "reset") {
      if ((*command)["system"]) {
        auto robotComponent = (*command)["system"].as<utl::ERobotComponent>();
        cmd::Command c;
        c.payload = cmd::ReconnectCommand(robotComponent);
        auto fut = c.reply.get_future();
        _commandQueue.push(std::move(c));
        std::string reply = fut.get();
        response["message"] = "";
        if (reply.empty()) {  // everything OK
          response["status"] = "OK";
        } else {  // there was a problem
          response["status"] = "Error";
          response["message"] = reply;
        }
      }
    }

    else {
      std::string msg = std::format("Unknown command type '{}'!", type);
      SPDLOG_ERROR(msg);
      response["status"] = "Error";
      response["message"] = msg;
      response["response"] = "YAY!";
    }
    _robotServer.sendResponse(response);
  }
  SPDLOG_INFO("Command Server thread finished!");
}

void Machine::handleToolChangerCommand(const cmd::ToolChangerCommand& c) {
  SPDLOG_INFO("Changing status of '{}' tool changer to '{}'",
              magic_enum::enum_name(c.arm), magic_enum::enum_name(c.action));
  if (_contec.state() == MachineComponent::State::Error) {
    std::string msg =
        "Contec is in error state. Not possible to alter tool changer state!";
    SPDLOG_ERROR(msg);
    throw std::runtime_error(msg);
  }
  signalMap_t outputSignals;
  if (c.arm == utl::EArm::Left) {
    outputSignals["toolChangerLeft"] =
        (c.action == utl::EToolChangerAction::Open);
  } else if (c.arm == utl::EArm::Right) {
    outputSignals["toolChangerRight"] =
        (c.action == utl::EToolChangerAction::Open);
  }
  setOutputs(outputSignals);
  auto outputs = readOutputSignals();
  if (!outputs) {
    std::string msg =
        "Unable to read status of output signals, tool changer status update "
        "failed!";
    SPDLOG_ERROR(msg);
    throw std::runtime_error(msg);
  }
}
void Machine::handleReconnectCommand(const cmd::ReconnectCommand& c) {
  MachineComponent* component = getComponent(c.robotComponent);
  if (!component) {
    auto msg = std::format("Resetting of '{}' is not implemented!",
                           magic_enum::enum_name(c.robotComponent));
    throw std::runtime_error(msg);
  }
  SPDLOG_INFO("Reconnecting {}...", magic_enum::enum_name(c.robotComponent));
  component->reset();
  try {
    component->initialize();
  } catch (const std::exception& e) {
    auto msg = std::format("Resetting '{}' failed: {}",
                           magic_enum::enum_name(c.robotComponent), e.what());
    throw std::runtime_error(msg);
  }
  if (component->state() != MachineComponent::State::Normal) {
    auto msg = std::format("Resetting '{}' was unsuccessful!",
                           magic_enum::enum_name(c.robotComponent));
    throw std::runtime_error(msg);
  }
  makeDummyStatus();
}

void Machine::shutdown() {
  SPDLOG_INFO("Shutting down. Joining Threads...");
  _isRunning = false;
  if (_commandServerThread.joinable()) _commandServerThread.join();
  if (_processThread.joinable()) _processThread.join();
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
  for (const auto& [componentType, component] : _components) {
    _robotStatus.robotComponents[componentType] = stateToLed(component->state());
  }

  const auto joystick = _controlPanel.getSnapshot();
  _robotStatus.joystics[utl::EArm::Left] = {
      .x = joystick.x[0], .y = joystick.y[0], .btn = joystick.b[0]};
  _robotStatus.joystics[utl::EArm::Right] = {
      .x = joystick.x[1], .y = joystick.y[1], .btn = joystick.b[1]};
  _robotStatus.joystics[utl::EArm::Gantry] = {
      .x = joystick.x[2], .y = joystick.y[2], .btn = joystick.b[2]};

  auto inputs = readInputSignals();
  auto outputs = readOutputSignals();
  if (inputs) {
    _robotStatus.toolChangers[utl::EArm::Left]
        .flags[utl::EToolChangerStatusFlags::ProxSen] =
        inputs->at("button1") ? utl::ELEDState::On : utl::ELEDState::Off;
    _robotStatus.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::ProxSen] =
        inputs->at("button2") ? utl::ELEDState::On : utl::ELEDState::Off;
  }
  if (outputs) {
    auto tclStatus = outputs->at("toolChangerLeft");
    auto tcrStatus = outputs->at("toolChangerRight");
    _robotStatus.toolChangers[utl::EArm::Left]
        .flags[utl::EToolChangerStatusFlags::ClosedValve] =
        tclStatus ? utl::ELEDState::Off : utl::ELEDState::On;
    _robotStatus.toolChangers[utl::EArm::Left]
        .flags[utl::EToolChangerStatusFlags::OpenValve] =
        tclStatus ? utl::ELEDState::On : utl::ELEDState::Off;

    _robotStatus.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::ClosedValve] =
        tcrStatus ? utl::ELEDState::Off : utl::ELEDState::On;
    _robotStatus.toolChangers[utl::EArm::Right]
        .flags[utl::EToolChangerStatusFlags::OpenValve] =
        tcrStatus ? utl::ELEDState::On : utl::ELEDState::Off;
  }

  _robotServer.publish(_robotStatus);
}

MachineComponent* Machine::getComponent(utl::ERobotComponent component) {
  auto it = _components.find(component);
  return it == _components.end() ? nullptr : it->second;
}

utl::ELEDState Machine::stateToLed(MachineComponent::State state) {
  return state == MachineComponent::State::Normal ? utl::ELEDState::On
                                                   : utl::ELEDState::Error;
}
