#pragma once

#include <QPushButton>
#include <QWidget>

#include "GuiCommand.hpp"
#include "HomingWindow.hpp"
#include "LedIndicator.hpp"
#include "RobotStatusViewModel.hpp"

namespace Ui {
class ResetControls;
}

class ResetControls : public QWidget {
  Q_OBJECT
 public:
  ResetControls(QWidget* parent);
  void applyViewModel(const ResetControlsViewModel& vm) const;
 private slots:
  void handleButtons();
  void openHomingWindow();

 signals:
  void buttonPressed(const GuiCommand& command) const;

 private:
  Ui::ResetControls* _ui;
  QPushButton* _bResetContec;
  QPushButton* _bResetMotors;
  QPushButton* _bResetControlPanel;
  QPushButton* _bEnableAllMotors;
  QPushButton* _bDisableAllMotors;
  QPushButton* _bHoming;
  LedIndicator* _lServerLed;
  LedIndicator* _lContecLed;
  LedIndicator* _lMotorLed;
  LedIndicator* _lControlPanelLed;
  LedIndicator* _lMotorPowerLed;
  QPushButton* _bEmergencyStop;
  HomingWindow* _homingWindow{nullptr};
};
