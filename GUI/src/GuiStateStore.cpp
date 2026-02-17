#include "GuiStateStore.hpp"

GuiStateStore::GuiStateStore(QObject* parent) : QObject(parent) {}

std::optional<utl::RobotStatus> GuiStateStore::latestStatus() const {
  return _latestStatus;
}

bool GuiStateStore::isConnected() const { return _connected; }

void GuiStateStore::onStatusReceived(const utl::RobotStatus& status) {
  const bool wasConnected = _connected;
  _connected = true;
  _latestStatus = status;
  if (!wasConnected) {
    emit connectionChanged(true);
  }
  emit statusUpdated(status);
}

void GuiStateStore::onServerDisconnected() {
  if (!_connected) {
    return;
  }
  _connected = false;
  emit connectionChanged(false);
}
