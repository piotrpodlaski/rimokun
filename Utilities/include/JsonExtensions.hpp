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

template <typename TEnum>
TEnum enumFromJsonStringField(const nlohmann::json& in, const char* key) {
  if (!in.contains(key) || !in.at(key).is_string()) {
    throw std::runtime_error(std::string("Missing or invalid '") + key + "' entry");
  }
  return stringToEnum<TEnum>(in.at(key).get<std::string>());
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
        {"state", v.state},
        {"warningDescription", v.warningDescription},
        {"alarmDescription", v.alarmDescription},
        {"flags", utl::enumKeyedMapToJson(v.flags)},
    };
  }

  static void from_json(const json& j, utl::SingleMotorStatus& v) {
    v.currentPosition = j.at("currentPosition").get<double>();
    v.targetPosition = j.at("targetPosition").get<double>();
    v.speed = j.at("speed").get<double>();
    v.torque = j.at("torque").get<int>();
    v.state = j.contains("state") ? j.at("state").get<utl::ELEDState>()
                                   : utl::ELEDState::Off;
    v.warningDescription = j.value("warningDescription", std::string{});
    v.alarmDescription = j.value("alarmDescription", std::string{});
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
