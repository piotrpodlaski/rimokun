#pragma once

#include <string>

#include "ClassName.hpp"
#include "Singleton.hpp"
#include "logger.hpp"
#include "yaml-cpp/yaml.h"

namespace utl {
class Config : public Singleton<Config> {
 public:
  void setConfigPath(const std::string& configPath);
  template <typename T>
  YAML::Node getConfig(T obj) {
    if (obj == nullptr) return {};
    if (configPath.empty()) {
      constexpr auto msg =
          "Config path is empty! Initialize the path first bu calling "
          "::setConfigPath method!";
      SPDLOG_ERROR((msg));
      throw std::runtime_error(msg);
    }
    auto className = getClassName(obj);
    return topNode["classes"][className];
  }

 private:
  Config() = default;
  ~Config() = default;

  std::string configPath{};
  YAML::Node topNode;
  friend class Singleton;
};
}  // namespace utl