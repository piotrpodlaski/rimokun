#include <ResetControls.hpp>
#include "spdlog/spdlog.h"
#include "ui_ResetControls.h"
#include <yaml-cpp/yaml.h>

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
  YAML::Node command;
  command["type"] = "reset";
  if (sender == _ui->resetContec) {
    command["system"] = "Contec";
  }
  else if (sender == _ui->resetMotors) {
    command["system"] = "MotorControl";
  }
  else if (sender == _ui->resetControlPanel) {
    command["system"] = "ControlPanel";
  }
  else
    return;

  emit buttonPressed(command);
}

void ResetControls::updateRobotStatus(
    const RobotStatus& robotStatus) const {
  if (robotStatus.robotComponents.contains(ERobotComponent::Contec)) {
    auto status = robotStatus.robotComponents.at(ERobotComponent::Contec);
    _lContecLed->setState(status);
    _bResetContec->setEnabled(status==ELEDState::Error);
  }
  if (robotStatus.robotComponents.contains(ERobotComponent::MotorControl)) {
    auto status = robotStatus.robotComponents.at(ERobotComponent::MotorControl);
    _lMotorLed->setState(status);
    _bResetMotors->setEnabled(status==ELEDState::Error);
  }
  if (robotStatus.robotComponents.contains(ERobotComponent::ControlPanel)) {
    auto status = robotStatus.robotComponents.at(ERobotComponent::ControlPanel);
    _lControlPanelLed->setState(status);
    _bResetControlPanel->setEnabled(status==ELEDState::Error);
  }
  _lServerLed->setState(ELEDState::On);
}
void ResetControls::announceServerError() {
  _lServerLed->setState(ELEDState::Error);
  _lContecLed->setState(ELEDState::Off);
  _lMotorLed->setState(ELEDState::Off);
  _lControlPanelLed->setState(ELEDState::Off);

  _bResetContec->setEnabled(false);
  _bResetMotors->setEnabled(false);
  _bResetControlPanel->setEnabled(false);

}