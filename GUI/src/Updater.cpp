#include "Updater.hpp"

#include "Logger.hpp"
#include "QMessageBox"

using namespace std::string_literals;

Updater::Updater(QObject* parent) : QObject(parent) { _client.init(); }

Updater::~Updater() { stopUpdaterThread(); }

void Updater::startUpdaterThread() {
  if (_running) {
    SPDLOG_WARN("Updater thread is already running! Doing nothing.");
    return;
  }
  SPDLOG_INFO("Spawning updater thread.");

  // cleanup, just in case
  if (_updaterThread.joinable()) {
    _updaterThread.join();
  }

  _running = true;
  _updaterThread = std::thread(&Updater::runner, this);
}

void Updater::sendCommand(const YAML::Node& command) {
  auto result = _client.sendCommand(command);
  if (!result) {
    SPDLOG_ERROR("Failed to send command and/or get the response!");
    return;
  }
  if ((*result)["status"].as<std::string>() != "OK") {
    const auto msgNode = (*result)["message"];
    auto message = msgNode.as<std::string>();
    if (message.empty()) {
      message ="Server did not provide any messages..."s;
    }
    SPDLOG_ERROR("Server returned error status with message: '{}'", message);
    QMessageBox::critical(dynamic_cast<QWidget*>(this), "Error",
                          QString::fromStdString(message));
  }
}

void Updater::runner() {
  while (_running) {
    auto status = _client.receiveRobotStatus();
    if (!status) {
      continue;
    }
    emit newDataArrived(*status);
  }
}

void Updater::stopUpdaterThread() {
  SPDLOG_INFO("Stopping updater thread");
  _running = false;
  if (_updaterThread.joinable()) {
    _updaterThread.join();
  }
}
