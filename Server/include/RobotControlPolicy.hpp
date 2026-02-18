#pragma once

#include <map>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "CommonDefinitions.hpp"
#include "MachineComponent.hpp"
#include "MotorControl.hpp"

class IRobotControlPolicy {
 public:
  using SignalMap = std::map<std::string, bool>;

  struct MotorIntent {
    utl::EMotor motorId;
    std::optional<MotorControlMode> mode;
    std::optional<MotorControlDirection> direction;
    std::optional<std::int32_t> speed;
    std::optional<std::int32_t> position;
    bool startMovement{false};
    bool stopMovement{false};
  };

  struct ControlDecision {
    std::optional<SignalMap> outputs;
    std::vector<MotorIntent> motorIntents;
    bool setToolChangerErrorBlinking{false};
  };

  virtual ~IRobotControlPolicy() = default;

  virtual ControlDecision decide(const std::optional<SignalMap>& inputs,
                                 const std::optional<SignalMap>& outputs,
                                 MachineComponent::State contecState,
                                 const utl::RobotStatus& robotStatus) const = 0;
};

class DefaultRobotControlPolicy final : public IRobotControlPolicy {
 public:
  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         const std::optional<SignalMap>& outputs,
                         MachineComponent::State contecState,
                         const utl::RobotStatus& robotStatus) const override;
};

class RimoKunControlPolicy final : public IRobotControlPolicy {
 public:
  RimoKunControlPolicy();

  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         const std::optional<SignalMap>& outputs,
                         MachineComponent::State contecState,
                         const utl::RobotStatus& robotStatus) const override;

 private:
  struct AxisConfig {
    double neutralAxisActivationThreshold{0.10};
    double speedUpdateAxisDeltaThreshold{0.02};
    double maxLinearSpeedMmPerSec{80.0};
    double stepsPerMm{100.0};
  };

 struct MotionConfig {
    AxisConfig leftArmX;
    AxisConfig leftArmY;
    AxisConfig rightArmX;
    AxisConfig rightArmY;
    AxisConfig gantryZ;
  };

  struct WarningState {
    bool missingContecStatus{false};
    bool missingMotorControlStatus{false};
    bool missingControlPanelStatus{false};
    bool contecUnavailable{false};
    bool motorControlUnavailable{false};
    bool controlPanelUnavailable{false};
    bool missingInputSignals{false};
  };

  struct MotorRuntimeState {
    bool wasActive{false};
    bool hasLastAxis{false};
    double lastAxis{0.0};
    std::optional<MotorControlDirection> lastDirection;
  };

  void warnOnce(bool& flag, const std::string& message) const;
  void setWarningFlag(bool& flag, bool value) const;

  MotionConfig _motion;
  mutable WarningState _warningState;
  mutable std::mutex _warningMutex;
  mutable std::map<utl::EMotor, MotorRuntimeState> _motorRuntime;
  mutable std::mutex _runtimeMutex;
};
