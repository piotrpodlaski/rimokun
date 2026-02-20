#include <ResetControls.hpp>
#include "spdlog/spdlog.h"
#include "ui_ResetControls.h"

using namespace utl;

ResetControls::ResetControls(QWidget* parent) : QWidget(parent), _ui(new Ui::ResetControls) {
  _ui->setupUi(this);

  _bResetContec=_ui->resetContec;
  _bResetMotors=_ui->resetMotors;
  _bResetControlPanel=_ui->resetControlPanel;
  _bEnableAllMotors = _ui->enableAllMotors;
  _bDisableAllMotors = _ui->disableAllMotors;

  _lServerLed = _ui->ledServer;
  _lContecLed = _ui->ledContec;
  _lMotorLed = _ui->ledMotors;
  _lControlPanelLed = _ui->ledControlPanel;

  _lServerLed->setState(ELEDState::Error);
  _bResetContec->setEnabled(false);
  _bResetMotors->setEnabled(false);
  _bResetControlPanel->setEnabled(false);
  _bEnableAllMotors->setEnabled(false);
  _bDisableAllMotors->setEnabled(false);

  connect(_bResetContec, &QPushButton::clicked, this, &ResetControls::handleButtons);
  connect(_bResetMotors, &QPushButton::clicked, this, &ResetControls::handleButtons);
  connect(_bResetControlPanel, &QPushButton::clicked, this, &ResetControls::handleButtons);
  connect(_bEnableAllMotors, &QPushButton::clicked, this,
          &ResetControls::handleButtons);
  connect(_bDisableAllMotors, &QPushButton::clicked, this,
          &ResetControls::handleButtons);
}

void ResetControls::applyViewModel(const ResetControlsViewModel& vm) const {
  _lServerLed->setState(vm.server);
  _lContecLed->setState(vm.contec);
  _lMotorLed->setState(vm.motor);
  _lControlPanelLed->setState(vm.controlPanel);
  _bResetContec->setEnabled(vm.resetContecEnabled);
  _bResetMotors->setEnabled(vm.resetMotorEnabled);
  _bResetControlPanel->setEnabled(vm.resetControlPanelEnabled);
  _bEnableAllMotors->setEnabled(vm.enableAllMotorsEnabled);
  _bDisableAllMotors->setEnabled(vm.disableAllMotorsEnabled);
}

void ResetControls::handleButtons() {
  const auto sender = dynamic_cast<QPushButton*>(QObject::sender());
  GuiCommand command;
  if (sender == _ui->resetContec) {
    command.payload = GuiReconnectCommand{.component = ERobotComponent::Contec};
  }
  else if (sender == _ui->resetMotors) {
    command.payload =
        GuiReconnectCommand{.component = ERobotComponent::MotorControl};
  }
  else if (sender == _ui->resetControlPanel) {
    command.payload =
        GuiReconnectCommand{.component = ERobotComponent::ControlPanel};
  }
  else if (sender == _ui->enableAllMotors) {
    command.payload = GuiSetAllMotorsEnabledCommand{.enabled = true};
  }
  else if (sender == _ui->disableAllMotors) {
    command.payload = GuiSetAllMotorsEnabledCommand{.enabled = false};
  }
  else
    return;

  emit buttonPressed(command);
}
