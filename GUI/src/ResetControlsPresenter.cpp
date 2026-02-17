#include "ResetControlsPresenter.hpp"

#include "RobotStatusViewModel.hpp"

#include <optional>

ResetControlsPresenter::ResetControlsPresenter(ResetControls* view,
                                               GuiStateStore* store,
                                               QObject* parent)
    : QObject(parent), _view(view) {
  connect(_view, &ResetControls::buttonPressed, this,
          &ResetControlsPresenter::commandIssued);
  connect(store, &GuiStateStore::statusUpdated, this,
          &ResetControlsPresenter::onStatusUpdated);
  connect(store, &GuiStateStore::connectionChanged, this,
          &ResetControlsPresenter::onConnectionChanged);
}

void ResetControlsPresenter::onStatusUpdated(const utl::RobotStatus& status) {
  const auto vm =
      RobotStatusViewModel::resetControlsForStatus(std::make_optional(status));
  _view->applyViewModel(vm);
}

void ResetControlsPresenter::onConnectionChanged(const bool connected) {
  if (connected) {
    return;
  }
  const auto vm = RobotStatusViewModel::resetControlsForStatus(std::nullopt);
  _view->applyViewModel(vm);
}
