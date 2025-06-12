#include "Updater.hpp"

#include "logger.hpp"

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
