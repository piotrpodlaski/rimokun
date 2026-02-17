#include "ToolChangerPresenter.hpp"

#include "RobotStatusViewModel.hpp"

ToolChangerPresenter::ToolChangerPresenter(ToolChanger* view, const utl::EArm arm,
                                           GuiStateStore* store, QObject* parent)
    : QObject(parent), _view(view), _arm(arm) {
  _view->setArm(_arm);
  connect(_view, &ToolChanger::buttonPressed, this,
          &ToolChangerPresenter::commandIssued);
  connect(store, &GuiStateStore::statusUpdated, this,
          &ToolChangerPresenter::onStatusUpdated);
  connect(store, &GuiStateStore::connectionChanged, this,
          &ToolChangerPresenter::onConnectionChanged);
}

void ToolChangerPresenter::onStatusUpdated(const utl::RobotStatus& status) {
  const auto vm = RobotStatusViewModel::toolChangerForArm(status, _arm);
  if (!vm.has_value()) {
    return;
  }
  _view->applyViewModel(*vm);
}

void ToolChangerPresenter::onConnectionChanged(const bool connected) {
  if (connected) {
    return;
  }
  ToolChangerViewModel vm;
  vm.prox = utl::ELEDState::Off;
  vm.openSensor = utl::ELEDState::Off;
  vm.closedSensor = utl::ELEDState::Off;
  vm.openValve = utl::ELEDState::Off;
  vm.closedValve = utl::ELEDState::Off;
  vm.openButtonEnabled = false;
  vm.closeButtonEnabled = false;
  _view->applyViewModel(vm);
}
