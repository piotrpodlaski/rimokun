#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>
#include <map>
#include <yaml-cpp/yaml.h>

namespace utl {
enum class EMotor { XLeft, XRight, YLeft, YRight, ZLeft, ZRight };

enum class ELEDState {On, Off, Error};

std::string getMotorName(EMotor em);
EMotor getMotorType(std::string name);

struct SingleMotorStatus {
  double currentPosition;
  double targetPosition;
  double speed;
  std::map<std::string, bool> flags;
};

struct RobotStatus {
  std::map<EMotor, SingleMotorStatus> motors;
};

}  // namespace utl

namespace YAML {
template <>
struct convert<utl::EMotor> {
  static Node encode(const utl::EMotor& rhs) {
    return Node(std::string(magic_enum::enum_name(rhs)));
  }

  static bool decode(const Node& node, utl::EMotor& rhs) {
    if (!node.IsScalar()) return false;
    const auto opt = magic_enum::enum_cast<utl::EMotor>(node.as<std::string>());
    if (!opt.has_value()) return false;
    rhs = opt.value();
    return true;
  }
};


template<>
struct convert<utl::SingleMotorStatus> {
  static Node encode(const utl::SingleMotorStatus& rhs) {
    Node node;
    node["currentPosition"] = rhs.currentPosition;
    node["targetPosition"] = rhs.targetPosition;
    node["speed"] = rhs.speed;
    node["flags"] = rhs.flags;
    return node;
  }

  static bool decode(const Node& node, utl::SingleMotorStatus& rhs) {
    if (!node.IsMap()) return false;
    rhs.currentPosition = node["currentPosition"].as<double>();
    rhs.targetPosition = node["targetPosition"].as<double>();
    rhs.speed = node["speed"].as<double>();
    rhs.flags = node["flags"].as<std::map<std::string, bool>>();
    return true;
  }
};

template<>
struct convert<utl::RobotStatus> {
  static Node encode(const utl::RobotStatus& rhs) {
    Node node;
    for (const auto& [motor, status] : rhs.motors) {
      node[YAML::convert<utl::EMotor>::encode(motor)] = status;
    }
    return node;
  }

  static bool decode(const Node& node, utl::RobotStatus& rhs) {
    if (!node.IsMap()) return false;
    for (const auto& kv : node) {
      utl::EMotor motor;
      if (!YAML::convert<utl::EMotor>::decode(kv.first, motor))
        return false;
      rhs.motors[motor] = kv.second.as<utl::SingleMotorStatus>();
    }
    return true;
  }
};

}