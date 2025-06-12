#include "Config.hpp"

#include <string>

#include "ClassName.hpp"
#include "spdlog/spdlog.h"

using namespace std::string_literals;

namespace utl {

void Config::setConfigPath(const std::string& configPath) {
  this->configPath = configPath;
  topNode = YAML::LoadFile(configPath);
}
}  // namespace utl