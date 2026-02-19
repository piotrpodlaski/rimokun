#include "MotorStats.hpp"
#include "magic_enum/magic_enum.hpp"

#include <format>
#include <print>

#include "spdlog/spdlog.h"
#include "ui_motorstats.h"

MotorStats::MotorStats(QWidget* parent)
    : QWidget(parent), _ui(new Ui::MotorStats) {
  _ui->setupUi(this);
}

namespace {
QString formatNumber(const double value) {
  return QString::number(value, 'f', 1);
}
}  // namespace

MotorStats::~MotorStats() { delete _ui; }
void MotorStats::setTorque(const int value) const {
  _ui->torquePercent->setValue(value);
}
void MotorStats::setSpeed(const double value) const {
  _ui->speedCms->display(formatNumber(value));
}
void MotorStats::setCurrentPosition(const double value) const {
  _ui->curentPosition->display(formatNumber(value));
}
void MotorStats::setTargetPosition(const double value) const {
  _ui->targetPosition->display(formatNumber(value));
}
void MotorStats::setBrake(const utl::ELEDState value) const {
  _ui->brakeLED->setState(value);
}
void MotorStats::setEnabled(const utl::ELEDState value) const {
  (void)value;
}
void MotorStats::setStatus(const utl::ELEDState value) const {
  _ui->statusLED->setState(value);
}
// void MotorStats::setMotorName(const std::string& name) const {
//   ui->groupBox->setTitle(name.c_str());
// }

void MotorStats::setMotorId(utl::EMotor mot) {
  _motorId = mot;
  _motorName = magic_enum::enum_name(mot);
  _ui->groupBox->setTitle(_motorName.c_str());
}

void MotorStats::handleUpdate(const utl::RobotStatus& rs) {
  if (!rs.motors.contains(_motorId)) {
    SPDLOG_WARN("Update came without data for {} motor! Doing nothing.",
                _motorName);
    return;
  }
  const auto motData = rs.motors.at(_motorId);
  configure(motData);
}
