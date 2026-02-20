#include "ContecPanelWindow.hpp"

#include <QCheckBox>
#include <QCloseEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "LedIndicator.hpp"

ContecPanelWindow::ContecPanelWindow(QWidget* parent) : QDialog(parent) {
  setWindowTitle("Contec monitor");
  setWindowModality(Qt::NonModal);
  setWindowFlag(Qt::Tool, true);
  setAttribute(Qt::WA_DeleteOnClose, false);

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(10, 10, 10, 10);
  root->setSpacing(8);

  auto* topRow = new QHBoxLayout();
  _stateLamp = new LedIndicator(this);
  _refreshButton = new QPushButton("Refresh", this);
  _autoRefreshCheck = new QCheckBox("Auto-refresh", this);
  _autoRefreshCheck->setChecked(false);
  topRow->addWidget(new QLabel("State:", this));
  topRow->addWidget(_stateLamp);
  topRow->addWidget(_refreshButton);
  topRow->addWidget(_autoRefreshCheck);
  topRow->addStretch(1);
  root->addLayout(topRow);

  auto* mapGroup = new QGroupBox("Mapped I/O", this);
  auto* mapLayout = new QGridLayout(mapGroup);
  auto* inputGroup = new QGroupBox("Inputs", mapGroup);
  auto* outputGroup = new QGroupBox("Outputs", mapGroup);
  auto* inputLayout = new QGridLayout(inputGroup);
  auto* outputLayout = new QGridLayout(outputGroup);
  _inputRows = createRows(this, inputLayout);
  _outputRows = createRows(this, outputLayout);
  mapLayout->addWidget(inputGroup, 0, 0);
  mapLayout->addWidget(outputGroup, 0, 1);
  root->addWidget(mapGroup, 1);

  _errorLabel = new QLabel("-", this);
  root->addWidget(_errorLabel);

  _refreshTimer = new QTimer(this);
  _refreshTimer->setInterval(500);
  _refreshTimer->start();

  connect(_refreshButton, &QPushButton::clicked, this,
          [this]() { requestDiagnostics(); });
  connect(_refreshTimer, &QTimer::timeout, this, [this]() {
    if (_autoRefreshCheck->isChecked()) {
      requestDiagnostics();
    }
  });
}

void ContecPanelWindow::processResponse(const GuiResponse& response) {
  if (!response.ok) {
    updateFromPayload(nlohmann::json::object());
    _stateLamp->setState(utl::ELEDState::Error);
    const auto message =
        response.message.empty() ? std::string("Unknown Contec monitor error")
                                 : response.message;
    _errorLabel->setText(
        QString("Error: %1").arg(QString::fromStdString(message)));
    return;
  }
  if (!response.payload.has_value()) {
    updateFromPayload(nlohmann::json::object());
    _stateLamp->setState(utl::ELEDState::Error);
    return;
  }
  updateFromPayload(*response.payload);
  if (response.payload->contains("diagnosticsError") &&
      (*response.payload)["diagnosticsError"].is_string()) {
    _stateLamp->setState(utl::ELEDState::Error);
    _errorLabel->setText(QString("Error: %1").arg(QString::fromStdString(
        (*response.payload)["diagnosticsError"].get<std::string>())));
  } else {
    _stateLamp->setState(utl::ELEDState::On);
    _errorLabel->setText("-");
  }
}

void ContecPanelWindow::closeEvent(QCloseEvent* event) {
  hide();
  event->ignore();
}

void ContecPanelWindow::requestDiagnostics() {
  GuiCommand command;
  command.payload = GuiContecDiagnosticsCommand{};
  emit commandIssued(command);
}

void ContecPanelWindow::updateFromPayload(const nlohmann::json& payload) {
  for (auto& row : _inputRows) {
    row.signalLabel->setText("-");
    row.led->setState(utl::ELEDState::Off);
  }
  for (auto& row : _outputRows) {
    row.signalLabel->setText("-");
    row.led->setState(utl::ELEDState::Off);
  }

  auto updateRows = [](const nlohmann::json& nodes, std::array<ChannelRow, 8>& rows) {
    if (!nodes.is_array()) {
      return;
    }
    for (std::size_t i = 0; i < rows.size() && i < nodes.size(); ++i) {
      if (!nodes[i].is_object()) {
        continue;
      }
      const auto signal = nodes[i].value("signal", std::string{});
      const auto active = nodes[i].value("active", false);
      rows[i].signalLabel->setText(signal.empty() ? "-" : QString::fromStdString(signal));
      rows[i].led->setState(active ? utl::ELEDState::On : utl::ELEDState::Off);
    }
  };
  updateRows(payload.contains("inputs") ? payload["inputs"] : nlohmann::json::array(),
             _inputRows);
  updateRows(payload.contains("outputs") ? payload["outputs"] : nlohmann::json::array(),
             _outputRows);
}

std::array<ContecPanelWindow::ChannelRow, 8> ContecPanelWindow::createRows(
    QWidget* parent, QGridLayout* layout) {
  std::array<ChannelRow, 8> rows{};
  for (int i = 0; i < 8; ++i) {
    auto* mappingLabel = new QLabel("-", parent);
    auto* led = new LedIndicator(parent);
    led->setState(utl::ELEDState::Off);
    layout->addWidget(mappingLabel, i, 0);
    layout->addWidget(led, i, 1);
    rows[static_cast<std::size_t>(i)] = ChannelRow{
        .signalLabel = mappingLabel,
        .led = led,
    };
  }
  layout->setColumnStretch(0, 1);
  return rows;
}
