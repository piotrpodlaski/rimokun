#include "Config.hpp"

#include <string>

#include "ClassName.hpp"
#include "spdlog/spdlog.h"

using namespace std::string_literals;

namespace utl {

void Config::setConfigPath(const std::string& configPath) {
  this->_configPath = configPath;
  _topNode = YAML::LoadFile(configPath);
}
YAML::Node Config::getClassConfig(const std::string& className) {
  return _topNode["classes"][className];
}
}  // namespace utl