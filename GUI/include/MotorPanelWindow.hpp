#pragma once

#include <QDialog>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "CommonDefinitions.hpp"
#include "GuiCommand.hpp"
#include "ResponseConsumer.hpp"

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QGridLayout;
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
  void setSelectedMotor(utl::EMotor motor);
  bool suppressGlobalErrorPopup() const override { return true; }
  void processResponse(const GuiResponse& response) override;

 signals:
  void commandIssued(const GuiCommand& command);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  enum class PendingRequest {
    None,
    Diagnostics,
    ResetAlarm,
    EnableMotor,
    DisableMotor,
  };

  void rebuildMotorList(const std::vector<utl::EMotor>& motors);
  void updateStateLamp();
  void requestDiagnostics();
  void requestResetAlarm();
  void requestSetEnabled(bool enabled);
  [[nodiscard]] std::optional<utl::EMotor> selectedMotor() const;
  void updateFromDiagnostics(const nlohmann::json& payload);
  void createAssignmentRows(
      QGridLayout* layout, const std::vector<std::string>& channels,
      std::map<std::string, std::pair<QLabel*, LedIndicator*>>& targetRows);
  void updateAssignmentRows(
      const nlohmann::json& assignments,
      std::map<std::string, std::pair<QLabel*, LedIndicator*>>& rows);

  QComboBox* _motorSelector{nullptr};
  LedIndicator* _stateLamp{nullptr};
  LedIndicator* _enabledLamp{nullptr};
  QLabel* _inputRawLabel{nullptr};
  QLabel* _outputRawLabel{nullptr};
  QLabel* _netInputRawLabel{nullptr};
  QLabel* _netOutputRawLabel{nullptr};
  QTextEdit* _alarmText{nullptr};
  QTextEdit* _warningText{nullptr};
  QPushButton* _refreshButton{nullptr};
  QPushButton* _resetAlarmButton{nullptr};
  QPushButton* _enableButton{nullptr};
  QPushButton* _disableButton{nullptr};
  QCheckBox* _autoRefreshCheck{nullptr};
  QTimer* _refreshTimer{nullptr};
  std::map<std::string, std::pair<QLabel*, LedIndicator*>> _inputAssignmentRows;
  std::map<std::string, std::pair<QLabel*, LedIndicator*>> _outputAssignmentRows;
  std::map<std::string, std::pair<QLabel*, LedIndicator*>> _netInputAssignmentRows;
  std::map<std::string, std::pair<QLabel*, LedIndicator*>> _netOutputAssignmentRows;

  std::vector<utl::EMotor> _visibleMotors;
  utl::RobotStatus _lastStatus;
  PendingRequest _pendingRequest{PendingRequest::None};
};
