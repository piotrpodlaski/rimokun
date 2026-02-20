#pragma once

#include <QString>
#include <QWidget>

#include "VMotorStats.hpp"

class QMouseEvent;

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
  void setStatus(utl::ELEDState value) const override;
  void setMotorId(utl::EMotor id) override;

 signals:
  void clicked(utl::EMotor motor);

 public slots:
  void handleUpdate(const utl::RobotStatus&);

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  std::string _motorName;
  Ui::MotorStats* _ui;
  utl::EMotor _motorId;
};
