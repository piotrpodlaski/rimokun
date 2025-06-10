#include "MotorStats.hpp"
#include <format>
#include <print>

#include <QMetaObject>

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
void MotorStats::setMotorName(const std::string& name) const {
  ui->groupBox->setTitle(name.c_str());
}

void MotorStats::configure(const utl::SingleMotorStatus& s) {
  QMetaObject::invokeMethod(
      this, [&]() { configure_backend(s); }, Qt::QueuedConnection);
}
