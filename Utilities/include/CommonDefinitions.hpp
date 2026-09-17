#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace utl {
enum class EMotor { XLeft, XRight, YLeft, YRight, ZLeft, ZRight };
enum class ELEDState { On, Off, Warning, Error, ErrorBlinking };
enum class EMotorStatusFlags {
  BrakeApplied,
  Enabled,
  Warning,
  Alarm,
  Homing,
  Free
};
enum class EArm { Left, Right, Gantry };
enum class EToolChangerStatusFlags {
  ProxSen,
  OpenSen,
  ClosedSen,
  OpenValve,
  ClosedValve
};

enum class EToolChangerAction { Open, Close };

enum class ERobotComponent {Contec, MotorControl, ControlPanel};

enum class EAxisState { Locked, Slow, Fast };
enum class EHomingAxis { XLeft, YLeft, XRight, YRight, Z };
enum class EHomingState {
  Idle,
  Pending,
  Running,
  Succeeded,
  Failed,
  TimedOut,
  Cancelled
};

// Known Contec digital input signal names.  All keys in Machine.inputMapping
// must correspond to one of these values (validated at startup).
enum class EInputSignal {
  motorPower,
  safetyON,
  robotAUX1,
  robotAUX2,
  localAUX1,
  localAUX2,
  localAUX3,
  localAUX4,
  localAUX5,
  tclProx,
  tclOpen,
  tclClose,
  tcrProx,
  tcrOpen,
  tcrClose,
  cpuOverride
};

// Known Contec digital output signal names.  All keys in Machine.outputMapping
// must correspond to one of these values (validated at startup).
enum class EOutputSignal {
  toolChangerLeft,
  toolChangerRight,
  heartBeat,
  safetyReset,
  light1,
  light2,
  alarmIndicator
};

using SignalMap = std::map<std::string, bool, std::less<>>;

std::string getMotorName(EMotor em);
EMotor getMotorType(std::string name);

struct SingleMotorStatus {
  double currentPosition{0};
  double targetPosition{0};
  double speed{0};
  double speedRpm{0};
  int torque{0};
  ELEDState state{ELEDState::Off};
  std::string warningDescription;
  std::string alarmDescription;
  std::map<EMotorStatusFlags, ELEDState> flags;
  bool freeControllable{false};
  double speedCommandPercent{0};         // [-100, 100]; 0 when locked/idle
  double modeMaxLinearSpeedMmPerSec{0};  // max speed for the current mode
};

struct JoystickStatus {
  double x;
  double y;
  bool btn;
};


struct ToolChangerStatus {
  std::map<EToolChangerStatusFlags, ELEDState> flags;
};

struct HomingAxisStatus {
  EHomingAxis axis{EHomingAxis::Z};
  EHomingState state{EHomingState::Idle};
  std::string message;
};

struct HomingSessionStatus {
  EHomingState state{EHomingState::Idle};
  std::vector<HomingAxisStatus> axes;
  std::string message;
};

struct RobotStatus {
  std::map<EMotor, SingleMotorStatus> motors;
  std::map<EArm, ToolChangerStatus> toolChangers;
  std::map<ERobotComponent, ELEDState> robotComponents;
  std::map<EArm, JoystickStatus> joystics;
  std::optional<bool> motorPower;
  std::optional<bool> cpuOverride;
  std::optional<bool> heartBeatRunning;
  std::optional<bool> safetyOn;
  std::map<EArm, EAxisState> armStates;
  HomingSessionStatus homing;
};

inline constexpr char kCpuOverrideWarningMessage[] =
    "CPU OK signal override active, operation suspended. Disable override to keep "
    "using the system.";

}  // namespace utl

#ifndef RIMOKUN_SKIP_YAML_EXTENSIONS
#include <YamlExtensions.hpp>
#endif
