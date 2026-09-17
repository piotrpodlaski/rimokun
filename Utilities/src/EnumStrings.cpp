#include "EnumStrings.hpp"

#include <array>

namespace {
template <typename TEnum>
struct EnumEntry {
  TEnum value;
  std::string_view name;
};

template <typename TEnum, std::size_t N>
std::string_view enumToString(
    const std::array<EnumEntry<TEnum>, N>& entries, const TEnum value) {
  for (const auto& entry : entries) {
    if (entry.value == value) {
      return entry.name;
    }
  }
  return "Unknown";
}

template <typename TEnum, std::size_t N>
std::optional<TEnum> enumFromString(
    const std::array<EnumEntry<TEnum>, N>& entries, const std::string_view text) {
  for (const auto& entry : entries) {
    if (entry.name == text) {
      return entry.value;
    }
  }
  return std::nullopt;
}

constexpr std::array kMotorEntries{
    EnumEntry<utl::EMotor>{utl::EMotor::XLeft, "XLeft"},
    EnumEntry<utl::EMotor>{utl::EMotor::XRight, "XRight"},
    EnumEntry<utl::EMotor>{utl::EMotor::YLeft, "YLeft"},
    EnumEntry<utl::EMotor>{utl::EMotor::YRight, "YRight"},
    EnumEntry<utl::EMotor>{utl::EMotor::ZLeft, "ZLeft"},
    EnumEntry<utl::EMotor>{utl::EMotor::ZRight, "ZRight"},
};

constexpr std::array kLedStateEntries{
    EnumEntry<utl::ELEDState>{utl::ELEDState::On, "On"},
    EnumEntry<utl::ELEDState>{utl::ELEDState::Off, "Off"},
    EnumEntry<utl::ELEDState>{utl::ELEDState::Warning, "Warning"},
    EnumEntry<utl::ELEDState>{utl::ELEDState::Error, "Error"},
    EnumEntry<utl::ELEDState>{utl::ELEDState::ErrorBlinking, "ErrorBlinking"},
};

constexpr std::array kMotorStatusFlagEntries{
    EnumEntry<utl::EMotorStatusFlags>{utl::EMotorStatusFlags::BrakeApplied,
                                      "BrakeApplied"},
    EnumEntry<utl::EMotorStatusFlags>{utl::EMotorStatusFlags::Enabled,
                                      "Enabled"},
    EnumEntry<utl::EMotorStatusFlags>{utl::EMotorStatusFlags::Warning,
                                      "Warning"},
    EnumEntry<utl::EMotorStatusFlags>{utl::EMotorStatusFlags::Alarm, "Alarm"},
    EnumEntry<utl::EMotorStatusFlags>{utl::EMotorStatusFlags::Homing, "Homing"},
    EnumEntry<utl::EMotorStatusFlags>{utl::EMotorStatusFlags::Free, "Free"},
};

constexpr std::array kArmEntries{
    EnumEntry<utl::EArm>{utl::EArm::Left, "Left"},
    EnumEntry<utl::EArm>{utl::EArm::Right, "Right"},
    EnumEntry<utl::EArm>{utl::EArm::Gantry, "Gantry"},
};

constexpr std::array kToolChangerStatusFlagEntries{
    EnumEntry<utl::EToolChangerStatusFlags>{utl::EToolChangerStatusFlags::ProxSen,
                                            "ProxSen"},
    EnumEntry<utl::EToolChangerStatusFlags>{utl::EToolChangerStatusFlags::OpenSen,
                                            "OpenSen"},
    EnumEntry<utl::EToolChangerStatusFlags>{
        utl::EToolChangerStatusFlags::ClosedSen, "ClosedSen"},
    EnumEntry<utl::EToolChangerStatusFlags>{
        utl::EToolChangerStatusFlags::OpenValve, "OpenValve"},
    EnumEntry<utl::EToolChangerStatusFlags>{
        utl::EToolChangerStatusFlags::ClosedValve, "ClosedValve"},
};

constexpr std::array kToolChangerActionEntries{
    EnumEntry<utl::EToolChangerAction>{utl::EToolChangerAction::Open, "Open"},
    EnumEntry<utl::EToolChangerAction>{utl::EToolChangerAction::Close, "Close"},
};

constexpr std::array kRobotComponentEntries{
    EnumEntry<utl::ERobotComponent>{utl::ERobotComponent::Contec, "Contec"},
    EnumEntry<utl::ERobotComponent>{utl::ERobotComponent::MotorControl,
                                    "MotorControl"},
    EnumEntry<utl::ERobotComponent>{utl::ERobotComponent::ControlPanel,
                                    "ControlPanel"},
};

constexpr std::array kAxisStateEntries{
    EnumEntry<utl::EAxisState>{utl::EAxisState::Locked, "Locked"},
    EnumEntry<utl::EAxisState>{utl::EAxisState::Slow, "Slow"},
    EnumEntry<utl::EAxisState>{utl::EAxisState::Fast, "Fast"},
};

constexpr std::array kHomingAxisEntries{
    EnumEntry<utl::EHomingAxis>{utl::EHomingAxis::XLeft, "XLeft"},
    EnumEntry<utl::EHomingAxis>{utl::EHomingAxis::YLeft, "YLeft"},
    EnumEntry<utl::EHomingAxis>{utl::EHomingAxis::XRight, "XRight"},
    EnumEntry<utl::EHomingAxis>{utl::EHomingAxis::YRight, "YRight"},
    EnumEntry<utl::EHomingAxis>{utl::EHomingAxis::Z, "Z"},
};

constexpr std::array kHomingStateEntries{
    EnumEntry<utl::EHomingState>{utl::EHomingState::Idle, "Idle"},
    EnumEntry<utl::EHomingState>{utl::EHomingState::Pending, "Pending"},
    EnumEntry<utl::EHomingState>{utl::EHomingState::Running, "Running"},
    EnumEntry<utl::EHomingState>{utl::EHomingState::Succeeded, "Succeeded"},
    EnumEntry<utl::EHomingState>{utl::EHomingState::Failed, "Failed"},
    EnumEntry<utl::EHomingState>{utl::EHomingState::TimedOut, "TimedOut"},
    EnumEntry<utl::EHomingState>{utl::EHomingState::Cancelled, "Cancelled"},
};

constexpr std::array kInputSignalEntries{
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::motorPower, "motorPower"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::safetyON, "safetyON"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::robotAUX1, "robotAUX1"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::robotAUX2, "robotAUX2"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::localAUX1, "localAUX1"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::localAUX2, "localAUX2"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::localAUX3, "localAUX3"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::localAUX4, "localAUX4"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::localAUX5, "localAUX5"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::tclProx, "tclProx"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::tclOpen, "tclOpen"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::tclClose, "tclClose"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::tcrProx, "tcrProx"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::tcrOpen, "tcrOpen"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::tcrClose, "tcrClose"},
    EnumEntry<utl::EInputSignal>{utl::EInputSignal::cpuOverride, "cpuOverride"},
};

constexpr std::array kOutputSignalEntries{
    EnumEntry<utl::EOutputSignal>{utl::EOutputSignal::toolChangerLeft,
                                  "toolChangerLeft"},
    EnumEntry<utl::EOutputSignal>{utl::EOutputSignal::toolChangerRight,
                                  "toolChangerRight"},
    EnumEntry<utl::EOutputSignal>{utl::EOutputSignal::heartBeat, "heartBeat"},
    EnumEntry<utl::EOutputSignal>{utl::EOutputSignal::safetyReset,
                                  "safetyReset"},
    EnumEntry<utl::EOutputSignal>{utl::EOutputSignal::light1, "light1"},
    EnumEntry<utl::EOutputSignal>{utl::EOutputSignal::light2, "light2"},
    EnumEntry<utl::EOutputSignal>{utl::EOutputSignal::alarmIndicator, "alarmIndicator"},
};

#if __has_include(<libserial/SerialPortConstants.h>)
constexpr std::array kBaudRateEntries{
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_50, "BAUD_50"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_75, "BAUD_75"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_110, "BAUD_110"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_134, "BAUD_134"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_150, "BAUD_150"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_200, "BAUD_200"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_300, "BAUD_300"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_600, "BAUD_600"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_1200, "BAUD_1200"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_1800, "BAUD_1800"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_2400, "BAUD_2400"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_4800, "BAUD_4800"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_9600, "BAUD_9600"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_19200, "BAUD_19200"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_38400, "BAUD_38400"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_57600, "BAUD_57600"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_115200,
                                   "BAUD_115200"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_230400,
                                   "BAUD_230400"},
#ifdef __linux__
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_460800,
                                   "BAUD_460800"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_500000,
                                   "BAUD_500000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_576000,
                                   "BAUD_576000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_921600,
                                   "BAUD_921600"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_1000000,
                                   "BAUD_1000000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_1152000,
                                   "BAUD_1152000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_1500000,
                                   "BAUD_1500000"},
#if __MAX_BAUD > B2000000
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_2000000,
                                   "BAUD_2000000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_2500000,
                                   "BAUD_2500000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_3000000,
                                   "BAUD_3000000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_3500000,
                                   "BAUD_3500000"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_4000000,
                                   "BAUD_4000000"},
#endif
#endif
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_DEFAULT,
                                   "BAUD_DEFAULT"},
    EnumEntry<LibSerial::BaudRate>{LibSerial::BaudRate::BAUD_INVALID,
                                   "BAUD_INVALID"},
};

constexpr std::array kCharacterSizeEntries{
    EnumEntry<LibSerial::CharacterSize>{LibSerial::CharacterSize::CHAR_SIZE_5,
                                        "CHAR_SIZE_5"},
    EnumEntry<LibSerial::CharacterSize>{LibSerial::CharacterSize::CHAR_SIZE_6,
                                        "CHAR_SIZE_6"},
    EnumEntry<LibSerial::CharacterSize>{LibSerial::CharacterSize::CHAR_SIZE_7,
                                        "CHAR_SIZE_7"},
    EnumEntry<LibSerial::CharacterSize>{LibSerial::CharacterSize::CHAR_SIZE_8,
                                        "CHAR_SIZE_8"},
    EnumEntry<LibSerial::CharacterSize>{
        LibSerial::CharacterSize::CHAR_SIZE_DEFAULT, "CHAR_SIZE_DEFAULT"},
    EnumEntry<LibSerial::CharacterSize>{
        LibSerial::CharacterSize::CHAR_SIZE_INVALID, "CHAR_SIZE_INVALID"},
};

constexpr std::array kFlowControlEntries{
    EnumEntry<LibSerial::FlowControl>{
        LibSerial::FlowControl::FLOW_CONTROL_HARDWARE, "FLOW_CONTROL_HARDWARE"},
    EnumEntry<LibSerial::FlowControl>{
        LibSerial::FlowControl::FLOW_CONTROL_SOFTWARE, "FLOW_CONTROL_SOFTWARE"},
    EnumEntry<LibSerial::FlowControl>{LibSerial::FlowControl::FLOW_CONTROL_NONE,
                                      "FLOW_CONTROL_NONE"},
    EnumEntry<LibSerial::FlowControl>{
        LibSerial::FlowControl::FLOW_CONTROL_DEFAULT, "FLOW_CONTROL_DEFAULT"},
    EnumEntry<LibSerial::FlowControl>{
        LibSerial::FlowControl::FLOW_CONTROL_INVALID, "FLOW_CONTROL_INVALID"},
};

constexpr std::array kParityEntries{
    EnumEntry<LibSerial::Parity>{LibSerial::Parity::PARITY_EVEN, "PARITY_EVEN"},
    EnumEntry<LibSerial::Parity>{LibSerial::Parity::PARITY_ODD, "PARITY_ODD"},
    EnumEntry<LibSerial::Parity>{LibSerial::Parity::PARITY_NONE, "PARITY_NONE"},
    EnumEntry<LibSerial::Parity>{LibSerial::Parity::PARITY_DEFAULT,
                                 "PARITY_DEFAULT"},
    EnumEntry<LibSerial::Parity>{LibSerial::Parity::PARITY_INVALID,
                                 "PARITY_INVALID"},
};

constexpr std::array kStopBitsEntries{
    EnumEntry<LibSerial::StopBits>{LibSerial::StopBits::STOP_BITS_1,
                                   "STOP_BITS_1"},
    EnumEntry<LibSerial::StopBits>{LibSerial::StopBits::STOP_BITS_2,
                                   "STOP_BITS_2"},
    EnumEntry<LibSerial::StopBits>{LibSerial::StopBits::STOP_BITS_DEFAULT,
                                   "STOP_BITS_DEFAULT"},
    EnumEntry<LibSerial::StopBits>{LibSerial::StopBits::STOP_BITS_INVALID,
                                   "STOP_BITS_INVALID"},
};
#endif
}  // namespace

namespace utl::detail {
std::string_view enumToStringImpl(const EMotor value) {
  return enumToString(kMotorEntries, value);
}
std::optional<EMotor> enumFromStringImpl(const std::type_identity<EMotor>,
                                         const std::string_view text) {
  return enumFromString(kMotorEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EMotor>) {
  return "XLeft, XRight, YLeft, YRight, ZLeft, ZRight";
}

std::string_view enumToStringImpl(const ELEDState value) {
  return enumToString(kLedStateEntries, value);
}
std::optional<ELEDState> enumFromStringImpl(const std::type_identity<ELEDState>,
                                            const std::string_view text) {
  return enumFromString(kLedStateEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<ELEDState>) {
  return "On, Off, Warning, Error, ErrorBlinking";
}

std::string_view enumToStringImpl(const EMotorStatusFlags value) {
  return enumToString(kMotorStatusFlagEntries, value);
}
std::optional<EMotorStatusFlags> enumFromStringImpl(
    const std::type_identity<EMotorStatusFlags>, const std::string_view text) {
  return enumFromString(kMotorStatusFlagEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EMotorStatusFlags>) {
  return "BrakeApplied, Enabled, Warning, Alarm, Homing, Free";
}

std::string_view enumToStringImpl(const EArm value) {
  return enumToString(kArmEntries, value);
}
std::optional<EArm> enumFromStringImpl(const std::type_identity<EArm>,
                                       const std::string_view text) {
  return enumFromString(kArmEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EArm>) {
  return "Left, Right, Gantry";
}

std::string_view enumToStringImpl(const EToolChangerStatusFlags value) {
  return enumToString(kToolChangerStatusFlagEntries, value);
}
std::optional<EToolChangerStatusFlags> enumFromStringImpl(
    const std::type_identity<EToolChangerStatusFlags>,
    const std::string_view text) {
  return enumFromString(kToolChangerStatusFlagEntries, text);
}
std::string_view knownEnumNamesImpl(
    const std::type_identity<EToolChangerStatusFlags>) {
  return "ProxSen, OpenSen, ClosedSen, OpenValve, ClosedValve";
}

std::string_view enumToStringImpl(const EToolChangerAction value) {
  return enumToString(kToolChangerActionEntries, value);
}
std::optional<EToolChangerAction> enumFromStringImpl(
    const std::type_identity<EToolChangerAction>, const std::string_view text) {
  return enumFromString(kToolChangerActionEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EToolChangerAction>) {
  return "Open, Close";
}

std::string_view enumToStringImpl(const ERobotComponent value) {
  return enumToString(kRobotComponentEntries, value);
}
std::optional<ERobotComponent> enumFromStringImpl(
    const std::type_identity<ERobotComponent>, const std::string_view text) {
  return enumFromString(kRobotComponentEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<ERobotComponent>) {
  return "Contec, MotorControl, ControlPanel";
}

std::string_view enumToStringImpl(const EAxisState value) {
  return enumToString(kAxisStateEntries, value);
}
std::optional<EAxisState> enumFromStringImpl(const std::type_identity<EAxisState>,
                                             const std::string_view text) {
  return enumFromString(kAxisStateEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EAxisState>) {
  return "Locked, Slow, Fast";
}

std::string_view enumToStringImpl(const EHomingAxis value) {
  return enumToString(kHomingAxisEntries, value);
}
std::optional<EHomingAxis> enumFromStringImpl(
    const std::type_identity<EHomingAxis>, const std::string_view text) {
  return enumFromString(kHomingAxisEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EHomingAxis>) {
  return "XLeft, YLeft, XRight, YRight, Z";
}

std::string_view enumToStringImpl(const EHomingState value) {
  return enumToString(kHomingStateEntries, value);
}
std::optional<EHomingState> enumFromStringImpl(
    const std::type_identity<EHomingState>, const std::string_view text) {
  return enumFromString(kHomingStateEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EHomingState>) {
  return "Idle, Pending, Running, Succeeded, Failed, TimedOut, Cancelled";
}

std::string_view enumToStringImpl(const EInputSignal value) {
  return enumToString(kInputSignalEntries, value);
}
std::optional<EInputSignal> enumFromStringImpl(
    const std::type_identity<EInputSignal>, const std::string_view text) {
  return enumFromString(kInputSignalEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EInputSignal>) {
  return "motorPower, safetyON, robotAUX1, robotAUX2, localAUX1, localAUX2, "
         "localAUX3, localAUX4, localAUX5, tclProx, tclOpen, tclClose, "
         "tcrProx, tcrOpen, tcrClose, cpuOverride";
}

std::string_view enumToStringImpl(const EOutputSignal value) {
  return enumToString(kOutputSignalEntries, value);
}
std::optional<EOutputSignal> enumFromStringImpl(
    const std::type_identity<EOutputSignal>, const std::string_view text) {
  return enumFromString(kOutputSignalEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<EOutputSignal>) {
  return "toolChangerLeft, toolChangerRight, heartBeat, safetyReset, light1, "
         "light2, alarmIndicator";
}

#if __has_include(<libserial/SerialPortConstants.h>)
std::string_view enumToStringImpl(const LibSerial::BaudRate value) {
  return enumToString(kBaudRateEntries, value);
}
std::optional<LibSerial::BaudRate> enumFromStringImpl(
    const std::type_identity<LibSerial::BaudRate>, const std::string_view text) {
  return enumFromString(kBaudRateEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<LibSerial::BaudRate>) {
  return "LibSerial::BaudRate";
}

std::string_view enumToStringImpl(const LibSerial::CharacterSize value) {
  return enumToString(kCharacterSizeEntries, value);
}
std::optional<LibSerial::CharacterSize> enumFromStringImpl(
    const std::type_identity<LibSerial::CharacterSize>,
    const std::string_view text) {
  return enumFromString(kCharacterSizeEntries, text);
}
std::string_view knownEnumNamesImpl(
    const std::type_identity<LibSerial::CharacterSize>) {
  return "CHAR_SIZE_5, CHAR_SIZE_6, CHAR_SIZE_7, CHAR_SIZE_8, "
         "CHAR_SIZE_DEFAULT, CHAR_SIZE_INVALID";
}

std::string_view enumToStringImpl(const LibSerial::FlowControl value) {
  return enumToString(kFlowControlEntries, value);
}
std::optional<LibSerial::FlowControl> enumFromStringImpl(
    const std::type_identity<LibSerial::FlowControl>,
    const std::string_view text) {
  return enumFromString(kFlowControlEntries, text);
}
std::string_view knownEnumNamesImpl(
    const std::type_identity<LibSerial::FlowControl>) {
  return "FLOW_CONTROL_HARDWARE, FLOW_CONTROL_SOFTWARE, FLOW_CONTROL_NONE, "
         "FLOW_CONTROL_DEFAULT, FLOW_CONTROL_INVALID";
}

std::string_view enumToStringImpl(const LibSerial::Parity value) {
  return enumToString(kParityEntries, value);
}
std::optional<LibSerial::Parity> enumFromStringImpl(
    const std::type_identity<LibSerial::Parity>, const std::string_view text) {
  return enumFromString(kParityEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<LibSerial::Parity>) {
  return "PARITY_EVEN, PARITY_ODD, PARITY_NONE, PARITY_DEFAULT, PARITY_INVALID";
}

std::string_view enumToStringImpl(const LibSerial::StopBits value) {
  return enumToString(kStopBitsEntries, value);
}
std::optional<LibSerial::StopBits> enumFromStringImpl(
    const std::type_identity<LibSerial::StopBits>, const std::string_view text) {
  return enumFromString(kStopBitsEntries, text);
}
std::string_view knownEnumNamesImpl(const std::type_identity<LibSerial::StopBits>) {
  return "STOP_BITS_1, STOP_BITS_2, STOP_BITS_DEFAULT, STOP_BITS_INVALID";
}
#endif
}  // namespace utl::detail
