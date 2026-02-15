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
}

Updater::~Updater() { stopUpdaterThread(); }

void Updater::startUpdaterThread() {
  if (_running.load(std::memory_order_acquire)) {
    SPDLOG_WARN("Updater thread is already running! Doing nothing.");
    return;
  }
  SPDLOG_INFO("Spawning updater thread.");

  // cleanup, just in case
  if (_updaterThread.joinable()) {
    _updaterThread.join();
  }

  _running.store(true, std::memory_order_release);
  _updaterThread = std::thread(&Updater::runner, this);
}

void Updater::sendCommand(const YAML::Node& command) {
  if (!_running.load(std::memory_order_acquire)) {
    SPDLOG_ERROR("Updater thread is not running; command not sent.");
    StyledPopup::showErrorAsync("Error", "Updater thread is not running.");
    return;
  }

  auto* senderObject = QObject::sender();
  PendingCommand pending;
  pending.command = command;
  pending.sender = senderObject;
  pending.expectsResponse =
      dynamic_cast<ResponseConsumer*>(senderObject) != nullptr;
  if (senderObject) {
    pending.senderClassName = senderObject->metaObject()->className();
  } else {
    pending.senderClassName = "UnknownSender";
  }

  std::lock_guard<std::mutex> lock(_commandMutex);
  _pendingCommands.push(std::move(pending));
}

void Updater::runner() {
  _client.init();
  while (_running.load(std::memory_order_acquire)) {
    processPendingCommands();
    auto status = _client.receiveRobotStatus();
    if (!status) {
      emit serverNotConnected();
      continue;
    }
    emit newDataArrived(*status);
    processPendingCommands();
  }
  processPendingCommands();
}

void Updater::processPendingCommands() {
  while (true) {
    PendingCommand pending;
    {
      std::lock_guard<std::mutex> lock(_commandMutex);
      if (_pendingCommands.empty()) {
        return;
      }
      pending = std::move(_pendingCommands.front());
      _pendingCommands.pop();
    }

    auto result = _client.sendCommand(pending.command);
    if (!result) {
      SPDLOG_ERROR("Failed to send command and/or get the response!");
      QMetaObject::invokeMethod(
          this,
          []() {
            StyledPopup::showErrorAsync(
                "Error", "Failed to send command and/or get response.");
          },
          Qt::QueuedConnection);
      continue;
    }
    if ((*result)["status"].as<std::string>() != "OK") {
      const auto msgNode = (*result)["message"];
      auto message = msgNode.as<std::string>();
      if (message.empty()) {
        message = "Server did not provide any messages..."s;
      }
      SPDLOG_ERROR("Server returned error status with message: '{}'", message);
      QMetaObject::invokeMethod(
          this,
          [message = QString::fromStdString(message)]() {
            StyledPopup::showErrorAsync("Error", message);
          },
          Qt::QueuedConnection);
    }

    const auto response = (*result)["response"];
    auto* senderObject = pending.sender.data();
    if (pending.expectsResponse) {
      if (!response) {
        SPDLOG_CRITICAL(
            "Class {} was expecting a response, but server did not provide!",
            pending.senderClassName.toStdString());
        continue;
      }
      if (!senderObject) {
        SPDLOG_WARN(
            "Sender {} was destroyed before response arrived. Dropping "
            "response.",
            pending.senderClassName.toStdString());
        continue;
      }
      auto responseCopy = YAML::Clone(response);
      QMetaObject::invokeMethod(
          this,
          [senderObject, responseCopy]() mutable {
            auto* consumer = dynamic_cast<ResponseConsumer*>(senderObject);
            if (!consumer) {
              return;
            }
            consumer->processResponse(responseCopy);
          },
          Qt::QueuedConnection);
    } else if (response) {
      SPDLOG_WARN(
          "Class {} received unexpected response payload; ignoring.",
          pending.senderClassName.toStdString());
    }
  }
}

void Updater::stopUpdaterThread() {
  SPDLOG_INFO("Stopping updater thread");
  _running.store(false, std::memory_order_release);
  if (_updaterThread.joinable()) {
    _updaterThread.join();
  }
}
