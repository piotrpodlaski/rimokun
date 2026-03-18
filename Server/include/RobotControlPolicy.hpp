#pragma once

#include <map>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "CommonDefinitions.hpp"
#include "MachineComponent.hpp"
#include "MotorControl.hpp"

class IRobotControlPolicy {
 public:
  using SignalMap = utl::SignalMap;

  struct MotorIntent {
    utl::EMotor motorId;
    std::optional<MotorControlMode> mode;
    std::optional<MotorControlDirection> direction;
    std::optional<std::int32_t> acceleration;
    std::optional<std::int32_t> deceleration;
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
                                 const utl::RobotStatus& robotStatus) = 0;
};

class RimoKunControlPolicy final : public IRobotControlPolicy {
 public:
  RimoKunControlPolicy();

  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         const std::optional<SignalMap>& outputs,
                         MachineComponent::State contecState,
                         const utl::RobotStatus& robotStatus) override;

 private:
  struct AxisConfig {
    double neutralAxisActivationThreshold{0.10};
    double speedUpdateAxisDeltaThreshold{0.02};
    double maxLinearSpeedMmPerSec{80.0};
    double stepsPerMm{100.0};
    bool invertAxis{false};
    std::int32_t acceleration001MsPerKHz{24575};
    std::int32_t deceleration001MsPerKHz{24575};
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
    bool motorAlarmActive{false};
    bool missingInputSignals{false};
  };

  struct MotorRuntimeState {
    bool wasActive{false};
    bool hasLastAxis{false};
    double lastAxis{0.0};
    std::optional<MotorControlDirection> lastDirection;
  };

  void warnOnce(bool& flag, const std::string& message);
  void setWarningFlag(bool& flag, bool value);

  MotionConfig _motion;
  WarningState _warningState;
  std::map<utl::EMotor, MotorRuntimeState> _motorRuntime;
};
