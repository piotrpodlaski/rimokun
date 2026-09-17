#pragma once

#include <QString>
#include <QWidget>

#include "GuiCommand.hpp"
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

  void setSpeed(double value) const override;
  void setSpeedRpm(double value) const override;
  void setCurrentPosition(double value) const override;
  void setTargetPosition(double value) const override;
  void setBrake(utl::ELEDState value) const override;
  void setEnabled(utl::ELEDState value) const override;
  void setHoming(utl::ELEDState value) const override;
  void setStatus(utl::ELEDState value) const override;
  void setMotorId(utl::EMotor id) override;
  void setAxisState(utl::EAxisState state) override;
  void setSpeedCommand(double percent, double maxMmPerSec) const override;

 signals:
  void clicked(utl::EMotor motor);
  void commandIssued(const GuiCommand& command);

 public slots:
  void handleUpdate(const utl::RobotStatus&);

 private slots:
  void handleFreeButtonClicked();

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void setFreeStatus(bool isFree, bool isControllable);
  std::string _motorName;
  Ui::MotorStats* _ui;
  utl::EMotor _motorId;
  bool _isFree{false};
};
