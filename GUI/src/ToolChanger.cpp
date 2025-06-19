#include "ToolChanger.hpp"

#include "magic_enum/magic_enum.hpp"
#include "spdlog/spdlog.h"
#include "ui_ToolChanger.h"

using namespace utl;

ToolChanger::ToolChanger(QWidget* parent)
    : QWidget(parent), ui(new Ui::ToolChanger) {
  ui->setupUi(this);
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
  for (const auto& [flag, status] : tcStatus.flags) {
    ledIndicators.at(flag)->setState(status);
  }
  if (ledIndicators.at(EToolChangerStatusFlags::ClosedValve)->state()==ELEDState::On)
    ui->closeButton->setEnabled(false);
  else
    ui->closeButton->setEnabled(true);

  if (ledIndicators.at(EToolChangerStatusFlags::OpenValve)->state()==ELEDState::On)
    ui->openButon->setEnabled(false);
  else
    ui->openButon->setEnabled(true);
}
