#include "GuiCommandJsonAdapter.hpp"

#include <JsonExtensions.hpp>

nlohmann::json GuiCommandJsonAdapter::toJson(const GuiCommand& command) {
  return std::visit(
      [](const auto& cmd) -> nlohmann::json {
        using CmdT = std::decay_t<decltype(cmd)>;
        if constexpr (std::is_same_v<CmdT, GuiToolChangerCommand>) {
          return nlohmann::json{
              {"type", "toolChanger"},
              {"position", utl::enumToString(cmd.arm)},
              {"action", utl::enumToString(cmd.action)},
          };
        } else if constexpr (std::is_same_v<CmdT, GuiReconnectCommand>) {
          return nlohmann::json{
              {"type", "reset"},
              {"system", utl::enumToString(cmd.component)},
          };
        } else if constexpr (std::is_same_v<CmdT, GuiMotorDiagnosticsCommand>) {
          return nlohmann::json{
              {"type", "motorDiagnostics"},
              {"motor", utl::enumToString(cmd.motor)},
          };
        } else if constexpr (std::is_same_v<CmdT, GuiResetMotorAlarmCommand>) {
          return nlohmann::json{
              {"type", "resetMotorAlarm"},
              {"motor", utl::enumToString(cmd.motor)},
          };
        } else if constexpr (std::is_same_v<CmdT, GuiSetMotorEnabledCommand>) {
          return nlohmann::json{
              {"type", "setMotorEnabled"},
              {"motor", utl::enumToString(cmd.motor)},
              {"enabled", cmd.enabled},
          };
        } else if constexpr (std::is_same_v<CmdT, GuiSetAllMotorsEnabledCommand>) {
          return nlohmann::json{
              {"type", "setAllMotorsEnabled"},
              {"enabled", cmd.enabled},
          };
        } else if constexpr (std::is_same_v<CmdT, GuiContecDiagnosticsCommand>) {
          return nlohmann::json{
              {"type", "contecDiagnostics"},
          };
        } else {
          return cmd.node;
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
