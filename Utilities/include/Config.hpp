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
  // template <typename T>
  // YAML::Node getClassConfig(T obj) {
  //   if (obj == nullptr) return {};
  //   if (_configPath.empty()) {
  //     constexpr auto msg =
  //         "Config path is empty! Initialize the path first before calling "
  //         "::setConfigPath method!";
  //     SPDLOG_ERROR((msg));
  //     throw std::runtime_error(msg);
  //   }
  //   auto className = getClassName(obj);
  //   std::cout<<className<<std::endl;
  //   return _topNode["classes"][className];
  // }

  YAML::Node getClassConfig(const std::string& className);

 private:
  Config() = default;
  ~Config() = default;

  std::string _configPath{};
  YAML::Node _topNode;
  friend class Singleton;
};
}  // namespace utl