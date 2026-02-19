#include "MotorPanelWindow.hpp"

#include <JsonExtensions.hpp>

#include <algorithm>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QStringList>

#include "LedIndicator.hpp"

namespace {
QString formatCode(const nlohmann::json& node, const char* key) {
  const auto it = node.find(key);
  if (it == node.end() || !it->is_number_unsigned()) {
    return "n/a";
  }
  return QString("0x%1").arg(it->get<unsigned int>(), 2, 16, QLatin1Char('0'));
}
}  // namespace

MotorPanelWindow::MotorPanelWindow(QWidget* parent) : QDialog(parent) {
  setWindowTitle("Motor monitor");
  setWindowModality(Qt::NonModal);
  setWindowFlag(Qt::Tool, true);
  setAttribute(Qt::WA_DeleteOnClose, false);

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(10, 10, 10, 10);
  root->setSpacing(8);

  auto* topRow = new QHBoxLayout();
  auto* motorLabel = new QLabel("Motor:", this);
  _motorSelector = new QComboBox(this);
  _stateLamp = new LedIndicator(this);
  _refreshButton = new QPushButton("Refresh", this);
  _resetAlarmButton = new QPushButton("Reset alarm", this);
  _autoRefreshCheck = new QCheckBox("Auto-refresh", this);
  _autoRefreshCheck->setChecked(false);
  topRow->addWidget(motorLabel);
  topRow->addWidget(_motorSelector, 1);
  topRow->addWidget(new QLabel("State:", this));
  topRow->addWidget(_stateLamp);
  topRow->addWidget(_refreshButton);
  topRow->addWidget(_resetAlarmButton);
  topRow->addWidget(_autoRefreshCheck);
  root->addLayout(topRow);

  auto* rawGroup = new QGroupBox("Raw status", this);
  auto* rawForm = new QFormLayout(rawGroup);
  _inputRawLabel = new QLabel("-", rawGroup);
  _outputRawLabel = new QLabel("-", rawGroup);
  rawForm->addRow("Input (0x007D):", _inputRawLabel);
  rawForm->addRow("Output (0x007F):", _outputRawLabel);
  root->addWidget(rawGroup);

  auto* flagsGroup = new QGroupBox("Bit flags", this);
  auto* flagsGrid = new QGridLayout(flagsGroup);
  _inputFlagsText = new QTextEdit(flagsGroup);
  _outputFlagsText = new QTextEdit(flagsGroup);
  _inputFlagsText->setReadOnly(true);
  _outputFlagsText->setReadOnly(true);
  flagsGrid->addWidget(new QLabel("Input flags"), 0, 0);
  flagsGrid->addWidget(new QLabel("Output flags"), 0, 1);
  flagsGrid->addWidget(_inputFlagsText, 1, 0);
  flagsGrid->addWidget(_outputFlagsText, 1, 1);
  root->addWidget(flagsGroup, 1);

  auto* diagGroup = new QGroupBox("Diagnostics", this);
  auto* diagGrid = new QGridLayout(diagGroup);
  _alarmText = new QTextEdit(diagGroup);
  _warningText = new QTextEdit(diagGroup);
  _alarmText->setReadOnly(true);
  _warningText->setReadOnly(true);
  diagGrid->addWidget(new QLabel("Alarm"), 0, 0);
  diagGrid->addWidget(new QLabel("Warning"), 0, 1);
  diagGrid->addWidget(_alarmText, 1, 0);
  diagGrid->addWidget(_warningText, 1, 1);
  root->addWidget(diagGroup, 1);

  _refreshTimer = new QTimer(this);
  _refreshTimer->setInterval(500);
  _refreshTimer->start();

  connect(_refreshButton, &QPushButton::clicked, this,
          [this]() { requestDiagnostics(); });
  connect(_resetAlarmButton, &QPushButton::clicked, this,
          [this]() { requestResetAlarm(); });
  connect(_motorSelector, &QComboBox::currentIndexChanged, this,
          [this](int) { requestDiagnostics(); });
  connect(_refreshTimer, &QTimer::timeout, this, [this]() {
    if (_autoRefreshCheck->isChecked()) {
      requestDiagnostics();
    }
  });
}

void MotorPanelWindow::setRobotStatus(const utl::RobotStatus& status) {
  _lastStatus = status;
  std::vector<utl::EMotor> motors;
  motors.reserve(status.motors.size());
  for (const auto& [motor, _] : status.motors) {
    motors.push_back(motor);
  }
  rebuildMotorList(motors);
  updateStateLamp();
}

void MotorPanelWindow::processResponse(const GuiResponse& response) {
  const auto pending = _pendingRequest;
  _pendingRequest = PendingRequest::None;
  if (!response.ok) {
    const auto message =
        response.message.empty() ? std::string("Unknown motor monitor error")
                                 : response.message;
    _alarmText->setPlainText(QString("Error: %1").arg(
        QString::fromStdString(message)));
    return;
  }
  if (pending == PendingRequest::ResetAlarm) {
    requestDiagnostics();
    return;
  }
  if (!response.payload.has_value()) {
    return;
  }
  updateFromDiagnostics(*response.payload);
}

void MotorPanelWindow::closeEvent(QCloseEvent* event) {
  hide();
  event->ignore();
}

void MotorPanelWindow::rebuildMotorList(const std::vector<utl::EMotor>& motors) {
  if (motors == _visibleMotors) {
    return;
  }
  const auto selected = selectedMotor();
  _visibleMotors = motors;
  _motorSelector->clear();
  for (const auto motor : _visibleMotors) {
    _motorSelector->addItem(
        QString::fromStdString(utl::enumToString(motor)),
        static_cast<int>(motor));
  }
  if (_visibleMotors.empty()) {
    _motorSelector->setEnabled(false);
    _refreshButton->setEnabled(false);
    _resetAlarmButton->setEnabled(false);
    return;
  }
  _motorSelector->setEnabled(true);
  _refreshButton->setEnabled(true);
  int restoreIndex = 0;
  if (selected.has_value()) {
    const auto it =
        std::find(_visibleMotors.begin(), _visibleMotors.end(), *selected);
    if (it != _visibleMotors.end()) {
      restoreIndex = static_cast<int>(std::distance(_visibleMotors.begin(), it));
    }
  }
  _motorSelector->setCurrentIndex(restoreIndex);
}

void MotorPanelWindow::updateStateLamp() {
  const auto motor = selectedMotor();
  if (!motor.has_value() || !_lastStatus.motors.contains(*motor)) {
    _stateLamp->setState(utl::ELEDState::Off);
    _resetAlarmButton->setEnabled(false);
    return;
  }
  const auto& status = _lastStatus.motors.at(*motor);
  _stateLamp->setState(status.state);
  _resetAlarmButton->setEnabled(status.state == utl::ELEDState::Error);
}

void MotorPanelWindow::requestDiagnostics() {
  const auto motor = selectedMotor();
  if (!motor.has_value()) {
    return;
  }
  GuiCommand command;
  command.payload = GuiMotorDiagnosticsCommand{.motor = *motor};
  _pendingRequest = PendingRequest::Diagnostics;
  emit commandIssued(command);
}

void MotorPanelWindow::requestResetAlarm() {
  const auto motor = selectedMotor();
  if (!motor.has_value()) {
    return;
  }
  GuiCommand command;
  command.payload = GuiResetMotorAlarmCommand{.motor = *motor};
  _pendingRequest = PendingRequest::ResetAlarm;
  emit commandIssued(command);
}

std::optional<utl::EMotor> MotorPanelWindow::selectedMotor() const {
  if (!_motorSelector || _motorSelector->currentIndex() < 0) {
    return std::nullopt;
  }
  const auto value = _motorSelector->currentData().toInt();
  return static_cast<utl::EMotor>(value);
}

void MotorPanelWindow::updateFromDiagnostics(const nlohmann::json& payload) {
  if (!payload.is_object()) {
    return;
  }
  if (payload.contains("inputRaw") && payload["inputRaw"].is_number_unsigned()) {
    _inputRawLabel->setText(
        QString("0x%1").arg(payload["inputRaw"].get<unsigned int>(), 4, 16,
                            QLatin1Char('0')));
  } else {
    _inputRawLabel->setText("-");
  }
  if (payload.contains("outputRaw") && payload["outputRaw"].is_number_unsigned()) {
    _outputRawLabel->setText(
        QString("0x%1").arg(payload["outputRaw"].get<unsigned int>(), 4, 16,
                            QLatin1Char('0')));
  } else {
    _outputRawLabel->setText("-");
  }

  auto stringifyFlags = [](const nlohmann::json& node) -> QString {
    if (!node.is_array() || node.empty()) {
      return "-";
    }
    QStringList lines;
    for (const auto& item : node) {
      if (item.is_string()) {
        lines.push_back(QString::fromStdString(item.get<std::string>()));
      }
    }
    return lines.join('\n');
  };
  _inputFlagsText->setPlainText(
      payload.contains("inputFlags") ? stringifyFlags(payload["inputFlags"]) : "-");
  _outputFlagsText->setPlainText(
      payload.contains("outputFlags") ? stringifyFlags(payload["outputFlags"])
                                      : "-");

  auto formatDiag = [](const nlohmann::json& diag) -> QString {
    if (!diag.is_object()) {
      return "-";
    }
    QStringList lines;
    lines << QString("Code: %1").arg(formatCode(diag, "code"));
    if (diag.contains("type") && diag["type"].is_string()) {
      lines << QString("Type: %1")
                   .arg(QString::fromStdString(diag["type"].get<std::string>()));
    }
    if (diag.contains("cause") && diag["cause"].is_string()) {
      lines << QString("Cause: %1")
                   .arg(QString::fromStdString(diag["cause"].get<std::string>()));
    }
    if (diag.contains("remedialAction") && diag["remedialAction"].is_string()) {
      lines << QString("Action: %1").arg(
          QString::fromStdString(diag["remedialAction"].get<std::string>()));
    }
    return lines.join('\n');
  };
  _alarmText->setPlainText(
      payload.contains("alarm") ? formatDiag(payload["alarm"]) : "-");
  _warningText->setPlainText(
      payload.contains("warning") ? formatDiag(payload["warning"]) : "-");
}
