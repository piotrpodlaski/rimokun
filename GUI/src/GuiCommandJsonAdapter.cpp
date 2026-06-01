#include "GuiCommandJsonAdapter.hpp"

#include <JsonExtensions.hpp>

namespace {
std::string homingTargetToString(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::All: return "All";
    case cmd::HomingTarget::XLeft: return "XLeft";
    case cmd::HomingTarget::YLeft: return "YLeft";
    case cmd::HomingTarget::XRight: return "XRight";
    case cmd::HomingTarget::YRight: return "YRight";
    case cmd::HomingTarget::Z: return "Z";
  }
  return "All";
}
}  // namespace

nlohmann::json GuiCommandJsonAdapter::toJson(const GuiCommand& command) {
  return std::visit(
      [](const auto& payload) -> nlohmann::json {
        using CmdT = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<CmdT, cmd::ToolChangerCommand>) {
          return nlohmann::json{
              {"type", "toolChanger"},
              {"position", utl::enumToString(payload.arm)},
              {"action", utl::enumToString(payload.action)},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::ReconnectCommand>) {
          return nlohmann::json{
              {"type", "reset"},
              {"system", utl::enumToString(payload.robotComponent)},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::MotorDiagnosticsCommand>) {
          return nlohmann::json{
              {"type", "motorDiagnostics"},
              {"motor", utl::enumToString(payload.motor)},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::ResetMotorAlarmCommand>) {
          return nlohmann::json{
              {"type", "resetMotorAlarm"},
              {"motor", utl::enumToString(payload.motor)},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::SetMotorEnabledCommand>) {
          return nlohmann::json{
              {"type", "setMotorEnabled"},
              {"motor", utl::enumToString(payload.motor)},
              {"enabled", payload.enabled},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::SetAllMotorsEnabledCommand>) {
          return nlohmann::json{
              {"type", "setAllMotorsEnabled"},
              {"enabled", payload.enabled},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::HomingCommand>) {
          return nlohmann::json{
              {"type", "homing"},
              {"target", homingTargetToString(payload.target)},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::ContecDiagnosticsCommand>) {
          return nlohmann::json{
              {"type", "contecDiagnostics"},
          };
        } else if constexpr (std::is_same_v<CmdT, cmd::EmergencyStopCommand>) {
          return nlohmann::json{
              {"type", "emergencyStop"},
          };
        } else {
          return payload.node;
        }
      },
      command.payload);
}

std::optional<GuiResponse> GuiCommandJsonAdapter::fromJson(
    const nlohmann::json& response) {
  if (!response.is_object()) return std::nullopt;

  GuiResponse out;
  if (response.contains("status") && response["status"].is_string()) {
    out.ok = (response["status"].get<std::string>() == "OK");
  }
  if (response.contains("message") && response["message"].is_string()) {
    out.message = response["message"].get<std::string>();
  }
  if (response.contains("response")) {
    out.payload = response["response"];
  }
  return out;
}
