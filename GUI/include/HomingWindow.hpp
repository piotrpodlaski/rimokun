#pragma once

#include <QDialog>
#include <QHash>
#include <QString>
#include <array>

#include "GuiCommand.hpp"

class GuiStateStore;
class QGridLayout;
class QLabel;
class QTimer;
class QVBoxLayout;

class HomingWindow final : public QDialog {
  Q_OBJECT
 public:
  explicit HomingWindow(GuiStateStore* store, QWidget* parent = nullptr);

 public slots:
  void handleStatusUpdated(const utl::RobotStatus& status);
  void handleConnectionChanged(bool connected);

 signals:
  void commandIssued(const GuiCommand& command) const;

 private:
  struct AxisRowWidgets {
    QLabel* axisLabel{nullptr};
    QLabel* stateLabel{nullptr};
  };

  bool confirmHomeAll() const;
  void ensureAxisRows(const utl::HomingSessionStatus& session);
  void updateSummary(const utl::HomingSessionStatus& session);
  void updateAxisRow(const utl::HomingAxisStatus& axisStatus);
  void setSpinnerForState(utl::EHomingState state);
  void refreshAnimation();
  static QString stateChipStyle(utl::EHomingState state);
  static QString summaryStyle(utl::EHomingState state);
  static QString axisDisplayName(utl::EHomingAxis axis);
  static QString stateText(const utl::HomingAxisStatus& axisStatus);
  void emitHomingCommand(cmd::HomingTarget target) const;

  GuiStateStore* _store;
  QLabel* _summaryLabel{nullptr};
  QLabel* _spinnerLabel{nullptr};
  QLabel* _detailLabel{nullptr};
  QGridLayout* _statusGrid{nullptr};
  QTimer* _animationTimer{nullptr};
  int _spinnerIndex{0};
  QHash<int, AxisRowWidgets> _axisRows;
};
