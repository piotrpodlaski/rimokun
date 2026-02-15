#include "Updater.hpp"

#include "Logger.hpp"
#include "StyledPopup.hpp"
#include "ResponseConsumer.hpp"

using namespace std::string_literals;

namespace {
void ensureMetaTypesRegistered() {
  static const int robotStatusMetaType =
      qRegisterMetaType<utl::RobotStatus>("utl::RobotStatus");
  (void)robotStatusMetaType;
}
}  // namespace

Updater::Updater(QObject* parent) : QObject(parent) {
  ensureMetaTypesRegistered();
  _client.init();
}

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
      message = "Server did not provide any messages..."s;
    }
    SPDLOG_ERROR("Server returned error status with message: '{}'", message);
    StyledPopup::showErrorAsync("Error", QString::fromStdString(message));
  }
  const auto sender = dynamic_cast<ResponseConsumer*>(QObject::sender());
  const bool expectsResponse = (sender != nullptr);
  const auto response = (*result)["response"];
  if (expectsResponse) {
    if (!response) {
      SPDLOG_CRITICAL(
          "Class {} was expecting a response, but server did not provide! This "
          "should never happen! Note, that sender will be waiting for the "
          "response and it may freeze the GUI!",
          QObject::sender()->metaObject()->className());
      return;
    }
    sender->processResponse(response);
  } else if (response) {
    SPDLOG_CRITICAL(
        "Class {} was NOT expecting a response, but server send none! This "
        "should never happen! Ignoring it and continuing execution",
        QObject::sender()->metaObject()->className());
  }
}

void Updater::runner() {
  while (_running) {
    auto status = _client.receiveRobotStatus();
    if (!status) {
      emit serverNotConnected();
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
