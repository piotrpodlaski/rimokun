#include "Config.hpp"
#include <string>

using namespace std::string_literals;

namespace utl {

void Config::setConfigPath(const std::string& configPath) {
  this->_configPath = configPath;
  _topNode = YAML::LoadFile(configPath);
}
YAML::Node Config::getClassConfig(std::string_view className) const {
  if (!_topNode || !_topNode["classes"]) {
    throw std::runtime_error(std::format(
        "Config file '{}' is missing 'classes' section", _configPath));
  }

  YAML::Node classNode = _topNode["classes"][std::string(className)];
  if (!classNode || !classNode.IsDefined()) {
    throw std::runtime_error(std::format(
        "Failed to find entry for '{}' in the config file '{}'",
        className, _configPath));
  }

  return classNode;
}

}  // namespace utl