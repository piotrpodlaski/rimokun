#pragma once

#include <iostream>
#include <string>

#include "ClassName.hpp"
#include "Logger.hpp"
#include "Singleton.hpp"
#include "yaml-cpp/yaml.h"

namespace utl {
class Config : public Singleton<Config> {
public:
  void setConfigPath(const std::string& configPath);


  YAML::Node getClassConfig(std::string_view className) const;

  template <typename T>
  T getRequired(std::string_view className,
                        std::string_view key) const {
    YAML::Node classNode = getClassConfig(className);

    YAML::Node valueNode = classNode[std::string(key)];
    if (!valueNode || !valueNode.IsDefined()) {
      throw std::runtime_error(std::format(
          "Missing required key '{}.{}' in config file '{}'",
          className, key, _configPath));
    }

    try {
      return valueNode.as<T>();
    } catch (const YAML::Exception& e) {
      throw std::runtime_error(std::format(
          "Invalid type for '{}.{}' in config file '{}': {}",
          className, key, _configPath, e.what()));
    }
  }

  template <typename T>
  T getOptional(std::string_view className,
                        std::string_view key,
                        T defaultValue) const {
    YAML::Node classNode = getClassConfig(className);

    YAML::Node valueNode = classNode[std::string(key)];
    if (!valueNode || !valueNode.IsDefined()) {
      return defaultValue;
    }

    try {
      return valueNode.as<T>();
    } catch (const YAML::Exception& e) {
      throw std::runtime_error(std::format(
          "Invalid type for optional '{}.{}' in '{}': {}",
          className, key, _configPath, e.what()));
    }
  }

private:
  Config() = default;
  ~Config() = default;

  std::string _configPath{};
  YAML::Node _topNode;
  friend class Singleton;
};
}  // namespace utl