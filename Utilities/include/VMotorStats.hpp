#pragma once

#include <CommonDefinitions.hpp>

class VMotorStats {
 public:
  virtual ~VMotorStats() = default;
  virtual void setTorque(int value) const = 0;
  virtual void setSpeed(double value) const = 0;
  virtual void setCurrentPosition(double value) const = 0;
  virtual void setTargetPosition(double value) const = 0;
  virtual void setBrake(utl::ELEDState value) const = 0;
  virtual void setEnabled(utl::ELEDState value) const = 0;
  virtual void setMotorName(const std::string& name) const = 0;
  virtual void configure(const utl::SingleMotorStatus& s) = 0;

 protected:
  void configure_backend(const utl::SingleMotorStatus& s) const;
};