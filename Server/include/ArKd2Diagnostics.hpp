#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

enum class ArKd2CodeDomain {
  Alarm,
  Warning,
  CommunicationError,
};

struct ArKd2CodeInfo {
  std::uint8_t code;
  std::string_view type;
  std::string_view cause;
  std::string_view remedialAction;
};

std::optional<ArKd2CodeInfo> arKd2FindAlarm(std::uint8_t code);
std::optional<ArKd2CodeInfo> arKd2FindWarning(std::uint8_t code);
std::optional<ArKd2CodeInfo> arKd2FindCommunicationError(std::uint8_t code);

std::optional<ArKd2CodeInfo> arKd2FindCode(ArKd2CodeDomain domain,
                                           std::uint8_t code);

std::string_view arKd2DomainName(ArKd2CodeDomain domain);

