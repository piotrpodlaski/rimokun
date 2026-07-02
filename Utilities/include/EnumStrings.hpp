#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
#include <type_traits>

#define RIMOKUN_SKIP_YAML_EXTENSIONS
#include "CommonDefinitions.hpp"
#undef RIMOKUN_SKIP_YAML_EXTENSIONS

#if __has_include(<libserial/SerialPortConstants.h>)
#include <libserial/SerialPortConstants.h>
#endif

namespace utl::detail {
std::string_view enumToStringImpl(EMotor value);
std::optional<EMotor> enumFromStringImpl(std::type_identity<EMotor>,
                                         std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EMotor>);

std::string_view enumToStringImpl(ELEDState value);
std::optional<ELEDState> enumFromStringImpl(std::type_identity<ELEDState>,
                                            std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<ELEDState>);

std::string_view enumToStringImpl(EMotorStatusFlags value);
std::optional<EMotorStatusFlags> enumFromStringImpl(
    std::type_identity<EMotorStatusFlags>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EMotorStatusFlags>);

std::string_view enumToStringImpl(EArm value);
std::optional<EArm> enumFromStringImpl(std::type_identity<EArm>,
                                       std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EArm>);

std::string_view enumToStringImpl(EToolChangerStatusFlags value);
std::optional<EToolChangerStatusFlags> enumFromStringImpl(
    std::type_identity<EToolChangerStatusFlags>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EToolChangerStatusFlags>);

std::string_view enumToStringImpl(EToolChangerAction value);
std::optional<EToolChangerAction> enumFromStringImpl(
    std::type_identity<EToolChangerAction>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EToolChangerAction>);

std::string_view enumToStringImpl(ERobotComponent value);
std::optional<ERobotComponent> enumFromStringImpl(
    std::type_identity<ERobotComponent>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<ERobotComponent>);

std::string_view enumToStringImpl(EAxisState value);
std::optional<EAxisState> enumFromStringImpl(std::type_identity<EAxisState>,
                                             std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EAxisState>);

std::string_view enumToStringImpl(EInputSignal value);
std::optional<EInputSignal> enumFromStringImpl(std::type_identity<EInputSignal>,
                                               std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EInputSignal>);

std::string_view enumToStringImpl(EOutputSignal value);
std::optional<EOutputSignal> enumFromStringImpl(
    std::type_identity<EOutputSignal>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<EOutputSignal>);

#if __has_include(<libserial/SerialPortConstants.h>)
std::string_view enumToStringImpl(LibSerial::BaudRate value);
std::optional<LibSerial::BaudRate> enumFromStringImpl(
    std::type_identity<LibSerial::BaudRate>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<LibSerial::BaudRate>);

std::string_view enumToStringImpl(LibSerial::CharacterSize value);
std::optional<LibSerial::CharacterSize> enumFromStringImpl(
    std::type_identity<LibSerial::CharacterSize>, std::string_view text);
std::string_view knownEnumNamesImpl(
    std::type_identity<LibSerial::CharacterSize>);

std::string_view enumToStringImpl(LibSerial::FlowControl value);
std::optional<LibSerial::FlowControl> enumFromStringImpl(
    std::type_identity<LibSerial::FlowControl>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<LibSerial::FlowControl>);

std::string_view enumToStringImpl(LibSerial::Parity value);
std::optional<LibSerial::Parity> enumFromStringImpl(
    std::type_identity<LibSerial::Parity>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<LibSerial::Parity>);

std::string_view enumToStringImpl(LibSerial::StopBits value);
std::optional<LibSerial::StopBits> enumFromStringImpl(
    std::type_identity<LibSerial::StopBits>, std::string_view text);
std::string_view knownEnumNamesImpl(std::type_identity<LibSerial::StopBits>);
#endif
}  // namespace utl::detail

namespace utl {
template <typename TEnum>
std::string_view enumToStringView(const TEnum value) {
  static_assert(std::is_enum_v<TEnum>);
  return detail::enumToStringImpl(value);
}

template <typename TEnum>
std::string enumToString(const TEnum value) {
  return std::string(enumToStringView(value));
}

template <typename TEnum>
std::optional<TEnum> tryEnumFromString(const std::string_view text) {
  static_assert(std::is_enum_v<TEnum>);
  return detail::enumFromStringImpl(std::type_identity<TEnum>{}, text);
}

template <typename TEnum>
TEnum stringToEnum(const std::string_view text) {
  static_assert(std::is_enum_v<TEnum>);
  const auto parsed = tryEnumFromString<TEnum>(text);
  if (!parsed.has_value()) {
    throw std::runtime_error("Invalid enum value: " + std::string(text));
  }
  return *parsed;
}

template <typename TEnum>
std::string_view knownEnumNames() {
  static_assert(std::is_enum_v<TEnum>);
  return detail::knownEnumNamesImpl(std::type_identity<TEnum>{});
}
}  // namespace utl
