#include "MotorStatsPresenter.hpp"

MotorStatsPresenter::MotorStatsPresenter(MotorStats* view, const utl::EMotor motorId,
                                         GuiStateStore* store, QObject* parent)
    : QObject(parent), _view(view), _motorId(motorId) {
  _view->setMotorId(_motorId);
  connect(store, &GuiStateStore::statusUpdated, this,
          &MotorStatsPresenter::onStatusUpdated);
  connect(store, &GuiStateStore::connectionChanged, this,
          &MotorStatsPresenter::onConnectionChanged);
}

void MotorStatsPresenter::onStatusUpdated(const utl::RobotStatus& status) {
  _view->handleUpdate(status);
}

void MotorStatsPresenter::onConnectionChanged(const bool connected) {
  if (connected) {
    return;
  }
  _view->setBrake(utl::ELEDState::Off);
  _view->setEnabled(utl::ELEDState::Off);
}
