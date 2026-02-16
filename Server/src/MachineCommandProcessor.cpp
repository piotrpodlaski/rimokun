#include "MachineCommandProcessor.hpp"

#include <format>

using namespace std::chrono_literals;

YAML::Node MachineCommandProcessor::processCommand(const YAML::Node& command,
                                                   const DispatchFn& dispatch) const {
  YAML::Node response;
  response["status"] = "OK";
  response["message"] = "";
  if (!command.IsMap()) {
    response["status"] = "Error";
    response["message"] = "Command must be a map! Ignoring!";
    return response;
  }
  const auto typeNode = command["type"];
  if (!typeNode || !typeNode.IsScalar()) {
    response["status"] = "Error";
    response["message"] = "Command lacks a valid 'type' entry! Ignoring!";
    return response;
  }

  const auto type = typeNode.as<std::string>();
  if (type == "toolChanger") {
    if (!command["position"] || !command["action"]) {
      response["status"] = "Error";
      response["message"] =
          "toolChanger command requires both 'position' and 'action'.";
      return response;
    }
    try {
      cmd::Command c;
      c.payload = cmd::ToolChangerCommand(
          command["position"].as<utl::EArm>(),
          command["action"].as<utl::EToolChangerAction>());
      const auto reply = dispatch(std::move(c), 2s);
      if (!reply.empty()) {
        response["status"] = "Error";
        response["message"] = reply;
      }
    } catch (const std::exception& e) {
      response["status"] = "Error";
      response["message"] = std::format("Invalid toolChanger command: {}", e.what());
    }
    return response;
  }

  if (type == "reset") {
    if (!command["system"]) {
      response["status"] = "Error";
      response["message"] = "reset command requires a 'system' field.";
      return response;
    }
    try {
      cmd::Command c;
      c.payload = cmd::ReconnectCommand(command["system"].as<utl::ERobotComponent>());
      const auto reply = dispatch(std::move(c), 2s);
      if (!reply.empty()) {
        response["status"] = "Error";
        response["message"] = reply;
      }
    } catch (const std::exception& e) {
      response["status"] = "Error";
      response["message"] = std::format("Invalid reset command: {}", e.what());
    }
    return response;
  }

  const std::string msg = std::format("Unknown command type '{}'!", type);
  response["status"] = "Error";
  response["message"] = msg;
  response["response"] = "YAY!";
  return response;
}
