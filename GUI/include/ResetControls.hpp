#pragma once

#include <QPushButton>
#include <QWidget>

#include "LedIndicator.hpp"
#include "yaml-cpp/node/node.h"

namespace Ui {
class ResetControls;
}

class ResetControls : public QWidget {
  Q_OBJECT
 public:
  ResetControls(QWidget* parent);
 private slots:
  void handleButtons();
 public slots:
  void updateRobotStatus(const utl::RobotStatus& robotStatus) const;
  void announceServerError();

 signals:
  void buttonPressed(YAML::Node button) const;

 private:
  Ui::ResetControls* _ui;
  QPushButton* _bResetContec;
  QPushButton* _bResetMotors;
  QPushButton* _bResetControlPanel;
  LedIndicator* _lServerLed;
  LedIndicator* _lContecLed;
  LedIndicator* _lMotorLed;
  LedIndicator* _lControlPanelLed;
};