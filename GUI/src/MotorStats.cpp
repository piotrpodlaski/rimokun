#include "MotorStats.hpp"
#include "magic_enum/magic_enum.hpp"

#include <format>
#include <print>

#include <QMouseEvent>
#include <QEvent>

#include "spdlog/spdlog.h"
#include "ui_motorstats.h"

MotorStats::MotorStats(QWidget* parent)
    : QWidget(parent), _ui(new Ui::MotorStats) {
  _ui->setupUi(this);
  setCursor(Qt::PointingHandCursor);
  setToolTip("Click to open motor monitor");
  setStyleSheet(
      "MotorStats:hover {"
      "  border: 1px solid #4c93d6;"
      "  border-radius: 6px;"
      "  background-color: rgba(76, 147, 214, 0.08);"
      "}");
  const auto children = findChildren<QWidget*>();
  for (auto* child : children) {
    child->installEventFilter(this);
    child->setCursor(Qt::PointingHandCursor);
  }
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
  _ui->enabledLED->setState(value);
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

void MotorStats::mousePressEvent(QMouseEvent* event) {
  if (event && event->button() == Qt::LeftButton) {
    emit clicked(_motorId);
    event->accept();
    return;
  }
  QWidget::mousePressEvent(event);
}

bool MotorStats::eventFilter(QObject* watched, QEvent* event) {
  if (watched && event && event->type() == QEvent::MouseButtonPress) {
    const auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
    if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
      emit clicked(_motorId);
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}
