#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

struct ArKd2RegisterEntry {
  std::uint16_t address;
  std::string_view name;
};

// Full AR-KD2 register-address list parsed from HM-60506E manual chapter 8
// (pages 227-242, "Register address list").
std::span<const ArKd2RegisterEntry> arKd2FullRegisterMap();

std::optional<std::string_view> arKd2RegisterName(std::uint16_t address);
