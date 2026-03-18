#pragma once

#include <CommandInterface.hpp>
#include <nlohmann/json.hpp>

namespace cmd {
nlohmann::json processCommand(const nlohmann::json& command, const DispatchFn& dispatch);
}
