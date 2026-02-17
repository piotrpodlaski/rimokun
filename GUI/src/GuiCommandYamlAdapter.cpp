#include "GuiCommandYamlAdapter.hpp"

#include "YamlExtensions.hpp"

namespace {
template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;
}  // namespace

YAML::Node GuiCommandYamlAdapter::toYaml(const GuiCommand& command) {
  return std::visit(
      Overloaded{
          [](const GuiToolChangerCommand& c) {
            YAML::Node node;
            node["type"] = "toolChanger";
            node["position"] = c.arm;
            node["action"] = c.action;
            return node;
          },
          [](const GuiReconnectCommand& c) {
            YAML::Node node;
            node["type"] = "reset";
            node["system"] = c.component;
            return node;
          },
          [](const GuiRawCommand& c) { return YAML::Clone(c.node); }},
      command.payload);
}

std::optional<GuiResponse> GuiCommandYamlAdapter::fromYaml(
    const YAML::Node& response) {
  if (!response || !response.IsMap()) {
    return std::nullopt;
  }
  GuiResponse out;
  const auto statusNode = response["status"];
  if (statusNode && statusNode.IsScalar()) {
    const auto status = statusNode.as<std::string>();
    out.ok = (status == "OK");
  }
  const auto messageNode = response["message"];
  if (messageNode && messageNode.IsScalar()) {
    out.message = messageNode.as<std::string>();
  }
  const auto payloadNode = response["response"];
  if (payloadNode && payloadNode.IsDefined()) {
    out.payload = YAML::Clone(payloadNode);
  }
  return out;
}
