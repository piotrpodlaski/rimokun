#include "Updater.hpp"

#include <utility>

#include "Logger.hpp"
#include "RimoTransportWorker.hpp"
#include "ResponseConsumer.hpp"
#include "StyledPopup.hpp"

using namespace std::string_literals;

namespace {
void ensureMetaTypesRegistered() {
  static const int robotStatusMetaType =
      qRegisterMetaType<utl::RobotStatus>("utl::RobotStatus");
  (void)robotStatusMetaType;
}
}  // namespace

Updater::Updater(QObject* parent, std::unique_ptr<ITransportWorker> worker,
                 ErrorNotifier errorNotifier)
    : QObject(parent),
      _worker(std::move(worker)),
      _errorNotifier(std::move(errorNotifier)) {
  ensureMetaTypesRegistered();
  if (!_worker) {
    _worker = std::make_unique<RimoTransportWorker>();
  }
  if (!_errorNotifier) {
    _errorNotifier = [](const QString& title, const QString& message) {
      StyledPopup::showErrorAsync(title, message);
    };
  }
}

Updater::~Updater() { stopUpdaterThread(); }

void Updater::startUpdaterThread() {
  if (_running.load(std::memory_order_acquire)) {
    SPDLOG_WARN("Updater thread is already running! Doing nothing.");
    return;
  }
  SPDLOG_INFO("Spawning updater thread.");

  _running.store(true, std::memory_order_release);
  _worker->start(
      [this](const utl::RobotStatus& status) {
        QMetaObject::invokeMethod(
            this, [this, status]() { emit newDataArrived(status); },
            Qt::QueuedConnection);
      },
      [this]() {
        QMetaObject::invokeMethod(
            this, [this]() { emit serverNotConnected(); }, Qt::QueuedConnection);
      },
      [this](const ITransportWorker::ResponseEvent& event) {
        QMetaObject::invokeMethod(
            this, [this, event]() { handleResponse(event); }, Qt::QueuedConnection);
      });
}

void Updater::sendCommand(const GuiCommand& command) {
  if (!_running.load(std::memory_order_acquire)) {
    SPDLOG_ERROR("Updater thread is not running; command not sent.");
    _errorNotifier("Error", "Updater thread is not running.");
    return;
  }

  auto* senderObject = QObject::sender();
  PendingRequestMeta meta;
  meta.sender = senderObject;
  meta.expectsResponse = dynamic_cast<ResponseConsumer*>(senderObject) != nullptr;
  if (senderObject) {
    meta.senderClassName = senderObject->metaObject()->className();
  } else {
    meta.senderClassName = "UnknownSender";
  }
  const auto requestId = nextRequestId();
  {
    std::lock_guard<std::mutex> lock(_pendingMutex);
    _pendingById.emplace(requestId, std::move(meta));
  }
  _worker->enqueue({.id = requestId, .command = command});
}

void Updater::handleResponse(const ITransportWorker::ResponseEvent& event) {
  PendingRequestMeta meta;
  {
    std::lock_guard<std::mutex> lock(_pendingMutex);
    const auto it = _pendingById.find(event.id);
    if (it == _pendingById.end()) {
      return;
    }
    meta = it->second;
    _pendingById.erase(it);
  }

  if (!event.response.has_value()) {
    SPDLOG_ERROR("Failed to send command and/or get the response!");
    _errorNotifier("Error", "Failed to send command and/or get response.");
    return;
  }

  auto response = *event.response;
  if (!response.ok) {
    if (response.message.empty()) {
      response.message = "Server did not provide any message..."s;
    }
    SPDLOG_ERROR("Server returned error status with message: '{}'", response.message);
    _errorNotifier("Error", QString::fromStdString(response.message));
  }

  auto* senderObject = meta.sender.data();
  if (!meta.expectsResponse) {
    if (response.payload.has_value()) {
      SPDLOG_WARN("Class {} received unexpected response payload; ignoring.",
                  meta.senderClassName.toStdString());
    }
    return;
  }
  if (!senderObject) {
    SPDLOG_WARN("Sender {} was destroyed before response arrived. Dropping response.",
                meta.senderClassName.toStdString());
    return;
  }
  auto* consumer = dynamic_cast<ResponseConsumer*>(senderObject);
  if (!consumer) {
    return;
  }
  consumer->processResponse(response);
}

void Updater::stopUpdaterThread() {
  SPDLOG_INFO("Stopping updater thread");
  _running.store(false, std::memory_order_release);
  _worker->stop();
  std::lock_guard<std::mutex> lock(_pendingMutex);
  _pendingById.clear();
}

std::uint64_t Updater::nextRequestId() {
  auto id = _requestCounter.fetch_add(1, std::memory_order_acq_rel);
  if (id == 0) {
    id = _requestCounter.fetch_add(1, std::memory_order_acq_rel);
  }
  return id;
}
