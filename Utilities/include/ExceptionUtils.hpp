#pragma once

#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

namespace utl {
namespace detail {
inline std::string_view baseName(const std::string_view path) {
  const auto slashPos = path.find_last_of("/\\");
  if (slashPos == std::string_view::npos) {
    return path;
  }
  return path.substr(slashPos + 1);
}
}  // namespace detail

[[noreturn]] inline void throwRuntimeError(
    std::string message,
    const std::source_location location = std::source_location::current()) {
  SPDLOG_ERROR("Throwing std::runtime_error at {}:{} in {}: {}",
               detail::baseName(location.file_name()), location.line(),
               location.function_name(), message);
  throw std::runtime_error(std::move(message));
}

}  // namespace utl
