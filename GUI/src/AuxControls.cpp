#include <AuxControls.hpp>
#include <iostream>

#include "spdlog/spdlog.h"
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
  SPDLOG_WARN(response.as<std::string>());
}

void AuxControls::handleButtons() {
  const auto sender = QObject::sender();
  YAML::Node command;
  command["type"] = "reset";
  if (sender == _ui->enableButton) {
    command["system"] = "Contec";
  }
  else if (sender == _ui->disableButton) {
    command["action"] = "disableMotors";
  }
  else
    return;

  emit buttonPressed(command);
}
