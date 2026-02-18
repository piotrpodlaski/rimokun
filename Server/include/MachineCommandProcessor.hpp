#pragma once

#include <chrono>
#include <functional>

#include "CommandInterface.hpp"
#include "CommonDefinitions.hpp"
#include "nlohmann/json.hpp"

class MachineCommandProcessor {
 public:
  using DispatchFn = std::function<std::string(cmd::Command, std::chrono::milliseconds)>;

  nlohmann::json processCommand(const nlohmann::json& command,
                                const DispatchFn& dispatch) const;
};
