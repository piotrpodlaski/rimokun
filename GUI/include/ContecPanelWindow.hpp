#pragma once

#include <QDialog>

#include <vector>

#include "GuiCommand.hpp"
#include "ResponseConsumer.hpp"

class QCheckBox;
class QCloseEvent;
class QGridLayout;
class QLabel;
class QPushButton;
class QTimer;
class LedIndicator;

class ContecPanelWindow final : public QDialog, public ResponseConsumer {
  Q_OBJECT

 public:
  explicit ContecPanelWindow(QWidget* parent = nullptr);
  bool suppressGlobalErrorPopup() const override { return true; }
  void processResponse(const GuiResponse& response) override;

 signals:
  void commandIssued(const GuiCommand& command);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  struct ChannelRow {
    QLabel* signalLabel{nullptr};
    LedIndicator* led{nullptr};
  };

  void requestDiagnostics();
  void updateFromPayload(const nlohmann::json& payload);
  static std::vector<ChannelRow> createRows(QWidget* parent, QGridLayout* layout,
                                            int count, int columns);

  LedIndicator* _stateLamp{nullptr};
  QPushButton* _refreshButton{nullptr};
  QCheckBox* _autoRefreshCheck{nullptr};
  QLabel* _errorLabel{nullptr};
  QTimer* _refreshTimer{nullptr};
  std::vector<ChannelRow> _inputRows{};
  std::vector<ChannelRow> _outputRows{};
};
