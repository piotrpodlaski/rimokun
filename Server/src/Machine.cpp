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
}

signalMap_t Machine::getInputSignals() {
  signalMap_t inputSignals;
  auto inputs = _contec.readInputs();
  for (const auto& [signal, index] : _inputMapping) {
    inputSignals[signal] = inputs[index];
  }
  return inputSignals;
}

void Machine::setOutputs(signalMap_t signals) {
  auto outputs = _contec.readOutputs();
  for (const auto& [signal, value] : signals) {
    outputs[_outputMapping[signal]] = value;
  }
  _contec.setOutputs(outputs);
}

signalMap_t Machine::readInputSignals() {
  auto inputs = _contec.readInputs();
  signalMap_t inputSignals;
  for (auto& [signal, index] : _inputMapping) {
    inputSignals[signal] = inputs[index];
  }
  return inputSignals;
}
void Machine::processThread() {
  while (_isRunning) {
    signalMap_t inputs = readInputSignals();
    signalMap_t outputSignals;
    outputSignals["light1"] = inputs.at("button1");
    outputSignals["light2"] = inputs.at("button2");
    std::this_thread::sleep_for(_loopSleepTime);
  }
}
void Machine::init() {}
void Machine::handleCommandsThread() {
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
        if (reply != "") {
          response["status"] = "OK";
          response["message"] = "";
        } else {
          response["status"] = "Error";
          response["message"] = reply;
        }
      }

      _robotServer.sendResponse(response);
    }
  }
}
