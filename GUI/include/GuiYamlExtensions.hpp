#pragma once

#include "GuiCommand.hpp"
#include "YamlExtensions.hpp"

namespace YAML {

template <>
struct convert<GuiToolChangerCommand> {
  static Node encode(const GuiToolChangerCommand& rhs) {
    Node node;
    node["type"] = "toolChanger";
    node["position"] = rhs.arm;
    node["action"] = rhs.action;
    return node;
  }

  static bool decode(const Node& node, GuiToolChangerCommand& rhs) {
    if (!node.IsMap()) return false;
    const auto type = node["type"];
    if (!type || !type.IsScalar() || type.as<std::string>() != "toolChanger") {
      return false;
    }
    if (!node["position"] || !node["action"]) return false;
    rhs.arm = node["position"].as<utl::EArm>();
    rhs.action = node["action"].as<utl::EToolChangerAction>();
    return true;
  }
};

template <>
struct convert<GuiReconnectCommand> {
  static Node encode(const GuiReconnectCommand& rhs) {
    Node node;
    node["type"] = "reset";
    node["system"] = rhs.component;
    return node;
  }

  static bool decode(const Node& node, GuiReconnectCommand& rhs) {
    if (!node.IsMap()) return false;
    const auto type = node["type"];
    if (!type || !type.IsScalar() || type.as<std::string>() != "reset") {
      return false;
    }
    if (!node["system"]) return false;
    rhs.component = node["system"].as<utl::ERobotComponent>();
    return true;
  }
};

template <>
struct convert<GuiRawCommand> {
  static Node encode(const GuiRawCommand& rhs) { return YAML::Clone(rhs.node); }

  static bool decode(const Node& node, GuiRawCommand& rhs) {
    rhs.node = YAML::Clone(node);
    return true;
  }
};

template <>
struct convert<GuiCommand> {
  static Node encode(const GuiCommand& rhs) {
    return std::visit(
        [](const auto& cmd) { return YAML::convert<std::decay_t<decltype(cmd)>>::encode(cmd); },
        rhs.payload);
  }

  static bool decode(const Node& node, GuiCommand& rhs) {
    if (!node.IsMap()) return false;
    const auto typeNode = node["type"];
    if (typeNode && typeNode.IsScalar()) {
      const auto type = typeNode.as<std::string>();
      try {
        if (type == "toolChanger") {
          rhs.payload = node.as<GuiToolChangerCommand>();
          return true;
        }
        if (type == "reset") {
          rhs.payload = node.as<GuiReconnectCommand>();
          return true;
        }
      } catch (const YAML::Exception&) {
        return false;
      }
    }
    rhs.payload = GuiRawCommand{.node = YAML::Clone(node)};
    return true;
  }
};

template <>
struct convert<GuiResponse> {
  static Node encode(const GuiResponse& rhs) {
    Node node;
    node["status"] = rhs.ok ? "OK" : "Error";
    node["message"] = rhs.message;
    if (rhs.payload.has_value()) {
      node["response"] = YAML::Clone(*rhs.payload);
    }
    return node;
  }

  static bool decode(const Node& node, GuiResponse& rhs) {
    if (!node.IsMap()) return false;
    rhs.ok = false;
    rhs.message.clear();
    rhs.payload.reset();

    const auto statusNode = node["status"];
    if (statusNode && statusNode.IsScalar()) {
      rhs.ok = (statusNode.as<std::string>() == "OK");
    }
    const auto messageNode = node["message"];
    if (messageNode && messageNode.IsScalar()) {
      rhs.message = messageNode.as<std::string>();
    }
    const auto payloadNode = node["response"];
    if (payloadNode && payloadNode.IsDefined()) {
      rhs.payload = YAML::Clone(payloadNode);
    }
    return true;
  }
};

}  // namespace YAML
