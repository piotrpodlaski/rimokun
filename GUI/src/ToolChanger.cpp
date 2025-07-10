#include "ToolChanger.hpp"

#include "QMessageBox"
#include "magic_enum/magic_enum.hpp"
#include "spdlog/spdlog.h"
#include "ui_ToolChanger.h"
#include "yaml-cpp/yaml.h"

using namespace utl;

ToolChanger::ToolChanger(QWidget* parent)
    : QWidget(parent), _ui(new Ui::ToolChanger) {
  _ui->setupUi(this);

  connect(_ui->closeButton, &QPushButton::clicked, this,
          &ToolChanger::handleButtons);
  connect(_ui->openButon, &QPushButton::clicked, this,
          &ToolChanger::handleButtons);
}
void ToolChanger::setArm(EArm arm) { this->_arm = arm; }
void ToolChanger::updateRobotStatus(const RobotStatus& robotStatus) const {
  if (!robotStatus.toolChangers.contains(_arm)) {
    SPDLOG_WARN(
        "Status of ToolChanger {} not provided in the update message from "
        "server!",
        magic_enum::enum_name(_arm));
    return;
  }
  auto tcStatus = robotStatus.toolChangers.at(_arm);
  const std::map<EToolChangerStatusFlags, LedIndicator*>& ledIndicators = {
      {EToolChangerStatusFlags::ProxSen, _ui->proxLamp},
      {EToolChangerStatusFlags::OpenSen, _ui->openLamp},
      {EToolChangerStatusFlags::ClosedSen, _ui->closedLamp},
      {EToolChangerStatusFlags::OpenValve, _ui->valveOpenLamp},
      {EToolChangerStatusFlags::ClosedValve, _ui->valveClosedLamp},
  };
  for (const auto& [flag, led] : ledIndicators) {
    if (tcStatus.flags.contains(flag))
      led->setState(tcStatus.flags.at(flag));
    else
      led->setState(ELEDState::Off);
  }
  if (*ledIndicators.at(EToolChangerStatusFlags::ClosedValve))
    _ui->closeButton->setEnabled(false);
  else
    _ui->closeButton->setEnabled(true);

  if (*ledIndicators.at(EToolChangerStatusFlags::OpenValve))
    _ui->openButon->setEnabled(false);
  else
    _ui->openButon->setEnabled(true);
}
void ToolChanger::handleButtons() {
  const auto sender = QObject::sender();
  YAML::Node command;
  command["type"] = "toolChanger";
  command["position"] = magic_enum::enum_name(_arm);
  auto reply = QMessageBox::No;
  if (sender == _ui->closeButton) {
    command["action"] = "close";
    auto msg =
        std::format("Are you sure you want to close the {} tool changer?",
                    magic_enum::enum_name(_arm));
    if (*(_ui->proxLamp))
      reply = QMessageBox::question(this, "Confirmation", QString(msg.c_str()),
                                    QMessageBox::Yes | QMessageBox::No);
    else {
      msg +=
          " Proximity sensor did not detect the clamp-side tool changer. Is it "
          "intentional?";
      reply = QMessageBox::warning(this, "Warning", QString(msg.c_str()),
                                   QMessageBox::Yes | QMessageBox::No);
    }
  } else if (sender == _ui->openButon) {
    command["action"] = "open";
    const auto msg =
        std::format("Are you sure you want to open the {} tool changer?",
                    magic_enum::enum_name(_arm));
    reply = QMessageBox::question(this, "Confirmation", QString(msg.c_str()),
                                  QMessageBox::Yes | QMessageBox::No);
  }
  if (reply == QMessageBox::Yes) emit buttonPressed(command);
}
