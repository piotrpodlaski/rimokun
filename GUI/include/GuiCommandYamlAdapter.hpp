#pragma once

#include "GuiCommand.hpp"

class GuiCommandYamlAdapter {
 public:
  static YAML::Node toYaml(const GuiCommand& command);
  static std::optional<GuiResponse> fromYaml(const YAML::Node& response);
};
