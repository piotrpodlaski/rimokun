#include "AxisHomingManager.hpp"

#include <Config.hpp>
#include <EnumStrings.hpp>
#include <ExceptionUtils.hpp>

#include <array>
#include <cmath>
#include <format>

using namespace std::chrono_literals;

namespace {
std::string_view homingTargetName(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::All: return "All";
    case cmd::HomingTarget::XLeft: return "XLeft";
    case cmd::HomingTarget::YLeft: return "YLeft";
    case cmd::HomingTarget::XRight: return "XRight";
    case cmd::HomingTarget::YRight: return "YRight";
    case cmd::HomingTarget::Z: return "Z";
  }
  return "Unknown";
}

std::optional<cmd::HomingTarget> parseHomingTarget(const std::string_view value) {
  if (value == "All") return cmd::HomingTarget::All;
  if (value == "XLeft") return cmd::HomingTarget::XLeft;
  if (value == "YLeft") return cmd::HomingTarget::YLeft;
  if (value == "XRight") return cmd::HomingTarget::XRight;
  if (value == "YRight") return cmd::HomingTarget::YRight;
  if (value == "Z") return cmd::HomingTarget::Z;
  return std::nullopt;
}

std::string_view axisConfigKey(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::XLeft: return "leftArmX";
    case cmd::HomingTarget::YLeft: return "leftArmY";
    case cmd::HomingTarget::XRight: return "rightArmX";
    case cmd::HomingTarget::YRight: return "rightArmY";
    case cmd::HomingTarget::Z: return "gantryZ";
    case cmd::HomingTarget::All: break;
  }
  utl::throwRuntimeError("Homing target 'All' does not map to a single axis.");
}

utl::EMotor motorForHomingTarget(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::XLeft: return utl::EMotor::XLeft;
    case cmd::HomingTarget::YLeft: return utl::EMotor::YLeft;
    case cmd::HomingTarget::XRight: return utl::EMotor::XRight;
    case cmd::HomingTarget::YRight: return utl::EMotor::YRight;
    case cmd::HomingTarget::Z: return utl::EMotor::ZLeft;
    case cmd::HomingTarget::All: break;
  }
  utl::throwRuntimeError("Homing target 'All' does not map to a single motor.");
}

std::string_view homingAxisName(const utl::EHomingAxis axis) {
  return utl::enumToStringView(axis);
}
}  // namespace

std::vector<cmd::HomingTarget> AxisHomingManager::loadOrder() {
  constexpr std::array defaultOrder{
      cmd::HomingTarget::Z,
      cmd::HomingTarget::XLeft,
      cmd::HomingTarget::XRight,
      cmd::HomingTarget::YLeft,
      cmd::HomingTarget::YRight,
  };

  std::vector<cmd::HomingTarget> order(defaultOrder.begin(), defaultOrder.end());
  try {
    const auto machineCfg = utl::Config::instance().getClassConfig("Machine");
    const auto homingCfg = machineCfg["homing"];
    if (!homingCfg || !homingCfg.IsMap()) {
      return order;
    }
    const auto orderCfg = homingCfg["order"];
    if (!orderCfg || !orderCfg.IsSequence()) {
      return order;
    }
    order.clear();
    for (const auto& node : orderCfg) {
      const auto value = node.as<std::string>();
      const auto parsed = parseHomingTarget(value);
      if (!parsed.has_value()) {
        utl::throwRuntimeError(
            std::format("Unknown Machine.homing.order target '{}'", value));
      }
      if (*parsed == cmd::HomingTarget::All) {
        utl::throwRuntimeError(
            "Machine.homing.order must list single axes, not 'All'.");
      }
      order.push_back(*parsed);
    }
  } catch (const std::exception& e) {
    SPDLOG_WARN("Using default homing order: {}", e.what());
    order.assign(defaultOrder.begin(), defaultOrder.end());
  }
  return order;
}

std::map<cmd::HomingTarget, AxisHomingManager::AxisConfig>
AxisHomingManager::loadAxisConfigs() {
  std::map<cmd::HomingTarget, AxisConfig> configs;
  try {
    const auto machineCfg = utl::Config::instance().getClassConfig("Machine");
    const auto axesCfg = machineCfg["motion"]["axes"];
    const auto directionCfg = machineCfg["homing"]["startingDirection"];
    if (!axesCfg || !axesCfg.IsMap()) {
      return configs;
    }
    if (!directionCfg || !directionCfg.IsMap()) {
      return configs;
    }

    for (const auto axis : {cmd::HomingTarget::XLeft, cmd::HomingTarget::YLeft,
                            cmd::HomingTarget::XRight, cmd::HomingTarget::YRight,
                            cmd::HomingTarget::Z}) {
      const auto axisKey = axisConfigKey(axis);
      const auto axisCfg = axesCfg[std::string(axisKey)];
      if (!axisCfg || !axisCfg.IsMap()) {
        utl::throwRuntimeError(
            std::format("Missing Machine.motion.axes.{} config for homing.", axisKey));
      }
      const auto directionNode = directionCfg[std::string(homingTargetName(axis))];
      if (!directionNode) {
        utl::throwRuntimeError(std::format(
            "Missing Machine.homing.startingDirection.{} config.",
            homingTargetName(axis)));
      }
      const auto startingDirection = directionNode.as<std::int32_t>();
      if (startingDirection != 0 && startingDirection != 1) {
        utl::throwRuntimeError(std::format(
            "Machine.homing.startingDirection.{} must be 0 or 1.",
            homingTargetName(axis)));
      }
      const auto slowSpeedMmPerSec = axisCfg["slowMaxLinearSpeedMmPerSec"].as<double>();
      const auto stepsPerMm = axisCfg["stepsPerMm"].as<double>();
      configs.emplace(axis, AxisConfig{
                                .motorId = motorForHomingTarget(axis),
                                .speedStepsPerSec = static_cast<std::int32_t>(
                                    std::llround(slowSpeedMmPerSec * stepsPerMm)),
                                .acceleration =
                                    axisCfg["slowAcceleration001MsPerKHz"].as<std::int32_t>(),
                                .startingDirection = startingDirection,
                            });
    }
  } catch (const std::exception& e) {
    SPDLOG_WARN("Homing axis config cache is incomplete: {}", e.what());
    configs.clear();
  }
  return configs;
}

std::chrono::milliseconds AxisHomingManager::loadTimeout() {
  try {
    const auto machineCfg = utl::Config::instance().getClassConfig("Machine");
    const auto homingCfg = machineCfg["homing"];
    if (!homingCfg || !homingCfg.IsMap()) {
      return 60s;
    }
    const auto timeoutMs = homingCfg["timeoutMS"];
    if (!timeoutMs) {
      return 60s;
    }
    return std::chrono::milliseconds{std::max(1, timeoutMs.as<int>())};
  } catch (const std::exception& e) {
    SPDLOG_WARN("Using default homing timeout: {}", e.what());
    return 60s;
  }
}

utl::EHomingAxis AxisHomingManager::axisFromTarget(const cmd::HomingTarget target) {
  switch (target) {
    case cmd::HomingTarget::XLeft: return utl::EHomingAxis::XLeft;
    case cmd::HomingTarget::YLeft: return utl::EHomingAxis::YLeft;
    case cmd::HomingTarget::XRight: return utl::EHomingAxis::XRight;
    case cmd::HomingTarget::YRight: return utl::EHomingAxis::YRight;
    case cmd::HomingTarget::Z: return utl::EHomingAxis::Z;
    case cmd::HomingTarget::All: break;
  }
  utl::throwRuntimeError("Homing target 'All' does not map to a single axis.");
}

std::optional<utl::EMotor> AxisHomingManager::motorFromAxis(const utl::EHomingAxis axis) {
  switch (axis) {
    case utl::EHomingAxis::XLeft: return utl::EMotor::XLeft;
    case utl::EHomingAxis::YLeft: return utl::EMotor::YLeft;
    case utl::EHomingAxis::XRight: return utl::EMotor::XRight;
    case utl::EHomingAxis::YRight: return utl::EMotor::YRight;
    case utl::EHomingAxis::Z: return utl::EMotor::ZLeft;
  }
  return std::nullopt;
}

AxisHomingManager::AxisHomingManager(std::shared_ptr<IClock> clock,
                                     const std::chrono::milliseconds timeout,
                                     std::vector<cmd::HomingTarget> order,
                                     std::map<cmd::HomingTarget, AxisConfig> axisConfigs,
                                     StartAxisFn startAxisFn)
    : _clock(std::move(clock)),
      _timeout(timeout),
      _order(std::move(order)),
      _axisConfigs(std::move(axisConfigs)),
      _startAxisFn(std::move(startAxisFn)) {
  if (!_clock) {
    utl::throwRuntimeError("AxisHomingManager requires a non-null clock.");
  }
  if (!_startAxisFn) {
    utl::throwRuntimeError("AxisHomingManager requires a start-axis callback.");
  }
}

void AxisHomingManager::start(const cmd::HomingTarget target, utl::RobotStatus& status,
                              std::map<utl::EMotor, bool>& homingRequested) {
  if (target == cmd::HomingTarget::All) {
    beginSession(_order, status, homingRequested);
    return;
  }
  beginSession({target}, status, homingRequested);
}

void AxisHomingManager::cancel(
    utl::RobotStatus& status,
    std::map<utl::EMotor, bool>& homingRequested) {
  if (!_runtime.active) {
    status.homing.state = utl::EHomingState::Cancelled;
    status.homing.message = "No active homing session to cancel.";
    return;
  }

  if (_runtime.currentAxisIndex < status.homing.axes.size()) {
    auto& axisStatus = status.homing.axes[_runtime.currentAxisIndex];
    axisStatus.state = utl::EHomingState::Cancelled;
    axisStatus.message = "Homing cancelled by operator.";
  }
  for (std::size_t i = _runtime.currentAxisIndex + 1;
       i < status.homing.axes.size(); ++i) {
    if (status.homing.axes[i].state == utl::EHomingState::Pending) {
      status.homing.axes[i].state = utl::EHomingState::Cancelled;
      status.homing.axes[i].message = "Not started.";
    }
  }
  for (auto& [_, requested] : homingRequested) {
    requested = false;
  }
  status.homing.state = utl::EHomingState::Cancelled;
  status.homing.message = "Homing cancelled by operator.";
  _runtime.active = false;
}

void AxisHomingManager::advanceFromStatus(utl::RobotStatus& status,
                                          std::map<utl::EMotor, bool>& homingRequested) {
  if (!_runtime.active || _runtime.currentAxisIndex >= status.homing.axes.size()) {
    return;
  }

  auto& axisStatus = status.homing.axes[_runtime.currentAxisIndex];
  const auto motor = motorFromAxis(axisStatus.axis);
  if (!motor.has_value()) {
    finishCurrentAxis(status, homingRequested, utl::EHomingState::Failed,
                      "Axis is not mapped to a motor.");
    return;
  }

  const auto motorIt = status.motors.find(*motor);
  if (motorIt == status.motors.end()) {
    return;
  }
  const auto& motorStatus = motorIt->second;

  const auto alarmIt = motorStatus.flags.find(utl::EMotorStatusFlags::Alarm);
  if (alarmIt != motorStatus.flags.end() &&
      alarmIt->second == utl::ELEDState::Error) {
    finishCurrentAxis(status, homingRequested, utl::EHomingState::Failed,
                      motorStatus.alarmDescription.empty() ? "Motor alarm during homing."
                                                           : motorStatus.alarmDescription);
    return;
  }

  const auto homingIt = motorStatus.flags.find(utl::EMotorStatusFlags::Homing);
  if (homingIt != motorStatus.flags.end() &&
      homingIt->second == utl::ELEDState::On) {
    finishCurrentAxis(status, homingRequested, utl::EHomingState::Succeeded,
                      "Homing complete.");
    return;
  }

  if ((_clock->now() - _runtime.currentAxisStartedAt) > _timeout) {
    finishCurrentAxis(status, homingRequested, utl::EHomingState::TimedOut,
                      std::format("Timed out after {} ms.", _timeout.count()));
  }
}

void AxisHomingManager::beginSession(const std::vector<cmd::HomingTarget>& axes,
                                     utl::RobotStatus& status,
                                     std::map<utl::EMotor, bool>& homingRequested) {
  if (axes.empty()) {
    utl::throwRuntimeError("Cannot start homing session with no axes.");
  }
  if (_runtime.active) {
    utl::throwRuntimeError("A homing session is already in progress.");
  }

  status.homing = {};
  status.homing.state = utl::EHomingState::Pending;
  _runtime = {};

  for (const auto axis : axes) {
    if (axis == cmd::HomingTarget::All) {
      utl::throwRuntimeError("Homing session axis list must not contain 'All'.");
    }
    const auto homingAxis = axisFromTarget(axis);
    status.homing.axes.push_back(
        {.axis = homingAxis, .state = utl::EHomingState::Pending, .message = {}});
    if (const auto motor = motorFromAxis(homingAxis); motor.has_value()) {
      homingRequested[*motor] = false;
    }
  }

  _runtime.active = true;
  _runtime.currentAxisIndex = 0;
  startNextAxis(status, homingRequested);
}

void AxisHomingManager::startNextAxis(utl::RobotStatus& status,
                                      std::map<utl::EMotor, bool>& homingRequested) {
  if (_runtime.currentAxisIndex >= status.homing.axes.size()) {
    status.homing.state = utl::EHomingState::Succeeded;
    status.homing.message = "Homing complete.";
    _runtime.active = false;
    return;
  }

  auto& axisStatus = status.homing.axes[_runtime.currentAxisIndex];
  const auto target = [&]() {
    switch (axisStatus.axis) {
      case utl::EHomingAxis::XLeft: return cmd::HomingTarget::XLeft;
      case utl::EHomingAxis::YLeft: return cmd::HomingTarget::YLeft;
      case utl::EHomingAxis::XRight: return cmd::HomingTarget::XRight;
      case utl::EHomingAxis::YRight: return cmd::HomingTarget::YRight;
      case utl::EHomingAxis::Z: return cmd::HomingTarget::Z;
    }
    return cmd::HomingTarget::Z;
  }();

  axisStatus.state = utl::EHomingState::Running;
  axisStatus.message = "Homing in progress";
  status.homing.state = utl::EHomingState::Running;
  status.homing.message = std::format("Homing {}...", homingAxisName(axisStatus.axis));
  _runtime.currentAxisStartedAt = _clock->now();
  try {
    _startAxisFn(target, requireAxisConfig(target));
  } catch (const std::exception& e) {
    finishCurrentAxis(status, homingRequested, utl::EHomingState::Failed, e.what());
    throw;
  }
}

void AxisHomingManager::finishCurrentAxis(utl::RobotStatus& status,
                                          std::map<utl::EMotor, bool>& homingRequested,
                                          const utl::EHomingState state,
                                          std::string message) {
  if (_runtime.currentAxisIndex >= status.homing.axes.size()) {
    return;
  }

  auto& axisStatus = status.homing.axes[_runtime.currentAxisIndex];
  axisStatus.state = state;
  axisStatus.message = std::move(message);
  if (const auto motor = motorFromAxis(axisStatus.axis); motor.has_value()) {
    homingRequested[*motor] = false;
  }

  if (state == utl::EHomingState::Succeeded) {
    ++_runtime.currentAxisIndex;
    startNextAxis(status, homingRequested);
    return;
  }

  status.homing.state = state;
  status.homing.message = axisStatus.message.empty()
                              ? std::format("Homing {} failed.", homingAxisName(axisStatus.axis))
                              : std::format("{}: {}", homingAxisName(axisStatus.axis),
                                            axisStatus.message);
  _runtime.active = false;

  for (std::size_t i = _runtime.currentAxisIndex + 1; i < status.homing.axes.size();
       ++i) {
    if (status.homing.axes[i].state == utl::EHomingState::Pending) {
      status.homing.axes[i].message = "Not started";
    }
  }
}

const AxisHomingManager::AxisConfig&
AxisHomingManager::requireAxisConfig(const cmd::HomingTarget axis) const {
  const auto cfgIt = _axisConfigs.find(axis);
  if (cfgIt == _axisConfigs.end()) {
    utl::throwRuntimeError(
        std::format("Missing cached homing config for {}.", homingTargetName(axis)));
  }
  return cfgIt->second;
}
