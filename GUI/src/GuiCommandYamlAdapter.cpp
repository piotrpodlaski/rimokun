#include "GuiCommandYamlAdapter.hpp"

#include "GuiYamlExtensions.hpp"

YAML::Node GuiCommandYamlAdapter::toYaml(const GuiCommand& command) {
  return YAML::convert<GuiCommand>::encode(command);
}

std::optional<GuiResponse> GuiCommandYamlAdapter::fromYaml(
    const YAML::Node& response) {
  if (!response || !response.IsMap()) return std::nullopt;
  try {
    return response.as<GuiResponse>();
  } catch (const YAML::Exception&) {
    return std::nullopt;
  }
}
