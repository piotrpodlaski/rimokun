#pragma once

#include <QPushButton>
#include <QWidget>

#include "GuiCommand.hpp"
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

 signals:
  void buttonPressed(const GuiCommand& command) const;

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
