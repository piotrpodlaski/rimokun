#pragma once

#include <optional>
#include <variant>

#include "CommonDefinitions.hpp"
#include "yaml-cpp/yaml.h"

struct GuiToolChangerCommand {
  utl::EArm arm;
  utl::EToolChangerAction action;
};

struct GuiReconnectCommand {
  utl::ERobotComponent component;
};

struct GuiRawCommand {
  YAML::Node node;
};

struct GuiCommand {
  std::variant<GuiToolChangerCommand, GuiReconnectCommand, GuiRawCommand> payload;
};

struct GuiResponse {
  bool ok{false};
  std::string message;
  std::optional<YAML::Node> payload;
};
