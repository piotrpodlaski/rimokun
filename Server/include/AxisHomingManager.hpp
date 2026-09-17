#pragma once

#include <CommonDefinitions.hpp>
#include <CommandInterface.hpp>
#include <IClock.hpp>

#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class AxisHomingManager {
 public:
  struct AxisConfig {
    utl::EMotor motorId;
    std::int32_t speedStepsPerSec{0};
    std::int32_t acceleration{0};
    std::int32_t homeSeekingMode{0};
    std::int32_t startingDirection{0};
  };

  using StartAxisFn = std::function<void(cmd::HomingTarget, const AxisConfig&)>;

  static std::vector<cmd::HomingTarget> loadOrder();
  static std::map<cmd::HomingTarget, AxisConfig> loadAxisConfigs();
  static std::chrono::milliseconds loadTimeout();
  static utl::EHomingAxis axisFromTarget(cmd::HomingTarget target);
  static std::optional<utl::EMotor> motorFromAxis(utl::EHomingAxis axis);

  AxisHomingManager(std::shared_ptr<IClock> clock, std::chrono::milliseconds timeout,
                    std::vector<cmd::HomingTarget> order,
                    std::map<cmd::HomingTarget, AxisConfig> axisConfigs,
                    StartAxisFn startAxisFn);

  void start(cmd::HomingTarget target, utl::RobotStatus& status,
             std::map<utl::EMotor, bool>& homingRequested);
  void cancel(utl::RobotStatus& status,
              std::map<utl::EMotor, bool>& homingRequested);
  void advanceFromStatus(utl::RobotStatus& status,
                         std::map<utl::EMotor, bool>& homingRequested);

 private:
  struct SessionRuntime {
    bool active{false};
    std::size_t currentAxisIndex{0};
    IClock::time_point currentAxisStartedAt{};
  };

  void beginSession(const std::vector<cmd::HomingTarget>& axes, utl::RobotStatus& status,
                    std::map<utl::EMotor, bool>& homingRequested);
  void startNextAxis(utl::RobotStatus& status,
                     std::map<utl::EMotor, bool>& homingRequested);
  void finishCurrentAxis(utl::RobotStatus& status,
                         std::map<utl::EMotor, bool>& homingRequested,
                         utl::EHomingState state, std::string message = {});
  const AxisConfig& requireAxisConfig(cmd::HomingTarget axis) const;

  std::shared_ptr<IClock> _clock;
  std::chrono::milliseconds _timeout;
  std::vector<cmd::HomingTarget> _order;
  std::map<cmd::HomingTarget, AxisConfig> _axisConfigs;
  StartAxisFn _startAxisFn;
  SessionRuntime _runtime;
};
