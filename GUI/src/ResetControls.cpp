#include <ResetControls.hpp>
#include <RobotStatusViewModel.hpp>
#include <optional>
#include "spdlog/spdlog.h"
#include "ui_ResetControls.h"

using namespace utl;

ResetControls::ResetControls(QWidget* parent) : QWidget(parent), _ui(new Ui::ResetControls) {
  _ui->setupUi(this);

  _bResetContec=_ui->resetContec;
  _bResetMotors=_ui->resetMotors;
  _bResetControlPanel=_ui->resetControlPanel;

  _lServerLed = _ui->ledServer;
  _lContecLed = _ui->ledContec;
  _lMotorLed = _ui->ledMotors;
  _lControlPanelLed = _ui->ledControlPanel;

  _lServerLed->setState(ELEDState::Error);
  _bResetContec->setEnabled(false);
  _bResetMotors->setEnabled(false);
  _bResetControlPanel->setEnabled(false);

  connect(_bResetContec, &QPushButton::clicked, this, &ResetControls::handleButtons);
  connect(_bResetMotors, &QPushButton::clicked, this, &ResetControls::handleButtons);
  connect(_bResetControlPanel, &QPushButton::clicked, this, &ResetControls::handleButtons);
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
  else
    return;

  emit buttonPressed(command);
}

void ResetControls::updateRobotStatus(
    const RobotStatus& robotStatus) const {
  const auto vm =
      RobotStatusViewModel::resetControlsForStatus(std::make_optional(robotStatus));
  _lServerLed->setState(vm.server);
  _lContecLed->setState(vm.contec);
  _lMotorLed->setState(vm.motor);
  _lControlPanelLed->setState(vm.controlPanel);
  _bResetContec->setEnabled(vm.resetContecEnabled);
  _bResetMotors->setEnabled(vm.resetMotorEnabled);
  _bResetControlPanel->setEnabled(vm.resetControlPanelEnabled);
}
void ResetControls::announceServerError() {
  const auto vm = RobotStatusViewModel::resetControlsForStatus(std::nullopt);
  _lServerLed->setState(vm.server);
  _lContecLed->setState(vm.contec);
  _lMotorLed->setState(vm.motor);
  _lControlPanelLed->setState(vm.controlPanel);
  _bResetContec->setEnabled(vm.resetContecEnabled);
  _bResetMotors->setEnabled(vm.resetMotorEnabled);
  _bResetControlPanel->setEnabled(vm.resetControlPanelEnabled);
}
