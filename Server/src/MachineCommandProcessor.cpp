#include "MachineCommandProcessor.hpp"

#include <JsonExtensions.hpp>

#include <format>

using namespace std::chrono_literals;

nlohmann::json MachineCommandProcessor::processCommand(
    const nlohmann::json& command, const DispatchFn& dispatch) const {
  nlohmann::json response{
      {"status", "OK"},
      {"message", ""},
  };
  if (!command.is_object()) {
    response["status"] = "Error";
    response["message"] = "Command must be a map! Ignoring!";
    return response;
  }
  if (!command.contains("type") || !command.at("type").is_string()) {
    response["status"] = "Error";
    response["message"] = "Command lacks a valid 'type' entry! Ignoring!";
    return response;
  }

  const auto type = command.at("type").get<std::string>();
  if (type == "toolChanger") {
    if (!command.contains("position") || !command.contains("action")) {
      response["status"] = "Error";
      response["message"] =
          "toolChanger command requires both 'position' and 'action'.";
      return response;
    }
    try {
      cmd::Command c;
      c.payload = cmd::ToolChangerCommand(
          utl::enumFromJsonStringField<utl::EArm>(command, "position"),
          utl::enumFromJsonStringField<utl::EToolChangerAction>(command, "action"));
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
    if (!command.contains("system")) {
      response["status"] = "Error";
      response["message"] = "reset command requires a 'system' field.";
      return response;
    }
    try {
      cmd::Command c;
      c.payload = cmd::ReconnectCommand(
          utl::enumFromJsonStringField<utl::ERobotComponent>(command, "system"));
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

  if (type == "motorDiagnostics") {
    if (!command.contains("motor")) {
      response["status"] = "Error";
      response["message"] = "motorDiagnostics command requires a 'motor' field.";
      return response;
    }
    try {
      cmd::Command c;
      c.payload = cmd::MotorDiagnosticsCommand(
          utl::enumFromJsonStringField<utl::EMotor>(command, "motor"));
      const auto reply = dispatch(std::move(c), 2s);
      if (reply.empty()) {
        response["status"] = "Error";
        response["message"] =
            "motorDiagnostics command returned empty diagnostics payload.";
        return response;
      }
      try {
        response["response"] = nlohmann::json::parse(reply);
      } catch (...) {
        response["status"] = "Error";
        response["message"] = reply;
      }
    } catch (const std::exception& e) {
      response["status"] = "Error";
      response["message"] =
          std::format("Invalid motorDiagnostics command: {}", e.what());
    }
    return response;
  }

  if (type == "resetMotorAlarm") {
    if (!command.contains("motor")) {
      response["status"] = "Error";
      response["message"] = "resetMotorAlarm command requires a 'motor' field.";
      return response;
    }
    try {
      cmd::Command c;
      c.payload = cmd::ResetMotorAlarmCommand(
          utl::enumFromJsonStringField<utl::EMotor>(command, "motor"));
      const auto reply = dispatch(std::move(c), 2s);
      if (!reply.empty()) {
        response["status"] = "Error";
        response["message"] = reply;
      }
    } catch (const std::exception& e) {
      response["status"] = "Error";
      response["message"] =
          std::format("Invalid resetMotorAlarm command: {}", e.what());
    }
    return response;
  }

  if (type == "contecDiagnostics") {
    try {
      cmd::Command c;
      c.payload = cmd::ContecDiagnosticsCommand{};
      const auto reply = dispatch(std::move(c), 2s);
      if (reply.empty()) {
        response["status"] = "Error";
        response["message"] =
            "contecDiagnostics command returned empty diagnostics payload.";
        return response;
      }
      try {
        response["response"] = nlohmann::json::parse(reply);
      } catch (...) {
        response["status"] = "Error";
        response["message"] = reply;
      }
    } catch (const std::exception& e) {
      response["status"] = "Error";
      response["message"] =
          std::format("Invalid contecDiagnostics command: {}", e.what());
    }
    return response;
  }

  const std::string msg = std::format("Unknown command type '{}'!", type);
  response["status"] = "Error";
  response["message"] = msg;
  response["response"] = "YAY!";
  return response;
}
