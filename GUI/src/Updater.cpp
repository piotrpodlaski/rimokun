#include "Updater.hpp"

#include "QMessageBox"
#include "logger.hpp"

using namespace std::string_literals;

Updater::Updater(QObject* parent) : QObject(parent) { client.init(); }

Updater::~Updater() { stopUpdaterThread(); }

void Updater::startUpdaterThread() {
  if (running) {
    SPDLOG_WARN("Updater thread is already running! Doing nothing.");
    return;
  }
  SPDLOG_INFO("Spawning updater thread.");

  // cleanup, just in case
  if (updaterThread.joinable()) {
    updaterThread.join();
  }

  running = true;
  updaterThread = std::thread(&Updater::runner, this);
}

void Updater::sendCommand(const YAML::Node& command) {
  auto result = client.sendCommand(command);
  if (!result) {
    SPDLOG_ERROR("Failed to send command and/or get the response!");
    return;
  }
  if ((*result)["status"].as<std::string>() != "OK") {
    auto msgNode = (*result)["message"];
    auto message = "Server did not provide any messages..."s;
    if (msgNode) {
      message = msgNode.as<std::string>();
    }
    SPDLOG_ERROR("Server returned error status with message: '{}'", message);
    QMessageBox::critical(dynamic_cast<QWidget*>(this), "Error",
                          QString::fromStdString(message));
  }
}

void Updater::runner() {
  while (running) {
    auto status = client.receiveRobotStatus();
    if (!status) {
      continue;
    }
    emit newDataArrived(*status);
  }
}

void Updater::stopUpdaterThread() {
  SPDLOG_INFO("Stopping updater thread");
  running = false;
  if (updaterThread.joinable()) {
    updaterThread.join();
  }
}
