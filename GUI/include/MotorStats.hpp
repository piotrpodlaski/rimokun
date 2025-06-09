#pragma once

#include <QString>
#include <QWidget>

#include "VMotorStats.hpp"

namespace Ui {
class MotorStats;
}

class MotorStats : public QWidget, VMotorStats {
  Q_OBJECT

 public:
  explicit MotorStats(QWidget* parent = nullptr);
  ~MotorStats();

  void setTorque(int value) const override;
  void setSpeed(double value) const override;
  void setCurrentPosition(double value) const override;
  void setTargetPosition(double value) const override;
  void setMotorName(const QString& name) const;

 private:
  Ui::MotorStats* ui;
};