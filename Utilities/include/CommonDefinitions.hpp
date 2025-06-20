#pragma once

#include <map>
#include <string>

class VMotorStats;
namespace utl {
enum class EMotor { XLeft, XRight, YLeft, YRight, ZLeft, ZRight };
enum class ELEDState { On, Off, Error, ErrorBlinking };
enum class EMotorStatusFlags { BrakeApplied, Enabled, Error };
enum class EArm { Left, Right };
enum class EToolChangerStatusFlags {
  ProxSen,
  OpenSen,
  ClosedSen,
  OpenValve,
  ClosedValve
};

std::string getMotorName(EMotor em);
EMotor getMotorType(std::string name);

struct SingleMotorStatus {
  double currentPosition{0};
  double targetPosition{0};
  double speed{0};
  int torque{0};
  std::map<EMotorStatusFlags, ELEDState> flags;
};

struct ToolChangerStatus {
  std::map<EToolChangerStatusFlags, ELEDState> flags;
};

struct RobotStatus {
  std::map<EMotor, SingleMotorStatus> motors;
  std::map<EArm, ToolChangerStatus> toolChangers;
};

typedef std::map<EMotor, VMotorStats*> MotorStatsMap_t;

}  // namespace utl
