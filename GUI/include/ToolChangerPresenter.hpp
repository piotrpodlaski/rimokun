#pragma once

#include <QObject>

#include "GuiCommand.hpp"
#include "GuiStateStore.hpp"
#include "ToolChanger.hpp"

class ToolChangerPresenter final : public QObject {
  Q_OBJECT
 public:
  ToolChangerPresenter(ToolChanger* view, utl::EArm arm, GuiStateStore* store,
                       QObject* parent = nullptr);

 signals:
  void commandIssued(const GuiCommand& command);

 private slots:
  void onStatusUpdated(const utl::RobotStatus& status);
  void onConnectionChanged(bool connected);

 private:
  ToolChanger* _view;
  utl::EArm _arm;
};
