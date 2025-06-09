#pragma once

class VMotorStats {
 public:
  virtual ~VMotorStats() = default;
  virtual void setTorque(int value) const = 0;
  virtual void setSpeed(double value) const = 0;
  virtual void setCurrentPosition(double value) const = 0;
  virtual void setTargetPosition(double value) const = 0;
};