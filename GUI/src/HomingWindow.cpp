#include "HomingWindow.hpp"

#include <GuiStateStore.hpp>

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QTimer>
#include <QVBoxLayout>

namespace {
struct AxisButtonSpec {
  const char* text;
  cmd::HomingTarget target;
  int row;
  int column;
  int rowSpan{1};
  int columnSpan{1};
  bool needsConfirmation{false};
};

struct HomingVisualState {
  const char* foreground;
  const char* spinnerText;
  const char* summaryText;
};

constexpr std::array kSpinnerFrames{"|", "/", "-", "\\"};

constexpr std::array kAxisButtons{
    AxisButtonSpec{"Home all", cmd::HomingTarget::All, 0, 0, 1, 2, true},
    AxisButtonSpec{"Home X left", cmd::HomingTarget::XLeft, 1, 0},
    AxisButtonSpec{"Home Y left", cmd::HomingTarget::YLeft, 2, 0},
    AxisButtonSpec{"Home X right", cmd::HomingTarget::XRight, 1, 1},
    AxisButtonSpec{"Home Y right", cmd::HomingTarget::YRight, 2, 1},
    AxisButtonSpec{"Home Z axis", cmd::HomingTarget::Z, 3, 0, 1, 2},
};

constexpr const char* kHeaderStyle = "font-weight: 700; font-size: 17px;";
constexpr const char* kTableHeaderStyle = "font-weight: 800; font-size: 13px;";
constexpr const char* kAxisLabelStyle = "font-weight: 600;";
constexpr const char* kSpinnerStyle = "font-weight: 700;";

QString makeSummaryStyle(const char* color) {
  return QString("font-weight: 700; font-size: 16px; color: %1;").arg(color);
}

QString makeChipStyle(const HomingVisualState& state) {
  return QString("font-weight: 600; padding: 2px 0; color: %1;")
      .arg(state.foreground);
}

QPushButton* makeHomingButton(const AxisButtonSpec& spec, QWidget* parent) {
  auto* button = new QPushButton(spec.text, parent);
  button->setMinimumHeight(42);
  return button;
}

QLabel* makeTableHeader(const char* text, QWidget* parent) {
  auto* label = new QLabel(text, parent);
  label->setStyleSheet(kTableHeaderStyle);
  label->setAlignment(Qt::AlignCenter);
  return label;
}

QLabel* makeAxisLabel(QWidget* parent) {
  auto* label = new QLabel(parent);
  label->setStyleSheet(kAxisLabelStyle);
  return label;
}

QLabel* makeStateLabel(QWidget* parent) {
  auto* label = new QLabel(parent);
  label->setWordWrap(true);
  label->setMinimumHeight(28);
  label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  return label;
}

HomingVisualState visualState(const utl::EHomingState state) {
  switch (state) {
    case utl::EHomingState::Idle:
      return {.foreground = "#5b534b",
              .spinnerText = "-",
              .summaryText = "Idle"};
    case utl::EHomingState::Pending:
      return {.foreground = "#c27a00",
              .spinnerText = "-",
              .summaryText = "Preparing"};
    case utl::EHomingState::Running:
      return {.foreground = "#0057d8",
              .spinnerText = "|",
              .summaryText = "Homing in progress"};
    case utl::EHomingState::Succeeded:
      return {.foreground = "#0f8a2b",
              .spinnerText = "OK",
              .summaryText = "Homing complete"};
    case utl::EHomingState::Failed:
      return {.foreground = "#d11f1f",
              .spinnerText = "ERR",
              .summaryText = "Homing failed"};
    case utl::EHomingState::TimedOut:
      return {.foreground = "#d46a00",
              .spinnerText = "TO",
              .summaryText = "Homing timed out"};
  }
  return {};
}

QString detailText(const utl::HomingSessionStatus& session) {
  if (session.state == utl::EHomingState::Idle) {
    return "No homing session started.";
  }
  return QString::fromStdString(session.message);
}
}  // namespace

HomingWindow::HomingWindow(GuiStateStore* store, QWidget* parent)
    : QDialog(parent), _store(store) {
  setWindowTitle("Homing");
  setModal(false);
  setMinimumSize(560, 700);
  resize(560, 700);

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(18, 18, 18, 18);
  root->setSpacing(12);

  auto* buttonsGrid = new QGridLayout();
  buttonsGrid->setSpacing(8);
  root->addLayout(buttonsGrid);

  for (const auto& spec : kAxisButtons) {
    auto* button = makeHomingButton(spec, this);
    buttonsGrid->addWidget(button, spec.row, spec.column, spec.rowSpan,
                           spec.columnSpan);
    connect(button, &QPushButton::clicked, this, [this, spec] {
      if (!spec.needsConfirmation || confirmHomeAll()) {
        emitHomingCommand(spec.target);
      }
    });
  }

  auto* statusBox = new QVBoxLayout();
  statusBox->setSpacing(10);
  root->addLayout(statusBox);

  auto* statusHeader = new QLabel("Homing status", this);
  statusHeader->setStyleSheet(kHeaderStyle);
  statusBox->addWidget(statusHeader);

  auto* summaryRow = new QHBoxLayout();
  summaryRow->setSpacing(8);
  _spinnerLabel = new QLabel("-", this);
  _spinnerLabel->setMinimumWidth(22);
  _spinnerLabel->setStyleSheet(kSpinnerStyle);
  _summaryLabel = new QLabel("Idle", this);
  summaryRow->addWidget(_spinnerLabel);
  summaryRow->addWidget(_summaryLabel, 1);
  statusBox->addLayout(summaryRow);

  _detailLabel = new QLabel("No homing session started.", this);
  _detailLabel->setWordWrap(true);
  statusBox->addWidget(_detailLabel);

  auto* tableCard = new QFrame(this);
  auto* tableLayout = new QVBoxLayout(tableCard);
  tableLayout->setContentsMargins(12, 12, 12, 12);
  tableLayout->setSpacing(10);

  auto* headerGrid = new QGridLayout();
  headerGrid->setColumnStretch(0, 1);
  headerGrid->setColumnStretch(1, 1);
  headerGrid->addWidget(makeTableHeader("Axis", tableCard), 0, 0);
  headerGrid->addWidget(makeTableHeader("Status", tableCard), 0, 1);
  tableLayout->addLayout(headerGrid);

  _statusGrid = new QGridLayout();
  _statusGrid->setHorizontalSpacing(14);
  _statusGrid->setVerticalSpacing(8);
  _statusGrid->setColumnStretch(0, 1);
  _statusGrid->setColumnStretch(1, 1);
  tableLayout->addLayout(_statusGrid);
  statusBox->addWidget(tableCard);
  statusBox->addStretch(1);

  auto* closeButton = new QPushButton("Close", this);
  closeButton->setMinimumHeight(42);
  connect(closeButton, &QPushButton::clicked, this, &HomingWindow::close);
  root->addWidget(closeButton);

  _animationTimer = new QTimer(this);
  _animationTimer->setInterval(120);
  connect(_animationTimer, &QTimer::timeout, this, &HomingWindow::refreshAnimation);

  if (_store) {
    connect(_store, &GuiStateStore::statusUpdated, this,
            &HomingWindow::handleStatusUpdated);
    connect(_store, &GuiStateStore::connectionChanged, this,
            &HomingWindow::handleConnectionChanged);
    if (const auto latest = _store->latestStatus(); latest.has_value()) {
      handleStatusUpdated(*latest);
    }
  }
}

bool HomingWindow::confirmHomeAll() const {
  return QMessageBox::warning(
             const_cast<HomingWindow*>(this), "Confirm Home All",
             "Home all will move every axis.\n\n"
             "Make sure the workspace is clear before continuing.",
             QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) ==
         QMessageBox::Yes;
}

void HomingWindow::handleStatusUpdated(const utl::RobotStatus& status) {
  ensureAxisRows(status.homing);
  updateSummary(status.homing);
  for (const auto& axisStatus : status.homing.axes) {
    updateAxisRow(axisStatus);
  }
  setSpinnerForState(status.homing.state);
}

void HomingWindow::handleConnectionChanged(const bool connected) {
  if (connected) {
    return;
  }
  _animationTimer->stop();
  const auto failedVisual = visualState(utl::EHomingState::Failed);
  _spinnerLabel->setText("-");
  _summaryLabel->setText("Disconnected");
  _summaryLabel->setStyleSheet(makeSummaryStyle(failedVisual.foreground));
  _detailLabel->setText("Server connection lost.");
}

void HomingWindow::ensureAxisRows(const utl::HomingSessionStatus& session) {
  QSet<int> activeKeys;
  for (const auto& axisStatus : session.axes) {
    const auto key = static_cast<int>(axisStatus.axis);
    activeKeys.insert(key);
    if (_axisRows.contains(key)) {
      continue;
    }

    const auto row = _axisRows.size();
    AxisRowWidgets widgets{.axisLabel = makeAxisLabel(this),
                           .stateLabel = makeStateLabel(this)};
    _statusGrid->addWidget(widgets.axisLabel, row, 0);
    _statusGrid->addWidget(widgets.stateLabel, row, 1);
    _axisRows.insert(key, widgets);
  }

  for (auto it = _axisRows.begin(); it != _axisRows.end(); ++it) {
    const auto visible = activeKeys.contains(it.key());
    it->axisLabel->setVisible(visible);
    it->stateLabel->setVisible(visible);
  }
}

void HomingWindow::updateSummary(const utl::HomingSessionStatus& session) {
  const auto visual = visualState(session.state);
  _summaryLabel->setText(visual.summaryText);
  _summaryLabel->setStyleSheet(makeSummaryStyle(visual.foreground));
  _detailLabel->setText(detailText(session));
}

void HomingWindow::updateAxisRow(const utl::HomingAxisStatus& axisStatus) {
  const auto key = static_cast<int>(axisStatus.axis);
  const auto row = _axisRows.value(key);
  if (!row.axisLabel || !row.stateLabel) {
    return;
  }
  row.axisLabel->setText(axisDisplayName(axisStatus.axis));
  row.stateLabel->setText(stateText(axisStatus));
  row.stateLabel->setStyleSheet(makeChipStyle(visualState(axisStatus.state)));
}

void HomingWindow::setSpinnerForState(const utl::EHomingState state) {
  if (state == utl::EHomingState::Running) {
    if (!_animationTimer->isActive()) {
      _animationTimer->start();
    }
    return;
  }
  _animationTimer->stop();
  _spinnerLabel->setText(visualState(state).spinnerText);
}

void HomingWindow::refreshAnimation() {
  _spinnerIndex = (_spinnerIndex + 1) % static_cast<int>(kSpinnerFrames.size());
  _spinnerLabel->setText(kSpinnerFrames[static_cast<std::size_t>(_spinnerIndex)]);
}

QString HomingWindow::stateChipStyle(const utl::EHomingState state) {
  return makeChipStyle(visualState(state));
}

QString HomingWindow::summaryStyle(const utl::EHomingState state) {
  return makeSummaryStyle(visualState(state).foreground);
}

QString HomingWindow::axisDisplayName(const utl::EHomingAxis axis) {
  switch (axis) {
    case utl::EHomingAxis::XLeft: return "X left";
    case utl::EHomingAxis::YLeft: return "Y left";
    case utl::EHomingAxis::XRight: return "X right";
    case utl::EHomingAxis::YRight: return "Y right";
    case utl::EHomingAxis::Z: return "Z axis";
  }
  return "Unknown";
}

QString HomingWindow::stateText(const utl::HomingAxisStatus& axisStatus) {
  QString state = visualState(axisStatus.state).summaryText;
  if (!axisStatus.message.empty() &&
      axisStatus.state != utl::EHomingState::Running &&
      axisStatus.state != utl::EHomingState::Pending &&
      axisStatus.state != utl::EHomingState::Idle) {
    state += QString(" (%1)").arg(QString::fromStdString(axisStatus.message));
  }
  return state;
}

void HomingWindow::emitHomingCommand(const cmd::HomingTarget target) const {
  GuiCommand command;
  command.payload = cmd::HomingCommand{.target = target};
  emit commandIssued(command);
}
