#pragma once

#include <CommonDefinitions.hpp>

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

namespace utl {

template <typename TEnum>
std::string enumToString(const TEnum value) {
  static_assert(std::is_enum_v<TEnum>);
  return std::string(magic_enum::enum_name(value));
}

template <typename TEnum>
TEnum stringToEnum(const std::string& text) {
  static_assert(std::is_enum_v<TEnum>);
  const auto parsed = magic_enum::enum_cast<TEnum>(text);
  if (!parsed.has_value()) {
    throw std::runtime_error("Invalid enum value: " + text);
  }
  return *parsed;
}

inline nlohmann::json yamlScalarToJson(const YAML::Node& node) {
  const auto scalar = node.as<std::string>();
  if (scalar == "true" || scalar == "True" || scalar == "TRUE") {
    return true;
  }
  if (scalar == "false" || scalar == "False" || scalar == "FALSE") {
    return false;
  }
  std::int64_t asInt = 0;
  {
    const auto* begin = scalar.data();
    const auto* end = scalar.data() + scalar.size();
    if (auto [ptr, ec] = std::from_chars(begin, end, asInt);
        ec == std::errc{} && ptr == end) {
      return asInt;
    }
  }
  {
    char* parsedEnd = nullptr;
    const double asDouble = std::strtod(scalar.c_str(), &parsedEnd);
    if (parsedEnd == scalar.c_str() + scalar.size() && std::isfinite(asDouble)) {
      return asDouble;
    }
  }
  return scalar;
}

inline nlohmann::json yamlNodeToJson(const YAML::Node& node) {
  if (!node || !node.IsDefined()) {
    return nullptr;
  }
  if (node.IsMap()) {
    nlohmann::json out = nlohmann::json::object();
    for (const auto& kv : node) {
      out[kv.first.as<std::string>()] = yamlNodeToJson(kv.second);
    }
    return out;
  }
  if (node.IsSequence()) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& item : node) {
      out.push_back(yamlNodeToJson(item));
    }
    return out;
  }
  if (node.IsScalar()) {
    return yamlScalarToJson(node);
  }
  return nullptr;
}

inline YAML::Node jsonToYamlNode(const nlohmann::json& value) {
  if (value.is_null()) {
    return {};
  }
  if (value.is_boolean()) {
    return YAML::Node(value.get<bool>());
  }
  if (value.is_number_integer()) {
    return YAML::Node(value.get<std::int64_t>());
  }
  if (value.is_number_unsigned()) {
    return YAML::Node(value.get<std::uint64_t>());
  }
  if (value.is_number_float()) {
    return YAML::Node(value.get<double>());
  }
  if (value.is_string()) {
    return YAML::Node(value.get<std::string>());
  }
  if (value.is_array()) {
    YAML::Node out(YAML::NodeType::Sequence);
    for (const auto& item : value) {
      out.push_back(jsonToYamlNode(item));
    }
    return out;
  }
  YAML::Node out(YAML::NodeType::Map);
  for (const auto& [key, item] : value.items()) {
    out[key] = jsonToYamlNode(item);
  }
  return out;
}

template <typename TEnum, typename TValue>
nlohmann::json enumKeyedMapToJson(const std::map<TEnum, TValue>& map) {
  nlohmann::json out = nlohmann::json::object();
  for (const auto& [k, v] : map) {
    out[enumToString(k)] = v;
  }
  return out;
}

template <typename TEnum, typename TValue>
std::map<TEnum, TValue> enumKeyedMapFromJson(const nlohmann::json& in) {
  std::map<TEnum, TValue> out;
  if (!in.is_object()) {
    throw std::runtime_error("Expected JSON object for enum-keyed map.");
  }
  for (const auto& [k, v] : in.items()) {
    out.emplace(stringToEnum<TEnum>(k), v.get<TValue>());
  }
  return out;
}

}  // namespace utl

namespace nlohmann {

template <>
struct adl_serializer<utl::ELEDState> {
  static void to_json(json& j, const utl::ELEDState& v) { j = utl::enumToString(v); }
  static void from_json(const json& j, utl::ELEDState& v) {
    v = utl::stringToEnum<utl::ELEDState>(j.get<std::string>());
  }
};

template <>
struct adl_serializer<utl::EMotorStatusFlags> {
  static void to_json(json& j, const utl::EMotorStatusFlags& v) { j = utl::enumToString(v); }
  static void from_json(const json& j, utl::EMotorStatusFlags& v) {
    v = utl::stringToEnum<utl::EMotorStatusFlags>(j.get<std::string>());
  }
};

template <>
struct adl_serializer<utl::EMotor> {
  static void to_json(json& j, const utl::EMotor& v) { j = utl::enumToString(v); }
  static void from_json(const json& j, utl::EMotor& v) {
    v = utl::stringToEnum<utl::EMotor>(j.get<std::string>());
  }
};

template <>
struct adl_serializer<utl::EArm> {
  static void to_json(json& j, const utl::EArm& v) { j = utl::enumToString(v); }
  static void from_json(const json& j, utl::EArm& v) {
    v = utl::stringToEnum<utl::EArm>(j.get<std::string>());
  }
};

template <>
struct adl_serializer<utl::EToolChangerStatusFlags> {
  static void to_json(json& j, const utl::EToolChangerStatusFlags& v) { j = utl::enumToString(v); }
  static void from_json(const json& j, utl::EToolChangerStatusFlags& v) {
    v = utl::stringToEnum<utl::EToolChangerStatusFlags>(j.get<std::string>());
  }
};

template <>
struct adl_serializer<utl::ERobotComponent> {
  static void to_json(json& j, const utl::ERobotComponent& v) { j = utl::enumToString(v); }
  static void from_json(const json& j, utl::ERobotComponent& v) {
    v = utl::stringToEnum<utl::ERobotComponent>(j.get<std::string>());
  }
};

template <>
struct adl_serializer<utl::SingleMotorStatus> {
  static void to_json(json& j, const utl::SingleMotorStatus& v) {
    j = json{
        {"currentPosition", v.currentPosition},
        {"targetPosition", v.targetPosition},
        {"speed", v.speed},
        {"torque", v.torque},
        {"flags", utl::enumKeyedMapToJson(v.flags)},
    };
  }

  static void from_json(const json& j, utl::SingleMotorStatus& v) {
    v.currentPosition = j.at("currentPosition").get<double>();
    v.targetPosition = j.at("targetPosition").get<double>();
    v.speed = j.at("speed").get<double>();
    v.torque = j.at("torque").get<int>();
    v.flags = utl::enumKeyedMapFromJson<utl::EMotorStatusFlags, utl::ELEDState>(
        j.at("flags"));
  }
};

template <>
struct adl_serializer<utl::ToolChangerStatus> {
  static void to_json(json& j, const utl::ToolChangerStatus& v) {
    j = json{{"flags", utl::enumKeyedMapToJson(v.flags)}};
  }

  static void from_json(const json& j, utl::ToolChangerStatus& v) {
    v.flags = utl::enumKeyedMapFromJson<utl::EToolChangerStatusFlags, utl::ELEDState>(
        j.at("flags"));
  }
};

template <>
struct adl_serializer<utl::JoystickStatus> {
  static void to_json(json& j, const utl::JoystickStatus& v) {
    j = json{{"x", v.x}, {"y", v.y}, {"btn", v.btn}};
  }

  static void from_json(const json& j, utl::JoystickStatus& v) {
    v.x = j.at("x").get<double>();
    v.y = j.at("y").get<double>();
    v.btn = j.at("btn").get<bool>();
  }
};

template <>
struct adl_serializer<utl::RobotStatus> {
  static void to_json(json& j, const utl::RobotStatus& v) {
    j = json{
        {"motors", utl::enumKeyedMapToJson(v.motors)},
        {"toolChangers", utl::enumKeyedMapToJson(v.toolChangers)},
        {"robotComponents", utl::enumKeyedMapToJson(v.robotComponents)},
        {"joystics", utl::enumKeyedMapToJson(v.joystics)},
    };
  }

  static void from_json(const json& j, utl::RobotStatus& v) {
    v.motors = utl::enumKeyedMapFromJson<utl::EMotor, utl::SingleMotorStatus>(
        j.at("motors"));
    v.toolChangers =
        utl::enumKeyedMapFromJson<utl::EArm, utl::ToolChangerStatus>(
            j.at("toolChangers"));
    v.robotComponents =
        utl::enumKeyedMapFromJson<utl::ERobotComponent, utl::ELEDState>(
            j.at("robotComponents"));
    v.joystics = utl::enumKeyedMapFromJson<utl::EArm, utl::JoystickStatus>(
        j.at("joystics"));
  }
};

}  // namespace nlohmann
