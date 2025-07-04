#include "ToolChanger.hpp"

#include "QMessageBox"
#include "magic_enum/magic_enum.hpp"
#include "spdlog/spdlog.h"
#include "ui_ToolChanger.h"
#include "yaml-cpp/yaml.h"

using namespace utl;

ToolChanger::ToolChanger(QWidget* parent)
    : QWidget(parent), ui(new Ui::ToolChanger) {
  ui->setupUi(this);

  connect(ui->closeButton, &QPushButton::clicked, this,
          &ToolChanger::handleButtons);
  connect(ui->openButon, &QPushButton::clicked, this,
          &ToolChanger::handleButtons);
}
void ToolChanger::setArm(EArm arm) { this->arm = arm; }
void ToolChanger::updateRobotStatus(const RobotStatus& robotStatus) const {
  if (!robotStatus.toolChangers.contains(arm)) {
    SPDLOG_WARN(
        "Status of ToolChanger {} not provided in the update message from "
        "server!",
        magic_enum::enum_name(arm));
    return;
  }
  auto tcStatus = robotStatus.toolChangers.at(arm);
  const std::map<EToolChangerStatusFlags, LedIndicator*>& ledIndicators = {
      {EToolChangerStatusFlags::ProxSen, ui->proxLamp},
      {EToolChangerStatusFlags::OpenSen, ui->openLamp},
      {EToolChangerStatusFlags::ClosedSen, ui->closedLamp},
      {EToolChangerStatusFlags::OpenValve, ui->valveOpenLamp},
      {EToolChangerStatusFlags::ClosedValve, ui->valveClosedLamp},
  };
  for (const auto& [flag, led] : ledIndicators) {
    if (tcStatus.flags.contains(flag))
      led->setState(tcStatus.flags.at(flag));
    else
      led->setState(ELEDState::Off);
  }
  if (*ledIndicators.at(EToolChangerStatusFlags::ClosedValve))
    ui->closeButton->setEnabled(false);
  else
    ui->closeButton->setEnabled(true);

  if (*ledIndicators.at(EToolChangerStatusFlags::OpenValve))
    ui->openButon->setEnabled(false);
  else
    ui->openButon->setEnabled(true);
}
void ToolChanger::handleButtons() {
  SPDLOG_INFO("Button pressed!");
  auto sender = QObject::sender();
  YAML::Node command;
  command["type"] = "toolChanger";
  command["position"] = magic_enum::enum_name(arm);
  auto reply = QMessageBox::No;
  if (sender == ui->closeButton) {
    command["action"] = "close";
    auto msg =
        std::format("Are you sure you want to close the {} tool changer?",
                    magic_enum::enum_name(arm));
    if (*(ui->proxLamp))
      reply = QMessageBox::question(this, "Confirmation", QString(msg.c_str()),
                                    QMessageBox::Yes | QMessageBox::No);
    else {
      msg +=
          " Proximity sensor did not detect the clamp-side tool changer. Is it "
          "intentional?";
      reply = QMessageBox::warning(this, "Warning", QString(msg.c_str()),
                                   QMessageBox::Yes | QMessageBox::No);
    }
  } else if (sender == ui->openButon) {
    command["action"] = "open";
    auto msg = std::format("Are you sure you want to open the {} tool changer?",
                           magic_enum::enum_name(arm));
    reply = QMessageBox::question(this, "Confirmation", QString(msg.c_str()),
                                  QMessageBox::Yes | QMessageBox::No);
  }
  if (reply == QMessageBox::Yes) emit buttonPressed(command);
}
