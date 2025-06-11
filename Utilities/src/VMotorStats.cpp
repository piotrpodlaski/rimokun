#include "VMotorStats.hpp"

void VMotorStats::configure(const utl::SingleMotorStatus& s) const {
  setCurrentPosition(s.currentPosition);
  setTargetPosition(s.targetPosition);
  setTorque(s.torque);
  setSpeed(s.speed);
  if (s.flags.contains(utl::EMotorStatusFlags::BrakeApplied))
    setBrake(s.flags.at(utl::EMotorStatusFlags::BrakeApplied));
  else
    setBrake(utl::ELEDState::Off);
  if (s.flags.contains(utl::EMotorStatusFlags::Enabled))
    setEnabled(s.flags.at(utl::EMotorStatusFlags::Enabled));
  else
    setEnabled(utl::ELEDState::Off);
}
