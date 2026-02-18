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

void AuxControls::processResponse(const GuiResponse& response) {
  if (response.ok) {
    SPDLOG_INFO("AuxControls response OK: {}", response.message);
  } else {
    SPDLOG_WARN("AuxControls response error: {}", response.message);
  }
}

void AuxControls::handleButtons() {
  const auto sender = QObject::sender();
  GuiCommand command;
  if (sender == _ui->enableButton) {
    command.payload = GuiReconnectCommand{.component = utl::ERobotComponent::Contec};
  }
  else if (sender == _ui->disableButton) {
    nlohmann::json raw{
        {"type", "disableMotors"},
    };
    command.payload = GuiRawCommand{.node = raw};
  }
  else
    return;

  emit buttonPressed(command);
}
