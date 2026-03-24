#include "ContecPanelWindow.hpp"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <Config.hpp>
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
  const auto inputCount = utl::Config::instance().getRequired<unsigned>("Contec", "nDI");
  const auto outputCount = utl::Config::instance().getRequired<unsigned>("Contec", "nDO");
  _inputRows = createRows(this, inputLayout, static_cast<int>(inputCount), 2);
  _outputRows = createRows(this, outputLayout, static_cast<int>(outputCount), 1);
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

  auto updateRows = [](const nlohmann::json& nodes, std::vector<ChannelRow>& rows) {
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

std::vector<ContecPanelWindow::ChannelRow> ContecPanelWindow::createRows(
    QWidget* parent, QGridLayout* layout, const int count, const int columns) {
  std::vector<ChannelRow> rows(static_cast<std::size_t>(count));
  const int safeColumns = std::max(1, columns);
  const int rowsPerColumn = (count + safeColumns - 1) / safeColumns;
  const int totalColumns = safeColumns * 3 - 1;
  for (int i = 0; i < count; ++i) {
    auto* mappingLabel = new QLabel("-", parent);
    auto* led = new LedIndicator(parent);
    led->setState(utl::ELEDState::Off);
    const int columnGroup = i / rowsPerColumn;
    const int row = i % rowsPerColumn;
    const int baseColumn = columnGroup * 3;
    const int displayRow = row * 2;
    layout->addWidget(mappingLabel, displayRow, baseColumn);
    layout->addWidget(led, displayRow, baseColumn + 1);
    rows[static_cast<std::size_t>(i)] = ChannelRow{
        .signalLabel = mappingLabel,
        .led = led,
    };
  }
  for (int column = 0; column < safeColumns - 1; ++column) {
    auto* separator = new QFrame(parent);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setLineWidth(1);
    layout->addWidget(separator, 0, column * 3 + 2, rowsPerColumn * 2 - 1, 1);
  }
  for (int row = 0; row < rowsPerColumn - 1; ++row) {
    auto* separator = new QFrame(parent);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setLineWidth(1);
    layout->addWidget(separator, row * 2 + 1, 0, 1, totalColumns);
  }
  for (int column = 0; column < safeColumns; ++column) {
    layout->setColumnStretch(column * 3, 1);
  }
  return rows;
}
