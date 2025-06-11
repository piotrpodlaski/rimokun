#pragma once
#include "spdlog/spdlog.h"

namespace utl {
// To be called in each top level executable
inline void configure_logger() {
  spdlog::set_level(
      static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
}
}  // namespace utl