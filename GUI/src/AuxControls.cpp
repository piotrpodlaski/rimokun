#include <AuxControls.hpp>
#include <iostream>

#include "ui_AuxControls.h"

AuxControls::AuxControls(QWidget* parent)
    : QWidget(parent), _ui(new Ui::AuxControls) {
  _ui->setupUi(this);
  _bEnable = _ui->enableButton;
  _bDisable = _ui->disableButton;
  _bLoad = _ui->loadButton;
  _bSave = _ui->saveButton;

  connect(_bEnable, &QPushButton::clicked, this, &AuxControls::handleButtons);
  connect(_bDisable, &QPushButton::clicked, this, &AuxControls::handleButtons);
  connect(_bLoad, &QPushButton::clicked, this, &AuxControls::handleButtons);
  connect(_bSave, &QPushButton::clicked, this, &AuxControls::handleButtons);
}

void AuxControls::processResponse(const YAML::Node response) {
  std::cout << response << std::endl;
}

void AuxControls::handleButtons() {
  const auto sender = QObject::sender();
  YAML::Node command;
  command["type"] = "auxControls";
  if (sender == _ui->enableButton) {
    command["action"] = "enableMotors";
  }
  else if (sender == _ui->disableButton) {
    command["action"] = "disableMotors";
  }
  else
    return;

  emit buttonPressed(command);
}
