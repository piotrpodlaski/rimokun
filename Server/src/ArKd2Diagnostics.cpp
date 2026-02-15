#include <ArKd2Diagnostics.hpp>

#include <array>

namespace {
using Entry = ArKd2CodeInfo;

constexpr std::array<Entry, 35> kAlarmCodes{{
    {0x10, "Excessive position deviation",
     "Deviation exceeded the configured alarm-at-current-ON threshold.",
     "Reduce load, relax accel/decel, verify motor cable."},
    {0x12, "Excessive position deviation during current OFF",
     "C-ON was turned on while excessive-position warning at current OFF was present.",
     "Avoid enabling C-ON in this state; review auto return setting."},
    {0x20, "Overcurrent", "Motor/cable/driver output short circuit detected.",
     "Power cycle and verify wiring/shorts before re-enabling."},
    {0x21, "Main circuit overheat", "Driver internal temperature exceeded limit.",
     "Improve cabinet cooling and ambient conditions."},
    {0x22, "Overvoltage", "Power supply voltage exceeded permitted range.",
     "Check supply voltage and deceleration profile/load."},
    {0x23, "Main power off", "Motor start attempted while main power was absent.",
     "Verify main power input and power sequencing."},
    {0x25, "Undervoltage", "Main power dropped or fell below permitted range.",
     "Check power supply voltage and source stability."},
    {0x28, "Sensor error", "Sensor error occurred during operation.",
     "Power cycle and verify motor/driver cable connection."},
    {0x29, "CPU peripheral circuit error", "Internal CPU-side error occurred.",
     "Power cycle and recheck. If persistent, service driver."},
    {0x30, "Overload", "Load/torque demand exceeded configured overload window.",
     "Reduce load, increase accel/decel time, review run current."},
    {0x31, "Overspeed", "Motor output speed exceeded limit.",
     "Adjust electronic gear/profile so speed stays in range."},
    {0x34, "Command pulse error", "Command pulse frequency exceeded specification.",
     "Review electronic gear and commanded speed profile."},
    {0x41, "EEPROM error", "Stored parameter data is corrupted.",
     "Initialize parameters and power cycle."},
    {0x42, "Initial sensor error", "Sensor error detected at power-on.",
     "Power cycle and verify motor cable/feedback wiring."},
    {0x43, "Initial rotor rotation error",
     "Output shaft was not still at power-on.",
     "Remove external force/load during power-up."},
    {0x45, "Motor combination error", "Unsupported motor/driver combination detected.",
     "Check model matching and wiring."},
    {0x4A, "Return-to-home incomplete", "Positioning started before origin was set.",
     "Perform preset or return-to-home before positioning."},
    {0x51, "Regeneration resistor overheat",
     "Regeneration resistor connection/thermal issue detected.",
     "Check TH1/TH2 and resistor sizing/wiring."},
    {0x60, "±LS both sides active", "+LS and -LS detected simultaneously.",
     "Verify sensor logic and LS contact configuration."},
    {0x61, "Reverse limit sensor connection",
     "Opposite-direction limit sensor detected during homing.",
     "Check limit sensor wiring and installation."},
    {0x62, "Home seeking error", "Return-to-home did not complete normally.",
     "Check load and home/limit sensor geometry and logic."},
    {0x63, "No HOMES",
     "HOMES not detected between positive/negative limit sensors.",
     "Place/adjust mechanical home sensor in expected span."},
    {0x64, "TIM/Z/SLIT signal error",
     "Required TIM/SLIT signals were not detected during homing.",
     "Adjust mechanics/sensors or disable unused detections."},
    {0x66, "Hardware overtravel",
     "+LS/-LS detected while hardware overtravel protection enabled.",
     "Exit limit region and verify LS setup."},
    {0x67, "Software overtravel",
     "Soft limit reached while software overtravel enabled.",
     "Adjust operation data and soft limits."},
    {0x6A, "Home seeking offset error",
     "Limit sensor detected during home offset movement.",
     "Review offset value and sensor placement."},
    {0x70, "Abnormal operation data",
     "Operation data combination is invalid for requested motion.",
     "Validate linked data and speed settings."},
    {0x71, "Electronic gear setting error",
     "Electronic gear resolution is out of specification.",
     "Set resolution to valid range and power cycle."},
    {0x72, "Wrap setting error",
     "Wrap setting range is inconsistent with configured resolution.",
     "Correct wrap parameter values and power cycle."},
    {0x81, "Network bus error",
     "Network converter host link disconnected during operation.",
     "Check network converter cable/connector/controller."},
    {0x83, "Communication switch setting error",
     "Transmission-rate switch setting is out of specification.",
     "Set valid switch value and power cycle."},
    {0x84, "RS-485 communication error",
     "Consecutive RS-485 errors exceeded configured threshold.",
     "Check bus wiring, noise, and communication settings."},
    {0x85, "RS-485 communication timeout",
     "No communication within configured timeout window.",
     "Verify host communication cadence and cable/port."},
    {0x8E, "Network converter error",
     "Network converter reported an alarm.",
     "Check network converter alarm details."},
    {0xF0, "CPU error", "CPU malfunction detected.",
     "Power cycle. If persistent, service driver."},
}};

constexpr std::array<Entry, 10> kWarningCodes{{
    {0x10, "Excessive position deviation",
     "Deviation exceeded configured warning threshold.",
     "Reduce load and adjust accel/decel/current settings."},
    {0x12, "Excessive position deviation during current OFF",
     "Deviation exceeded configured current-OFF threshold.",
     "Reduce drift at current OFF or adjust threshold."},
    {0x21, "Main circuit overheat", "Driver temperature exceeded warning setting.",
     "Improve ventilation/thermal conditions."},
    {0x22, "Overvoltage", "Power supply exceeded overvoltage warning level.",
     "Check supply and reduce regen stress."},
    {0x25, "Undervoltage", "Power supply dropped below warning level.",
     "Check supply sizing/stability."},
    {0x30, "Overload", "Load exceeded overload warning conditions.",
     "Reduce load and adjust accel/decel/current."},
    {0x31, "Overspeed", "Speed exceeded overspeed warning setting.",
     "Reduce commanded speed and tune profile."},
    {0x71, "Electronic gear setting error",
     "Electronic gear resolution is out of range.",
     "Correct electronic gear settings."},
    {0x72, "Wrap setting error",
     "Wrap range and resolution settings are inconsistent.",
     "Correct wrap/resolution settings."},
    {0x84, "RS-485 communication error", "RS-485 communication error detected.",
     "Check bus wiring and communication parameters."},
}};

constexpr std::array<Entry, 6> kCommunicationErrorCodes{{
    {0x84, "RS-485 communication error",
     "Transmission-level communication error (framing/BCC).",
     "Check cable/wiring/termination and serial settings."},
    {0x88, "Command not yet defined",
     "Requested command is not supported/defined.",
     "Validate function/register map and frame payload."},
    {0x89, "Execution disabled (user I/F communication in progress)",
     "Driver is busy with MEXE02/OPX-2A communication.",
     "Retry after external tool communication completes."},
    {0x8A, "Execution disabled (non-volatile memory processing)",
     "Driver is processing non-volatile memory or EEPROM alarm is active.",
     "Wait for S-BSY clear; resolve EEPROM alarm if present."},
    {0x8C, "Outside setting range",
     "Requested setting value is outside allowed range.",
     "Validate and clamp setting data before write."},
    {0x8D, "Command execute disable",
     "Command cannot be executed in current driver state.",
     "Check driver state/interlocks and retry when valid."},
}};

template <std::size_t N>
std::optional<ArKd2CodeInfo> findIn(const std::array<Entry, N>& entries,
                                    std::uint8_t code) {
  for (const auto& entry : entries) {
    if (entry.code == code) {
      return entry;
    }
  }
  return std::nullopt;
}
}  // namespace

std::optional<ArKd2CodeInfo> arKd2FindAlarm(const std::uint8_t code) {
  return findIn(kAlarmCodes, code);
}

std::optional<ArKd2CodeInfo> arKd2FindWarning(const std::uint8_t code) {
  return findIn(kWarningCodes, code);
}

std::optional<ArKd2CodeInfo> arKd2FindCommunicationError(
    const std::uint8_t code) {
  return findIn(kCommunicationErrorCodes, code);
}

std::optional<ArKd2CodeInfo> arKd2FindCode(const ArKd2CodeDomain domain,
                                           const std::uint8_t code) {
  switch (domain) {
    case ArKd2CodeDomain::Alarm:
      return arKd2FindAlarm(code);
    case ArKd2CodeDomain::Warning:
      return arKd2FindWarning(code);
    case ArKd2CodeDomain::CommunicationError:
      return arKd2FindCommunicationError(code);
  }
  return std::nullopt;
}

std::string_view arKd2DomainName(const ArKd2CodeDomain domain) {
  switch (domain) {
    case ArKd2CodeDomain::Alarm:
      return "alarm";
    case ArKd2CodeDomain::Warning:
      return "warning";
    case ArKd2CodeDomain::CommunicationError:
      return "communication error";
  }
  return "unknown";
}

