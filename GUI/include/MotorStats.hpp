#pragma once

#include <QString>
#include <QWidget>

#include "VMotorStats.hpp"

namespace Ui {
class MotorStats;
}

class MotorStats : public QWidget, public VMotorStats {
  Q_OBJECT

 public:
  explicit MotorStats(QWidget* parent = nullptr);
  ~MotorStats() override;

  void setTorque(int value) const override;
  void setSpeed(double value) const override;
  void setCurrentPosition(double value) const override;
  void setTargetPosition(double value) const override;
  void setBrake(utl::ELEDState value) const override;
  void setEnabled(utl::ELEDState value) const override;
  void setMotorId(utl::EMotor id) override;

 public slots:
  void handleUpdate(const utl::RobotStatus&);

 private:
  std::string motorName;
  Ui::MotorStats* ui;
  utl::EMotor motorId;
};