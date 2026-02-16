#pragma once

#include <chrono>
#include <functional>

#include "CommandInterface.hpp"
#include "CommonDefinitions.hpp"
#include "yaml-cpp/yaml.h"

class MachineCommandProcessor {
 public:
  using DispatchFn = std::function<std::string(cmd::Command, std::chrono::milliseconds)>;

  YAML::Node processCommand(const YAML::Node& command,
                            const DispatchFn& dispatch) const;
};
