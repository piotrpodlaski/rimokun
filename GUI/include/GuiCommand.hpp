#pragma once

#include <optional>
#include <variant>

#include <QMetaType>

#include "CommonDefinitions.hpp"
#include "nlohmann/json.hpp"

struct GuiToolChangerCommand {
  utl::EArm arm;
  utl::EToolChangerAction action;
};

struct GuiReconnectCommand {
  utl::ERobotComponent component;
};

struct GuiMotorDiagnosticsCommand {
  utl::EMotor motor;
};

struct GuiResetMotorAlarmCommand {
  utl::EMotor motor;
};

struct GuiRawCommand {
  nlohmann::json node;
};

struct GuiCommand {
  std::variant<GuiToolChangerCommand, GuiReconnectCommand,
               GuiMotorDiagnosticsCommand, GuiResetMotorAlarmCommand,
               GuiRawCommand>
      payload;
};

struct GuiResponse {
  bool ok{false};
  std::string message;
  std::optional<nlohmann::json> payload;
};

Q_DECLARE_METATYPE(GuiCommand)
