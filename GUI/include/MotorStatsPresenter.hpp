#pragma once

#include <QObject>

#include "GuiStateStore.hpp"
#include "MotorStats.hpp"

class MotorStatsPresenter final : public QObject {
  Q_OBJECT
 public:
  MotorStatsPresenter(MotorStats* view, utl::EMotor motorId, GuiStateStore* store,
                      QObject* parent = nullptr);

 private slots:
  void onStatusUpdated(const utl::RobotStatus& status);
  void onConnectionChanged(bool connected);

 private:
  MotorStats* _view;
  utl::EMotor _motorId;
};
