#include "MotorStats.hpp"
#include "SpeedCommandBar.hpp"
#include "EnumStrings.hpp"
#include "StyledPopup.hpp"

#include <format>
#include <print>

#include <QMouseEvent>
#include <QEvent>
#include <QMessageBox>
#include <QPushButton>

#include "spdlog/spdlog.h"
#include "ui_motorstats.h"

namespace {
std::optional<utl::EArm> motorToArm(const utl::EMotor motor) {
  switch (motor) {
    case utl::EMotor::XLeft:
    case utl::EMotor::YLeft:
      return utl::EArm::Left;
    case utl::EMotor::XRight:
    case utl::EMotor::YRight:
      return utl::EArm::Right;
    case utl::EMotor::ZLeft:
    case utl::EMotor::ZRight:
      return utl::EArm::Gantry;
  }
  return std::nullopt;
}
}  // namespace

MotorStats::MotorStats(QWidget* parent)
    : QWidget(parent), _ui(new Ui::MotorStats) {
  _ui->setupUi(this);
  setAxisState(utl::EAxisState::Locked);
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
  _ui->freeButton->setEnabled(false);
  connect(_ui->freeButton, &QPushButton::clicked, this,
          &MotorStats::handleFreeButtonClicked);
}

namespace {
QString formatNumber(const double value) {
  return QString::number(value, 'f', 1);
}
}  // namespace

MotorStats::~MotorStats() { delete _ui; }
void MotorStats::setSpeed(const double value) const {
  _ui->speedCms->display(formatNumber(value));
}
void MotorStats::setSpeedRpm(const double value) const {
  _ui->speedRpm->display(formatNumber(value));
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
void MotorStats::setHoming(const utl::ELEDState value) const {
  _ui->homingLED->setState(value);
}
void MotorStats::setStatus(const utl::ELEDState value) const {
  _ui->statusLED->setState(value);
}
// void MotorStats::setMotorName(const std::string& name) const {
//   ui->groupBox->setTitle(name.c_str());
// }

void MotorStats::setSpeedCommand(const double percent, const double maxMmPerSec) const {
  _ui->speedCommandBar->setValues(percent, maxMmPerSec);
}

void MotorStats::setMotorId(utl::EMotor mot) {
  _motorId = mot;
  _motorName = utl::enumToString(mot);
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
  const auto freeIt = motData.flags.find(utl::EMotorStatusFlags::Free);
  setFreeStatus(freeIt != motData.flags.end() &&
                    freeIt->second == utl::ELEDState::On,
                motData.freeControllable);
  if (_isFree) {
    // setAxisState gives FREE visual precedence regardless of axis-mode data.
    setAxisState(utl::EAxisState::Locked);
    return;
  }

  const auto arm = motorToArm(_motorId);
  if (arm.has_value() && rs.armStates.contains(*arm)) {
    setAxisState(rs.armStates.at(*arm));
  }
}

void MotorStats::setAxisState(const utl::EAxisState state) {
  if (_isFree) {
    _ui->axisStateLabel->setText("FREE");
    _ui->axisStateLabel->setStyleSheet(
        "QLabel { background-color: #c62828; color: #fff;"
        " border-radius: 4px; padding: 1px 4px; font-weight: bold; }");
    return;
  }
  switch (state) {
    case utl::EAxisState::Locked:
      _ui->axisStateLabel->setText("LOCKED");
      _ui->axisStateLabel->setStyleSheet(
          "QLabel { background-color: #444; color: #ccc;"
          " border-radius: 4px; padding: 1px 4px; font-weight: bold; }");
      break;
    case utl::EAxisState::Slow:
      _ui->axisStateLabel->setText("SLOW");
      _ui->axisStateLabel->setStyleSheet(
          "QLabel { background-color: #e6a817; color: #000;"
          " border-radius: 4px; padding: 1px 4px; font-weight: bold; }");
      break;
    case utl::EAxisState::Fast:
      _ui->axisStateLabel->setText("FAST");
      _ui->axisStateLabel->setStyleSheet(
          "QLabel { background-color: #2e9e4e; color: #fff;"
          " border-radius: 4px; padding: 1px 4px; font-weight: bold; }");
      break;
  }
}

void MotorStats::setFreeStatus(const bool isFree, const bool isControllable) {
  _isFree = isFree;
  _ui->freeButton->setText(_isFree ? "un-FREE" : "FREE");
  _ui->freeButton->setEnabled(isControllable);
}

void MotorStats::handleFreeButtonClicked() {
  if (!_isFree) {
    const auto reply = StyledPopup::askQuestion(
        "Confirmation", "Are you sure you want to free the motor?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No, this);
    if (reply != QMessageBox::Yes) {
      return;
    }
  }

  GuiCommand command;
  command.payload =
      cmd::SetMotorFreeCommand{.motor = _motorId, .enabled = !_isFree};
  emit commandIssued(command);
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
  if (watched == _ui->freeButton) {
    return QWidget::eventFilter(watched, event);
  }
  if (watched && event && event->type() == QEvent::MouseButtonPress) {
    const auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
    if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
      emit clicked(_motorId);
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}
