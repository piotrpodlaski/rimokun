#pragma once

#include <CommonDefinitions.hpp>

class VMotorStats {
 public:
  virtual ~VMotorStats() = default;
  virtual void setSpeed(double value) const = 0;
  virtual void setSpeedRpm(double value) const = 0;
  virtual void setCurrentPosition(double value) const = 0;
  virtual void setTargetPosition(double value) const = 0;
  virtual void setBrake(utl::ELEDState value) const = 0;
  virtual void setEnabled(utl::ELEDState value) const = 0;
  virtual void setHoming(utl::ELEDState value) const = 0;
  virtual void setStatus(utl::ELEDState value) const = 0;
  virtual void setMotorId(utl::EMotor) = 0;
  virtual void setAxisState(utl::EAxisState state) = 0;
  // percent: [-100, 100], maxMmPerSec: mode maximum linear speed
  virtual void setSpeedCommand(double percent, double maxMmPerSec) const = 0;

 protected:
  void configure(const utl::SingleMotorStatus& s) const;
};

// Non-owning pointers to Qt widgets owned by the MainWindow UI tree.
using MotorStatsMap_t = std::map<utl::EMotor, VMotorStats*>;
