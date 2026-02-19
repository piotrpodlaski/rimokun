#pragma once

#include <QDialog>

#include <optional>
#include <vector>

#include "CommonDefinitions.hpp"
#include "GuiCommand.hpp"
#include "ResponseConsumer.hpp"

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QPushButton;
class QTextEdit;
class QTimer;
class LedIndicator;

class MotorPanelWindow final : public QDialog, public ResponseConsumer {
  Q_OBJECT

 public:
  explicit MotorPanelWindow(QWidget* parent = nullptr);
  void setRobotStatus(const utl::RobotStatus& status);
  void processResponse(const GuiResponse& response) override;

 signals:
  void commandIssued(const GuiCommand& command);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  enum class PendingRequest { None, Diagnostics, ResetAlarm };

  void rebuildMotorList(const std::vector<utl::EMotor>& motors);
  void updateStateLamp();
  void requestDiagnostics();
  void requestResetAlarm();
  [[nodiscard]] std::optional<utl::EMotor> selectedMotor() const;
  void updateFromDiagnostics(const nlohmann::json& payload);

  QComboBox* _motorSelector{nullptr};
  LedIndicator* _stateLamp{nullptr};
  QLabel* _inputRawLabel{nullptr};
  QLabel* _outputRawLabel{nullptr};
  QTextEdit* _inputFlagsText{nullptr};
  QTextEdit* _outputFlagsText{nullptr};
  QTextEdit* _alarmText{nullptr};
  QTextEdit* _warningText{nullptr};
  QPushButton* _refreshButton{nullptr};
  QPushButton* _resetAlarmButton{nullptr};
  QCheckBox* _autoRefreshCheck{nullptr};
  QTimer* _refreshTimer{nullptr};

  std::vector<utl::EMotor> _visibleMotors;
  utl::RobotStatus _lastStatus;
  PendingRequest _pendingRequest{PendingRequest::None};
};
