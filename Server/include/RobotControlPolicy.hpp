#pragma once

#include <chrono>
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

  struct MotorSpeedCommand {
    double speedCommandPercent{0};        // [-100, 100]; 0 when locked/idle
    double modeMaxLinearSpeedMmPerSec{0}; // max speed for the current mode
  };

  struct ControlDecision {
    std::optional<SignalMap> outputs;
    std::vector<MotorIntent> motorIntents;
    bool setToolChangerErrorBlinking{false};
    std::map<utl::EArm, utl::EAxisState> armStates;
    std::map<utl::EMotor, MotorSpeedCommand> motorSpeedCommands;
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
    double slowMaxLinearSpeedMmPerSec{40.0};
    double fastMaxLinearSpeedMmPerSec{80.0};
    double stepsPerMm{100.0};
    std::int32_t slowAcceleration001MsPerKHz{24575};
    std::int32_t fastAcceleration001MsPerKHz{24575};
    std::int32_t slowDeceleration001MsPerKHz{24575};
    std::int32_t fastDeceleration001MsPerKHz{24575};
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

  static std::optional<utl::EArm> motorToArm(utl::EMotor motor);

  MotionConfig _motion;
  WarningState _warningState;
  std::map<utl::EMotor, MotorRuntimeState> _motorRuntime;

  std::int64_t _autoLockTimeoutMs{5000};
  std::map<utl::EArm, utl::EAxisState> _armStates{
      {utl::EArm::Left, utl::EAxisState::Locked},
      {utl::EArm::Right, utl::EAxisState::Locked},
      {utl::EArm::Gantry, utl::EAxisState::Locked},
  };
  std::map<utl::EArm, std::chrono::steady_clock::time_point> _lastMovementTime;
  std::map<utl::EArm, bool> _prevButtonState{
      {utl::EArm::Left, false},
      {utl::EArm::Right, false},
      {utl::EArm::Gantry, false},
  };
};
