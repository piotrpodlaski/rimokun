#include "MotorStats.hpp"

#include <QMetaObject>
#include <format>
#include <print>

#include "spdlog/spdlog.h"
#include "ui_motorstats.h"

MotorStats::MotorStats(QWidget* parent)
    : QWidget(parent), ui(new Ui::MotorStats) {
  ui->setupUi(this);
}

namespace {
QString formatNumber(const double value) {
    return QString::number(value,'f',1);
  }
}

MotorStats::~MotorStats() { delete ui; }
void MotorStats::setTorque(const int value) const {
  ui->torquePercent->setValue(value);
}
void MotorStats::setSpeed(const double value) const {
  ui->speedCms->display(formatNumber(value));
}
void MotorStats::setCurrentPosition(const double value) const {
  ui->curentPosition->display(formatNumber(value));
}
void MotorStats::setTargetPosition(const double value) const {
  ui->targetPosition->display(formatNumber(value));
}
void MotorStats::setBrake(const utl::ELEDState value) const {
  ui->brakeLED->setState(value);
}
void MotorStats::setEnabled(const utl::ELEDState value) const {
  ui->enaLED->setState(value);
}
// void MotorStats::setMotorName(const std::string& name) const {
//   ui->groupBox->setTitle(name.c_str());
// }

void MotorStats::setMotorId(utl::EMotor mot) {
  motorId = mot;
  motorName=magic_enum::enum_name(mot);
  ui->groupBox->setTitle(motorName.c_str());
}

void MotorStats::handleUpdate(const utl::RobotStatus& rs) {
  if (!rs.motors.contains(motorId)) {
    SPDLOG_WARN("Update came without data for {} motor! Doing nothing.",
                motorName);
    return;
  }
  const auto motData = rs.motors.at(motorId);
  configure(motData);
}

