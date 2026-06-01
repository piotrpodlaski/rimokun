#pragma once

#include <optional>
#include <variant>

#include <QMetaType>

#include "CommonDefinitions.hpp"
#include "CommandInterface.hpp"
#include "nlohmann/json.hpp"

struct GuiRawCommand {
  nlohmann::json node;
};

struct GuiCommand {
  std::variant<cmd::ToolChangerCommand, cmd::ReconnectCommand,
               cmd::MotorDiagnosticsCommand, cmd::ResetMotorAlarmCommand,
               cmd::SetMotorEnabledCommand, cmd::SetAllMotorsEnabledCommand,
               cmd::HomingCommand, cmd::ContecDiagnosticsCommand, GuiRawCommand,
               cmd::EmergencyStopCommand>
      payload;
};

struct GuiResponse {
  bool ok{false};
  std::string message;
  std::optional<nlohmann::json> payload;
};

Q_DECLARE_METATYPE(GuiCommand)
