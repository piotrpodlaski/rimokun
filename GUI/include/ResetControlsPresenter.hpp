#pragma once

#include <QObject>

#include "GuiCommand.hpp"
#include "GuiStateStore.hpp"
#include "ResetControls.hpp"

class ResetControlsPresenter final : public QObject {
  Q_OBJECT
 public:
  ResetControlsPresenter(ResetControls* view, GuiStateStore* store,
                         QObject* parent = nullptr);

 signals:
  void commandIssued(const GuiCommand& command);

 private slots:
  void onStatusUpdated(const utl::RobotStatus& status);
  void onConnectionChanged(bool connected);

 private:
  ResetControls* _view;
};
