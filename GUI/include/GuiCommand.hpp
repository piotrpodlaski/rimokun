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

struct GuiRawCommand {
  nlohmann::json node;
};

struct GuiCommand {
  std::variant<GuiToolChangerCommand, GuiReconnectCommand, GuiRawCommand> payload;
};

struct GuiResponse {
  bool ok{false};
  std::string message;
  std::optional<nlohmann::json> payload;
};

Q_DECLARE_METATYPE(GuiCommand)
