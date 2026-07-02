#include "MachineCommandProcessor.hpp"

#include <JsonExtensions.hpp>

#include <exception>
#include <format>
#include <string>
#include <unordered_map>
#include <utility>

using namespace std::chrono_literals;

namespace {
using HandlerFn = nlohmann::json (*)(const nlohmann::json&, const cmd::DispatchFn&);

nlohmann::json okResponse() {
  return nlohmann::json{{"status", "OK"}, {"message", ""}};
}

nlohmann::json errorResponse(const std::string& message) {
  return nlohmann::json{{"status", "Error"}, {"message", message}};
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

void applyDispatchReply(nlohmann::json& response, const std::string& reply) {
  if (!reply.empty()) {
    response["status"] = "Error";
    response["message"] = reply;
  }
}

void applyJsonReply(nlohmann::json& response, const std::string& reply,
                    const std::string& emptyMessage) {
  if (reply.empty()) {
    response["status"] = "Error";
    response["message"] = emptyMessage;
    return;
  }
  try {
    response["response"] = nlohmann::json::parse(reply);
  } catch (...) {
    response["status"] = "Error";
    response["message"] = reply;
  }
}

nlohmann::json handleToolChanger(const nlohmann::json& command,
                                 const cmd::DispatchFn& dispatch) {
  if (!command.contains("position") || !command.contains("action")) {
    return errorResponse(
        "toolChanger command requires both 'position' and 'action'.");
  }
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::ToolChangerCommand(
        utl::enumFromJsonStringField<utl::EArm>(command, "position"),
        utl::enumFromJsonStringField<utl::EToolChangerAction>(command, "action"));
    applyDispatchReply(response, dispatch(std::move(c), 2s));
  } catch (const std::exception& e) {
    response = errorResponse(std::format("Invalid toolChanger command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleReset(const nlohmann::json& command,
                           const cmd::DispatchFn& dispatch) {
  if (!command.contains("system")) {
    return errorResponse("reset command requires a 'system' field.");
  }
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::ReconnectCommand(
        utl::enumFromJsonStringField<utl::ERobotComponent>(command, "system"));
    applyDispatchReply(response, dispatch(std::move(c), 2s));
  } catch (const std::exception& e) {
    response = errorResponse(std::format("Invalid reset command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleMotorDiagnostics(const nlohmann::json& command,
                                      const cmd::DispatchFn& dispatch) {
  if (!command.contains("motor")) {
    return errorResponse("motorDiagnostics command requires a 'motor' field.");
  }
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::MotorDiagnosticsCommand(
        utl::enumFromJsonStringField<utl::EMotor>(command, "motor"));
    applyJsonReply(response, dispatch(std::move(c), 2s),
                   "motorDiagnostics command returned empty diagnostics payload.");
  } catch (const std::exception& e) {
    response =
        errorResponse(std::format("Invalid motorDiagnostics command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleResetMotorAlarm(const nlohmann::json& command,
                                     const cmd::DispatchFn& dispatch) {
  if (!command.contains("motor")) {
    return errorResponse("resetMotorAlarm command requires a 'motor' field.");
  }
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::ResetMotorAlarmCommand(
        utl::enumFromJsonStringField<utl::EMotor>(command, "motor"));
    applyDispatchReply(response, dispatch(std::move(c), 2s));
  } catch (const std::exception& e) {
    response =
        errorResponse(std::format("Invalid resetMotorAlarm command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleContecDiagnostics(const nlohmann::json&,
                                       const cmd::DispatchFn& dispatch) {
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::ContecDiagnosticsCommand{};
    applyJsonReply(response, dispatch(std::move(c), 2s),
                   "contecDiagnostics command returned empty diagnostics payload.");
  } catch (const std::exception& e) {
    response =
        errorResponse(std::format("Invalid contecDiagnostics command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleSetMotorEnabled(const nlohmann::json& command,
                                     const cmd::DispatchFn& dispatch) {
  if (!command.contains("motor") || !command.contains("enabled") ||
      !command.at("enabled").is_boolean()) {
    return errorResponse(
        "setMotorEnabled command requires 'motor' and boolean 'enabled'.");
  }
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::SetMotorEnabledCommand{
        .motor = utl::enumFromJsonStringField<utl::EMotor>(command, "motor"),
        .enabled = command.at("enabled").get<bool>()};
    applyDispatchReply(response, dispatch(std::move(c), 2s));
  } catch (const std::exception& e) {
    response =
        errorResponse(std::format("Invalid setMotorEnabled command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleSetAllMotorsEnabled(const nlohmann::json& command,
                                         const cmd::DispatchFn& dispatch) {
  if (!command.contains("enabled") || !command.at("enabled").is_boolean()) {
    return errorResponse(
        "setAllMotorsEnabled command requires boolean 'enabled'.");
  }
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::SetAllMotorsEnabledCommand{
        .enabled = command.at("enabled").get<bool>()};
    applyDispatchReply(response, dispatch(std::move(c), 2s));
  } catch (const std::exception& e) {
    response = errorResponse(
        std::format("Invalid setAllMotorsEnabled command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleHoming(const nlohmann::json& command,
                            const cmd::DispatchFn& dispatch) {
  if (!command.contains("target") || !command.at("target").is_string()) {
    return errorResponse("homing command requires a 'target' field.");
  }
  auto response = okResponse();
  try {
    const auto targetName = command.at("target").get<std::string>();
    const auto target = parseHomingTarget(targetName);
    if (!target.has_value()) {
      throw std::runtime_error(std::format("Unknown homing target '{}'", targetName));
    }

    cmd::Command c;
    c.payload = cmd::HomingCommand{.target = *target};
    applyDispatchReply(response, dispatch(std::move(c), 2s));
  } catch (const std::exception& e) {
    response = errorResponse(std::format("Invalid homing command: {}", e.what()));
  }
  return response;
}

nlohmann::json handleEmergencyStop(const nlohmann::json&,
                                   const cmd::DispatchFn& dispatch) {
  auto response = okResponse();
  try {
    cmd::Command c;
    c.payload = cmd::EmergencyStopCommand{};
    applyDispatchReply(response, dispatch(std::move(c), 2s));
  } catch (const std::exception& e) {
    response =
        errorResponse(std::format("emergencyStop command failed: {}", e.what()));
  }
  return response;
}

const std::unordered_map<std::string, HandlerFn>& handlers() {
  static const std::unordered_map<std::string, HandlerFn> table{
      {"toolChanger", handleToolChanger},
      {"reset", handleReset},
      {"motorDiagnostics", handleMotorDiagnostics},
      {"resetMotorAlarm", handleResetMotorAlarm},
      {"contecDiagnostics", handleContecDiagnostics},
      {"setMotorEnabled", handleSetMotorEnabled},
      {"setAllMotorsEnabled", handleSetAllMotorsEnabled},
      {"homing", handleHoming},
      {"emergencyStop", handleEmergencyStop},
  };
  return table;
}
}  // namespace

nlohmann::json cmd::processCommand(const nlohmann::json& command,
                                   const cmd::DispatchFn& dispatch) {
  if (!command.is_object()) {
    return errorResponse("Command must be a map! Ignoring!");
  }
  if (!command.contains("type") || !command.at("type").is_string()) {
    return errorResponse("Command lacks a valid 'type' entry! Ignoring!");
  }

  const auto type = command.at("type").get<std::string>();
  const auto it = handlers().find(type);
  if (it == handlers().end()) {
    return errorResponse(std::format("Unknown command type '{}'!", type));
  }
  return it->second(command, dispatch);
}
