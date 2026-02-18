#pragma once

#include <optional>

#include "GuiCommand.hpp"
#include "nlohmann/json.hpp"

class GuiCommandJsonAdapter {
 public:
  static nlohmann::json toJson(const GuiCommand& command);
  static std::optional<GuiResponse> fromJson(const nlohmann::json& response);
};
