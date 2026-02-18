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
