#include "MotorStats.hpp"

#include "ui_motorstats.h"

MotorStats::MotorStats(QWidget* parent)
    : QWidget(parent), ui(new Ui::MotorStats) {
  ui->setupUi(this);
}

MotorStats::~MotorStats() { delete ui; }
void MotorStats::setTorque(const int value) const {
  ui->torquePercent->setValue(value);
}
void MotorStats::setSpeed(const double value) const {
  ui->speedCms->display(value);
}
void MotorStats::setCurrentPosition(const double value) const {
  ui->curentPosition->display(value);
}
void MotorStats::setTargetPosition(const double value) const {
  ui->targetPosition->display(value);
}
void MotorStats::setBrake(const utl::ELEDState value) const {
  ui->brakeLED->setState(value);
}
void MotorStats::setEnabled(const utl::ELEDState value) const {
  ui->enaLED->setState(value);
}
void MotorStats::setMotorName(const QString& name) const {
  ui->groupBox->setTitle(name);
}