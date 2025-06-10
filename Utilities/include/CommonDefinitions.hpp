#pragma once

#include <yaml-cpp/yaml.h>

#include <magic_enum/magic_enum.hpp>
#include <map>
#include <string>

class VMotorStats;
namespace utl {
enum class EMotor { XLeft, XRight, YLeft, YRight, ZLeft, ZRight };

enum class ELEDState { On, Off, Error };
enum class EMotorStatusFlags { BrakeApplied, Enabled, Error };

std::string getMotorName(EMotor em);
EMotor getMotorType(std::string name);

struct SingleMotorStatus {
  double currentPosition{0};
  double targetPosition{0};
  double speed{0};
  int torque{0};
  std::map<EMotorStatusFlags, ELEDState> flags;
};

struct RobotStatus {
  std::map<EMotor, SingleMotorStatus> motors;
};

typedef std::map<EMotor, VMotorStats*> MotorStatsMap_t;

}  // namespace utl

namespace YAML {

// Primary template
template <typename T>
struct convert {
  static_assert(!std::is_enum_v<T>, "convert<T>: use convert_enum for enums");
};

// Generic enum converter
template <typename T>
struct convert_enum {
  static_assert(std::is_enum_v<T>, "convert_enum<T> requires an enum type");

  static Node encode(const T& rhs) {
    return Node(std::string(magic_enum::enum_name(rhs)));
  }

  static bool decode(const Node& node, T& rhs) {
    if (!node.IsScalar()) return false;
    auto opt = magic_enum::enum_cast<T>(node.as<std::string>());
    if (!opt.has_value()) return false;
    rhs = opt.value();
    return true;
  }
};

template <>
struct convert<utl::EMotor> : convert_enum<utl::EMotor> {};

template <>
struct convert<utl::ELEDState> : convert_enum<utl::ELEDState> {};

template <>
struct convert<utl::EMotorStatusFlags> : convert_enum<utl::EMotorStatusFlags> {
};

template <>
struct convert<utl::SingleMotorStatus> {
  static Node encode(const utl::SingleMotorStatus& rhs) {
    Node node;
    node["currentPosition"] = rhs.currentPosition;
    node["targetPosition"] = rhs.targetPosition;
    node["speed"] = rhs.speed;
    node["torque"] = rhs.torque;
    node["flags"] = rhs.flags;
    return node;
  }

  static bool decode(const Node& node, utl::SingleMotorStatus& rhs) {
    if (!node.IsMap()) return false;
    rhs.currentPosition = node["currentPosition"].as<double>();
    rhs.targetPosition = node["targetPosition"].as<double>();
    rhs.speed = node["speed"].as<double>();
    rhs.torque = node["torque"].as<int>();
    rhs.flags =
        node["flags"].as<std::map<utl::EMotorStatusFlags, utl::ELEDState>>();
    return true;
  }
};

template <>
struct convert<utl::RobotStatus> {
  static Node encode(const utl::RobotStatus& rhs) {
    Node node;
    for (const auto& [motor, status] : rhs.motors) {
      node[convert<utl::EMotor>::encode(motor)] = status;
    }
    return node;
  }

  static bool decode(const Node& node, utl::RobotStatus& rhs) {
    if (!node.IsMap()) return false;
    for (const auto& kv : node) {
      utl::EMotor motor;
      if (!convert<utl::EMotor>::decode(kv.first, motor)) return false;
      rhs.motors[motor] = kv.second.as<utl::SingleMotorStatus>();
    }
    return true;
  }
};

}  // namespace YAML