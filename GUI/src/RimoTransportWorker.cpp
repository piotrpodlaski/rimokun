#include "RimoTransportWorker.hpp"

#include "GuiCommandJsonAdapter.hpp"
#include "Logger.hpp"

namespace {
class ZmqRimoGuiClient final : public IRimoGuiClient {
 public:
  void init() override { _client.init(); }
  std::optional<utl::RobotStatus> receiveRobotStatus() override {
    return _client.receiveRobotStatus();
  }
  std::optional<nlohmann::json> sendCommand(
      const nlohmann::json& command) override {
    return _client.sendCommand(command);
  }

 private:
  utl::RimoClient<utl::RobotStatus> _client;
};
}  // namespace

RimoTransportWorker::RimoTransportWorker(std::unique_ptr<IRimoGuiClient> client)
    : _client(std::move(client)) {
  if (!_client) {
    _client = std::make_unique<ZmqRimoGuiClient>();
  }
}

RimoTransportWorker::~RimoTransportWorker() { stop(); }

void RimoTransportWorker::start(StatusCallback onStatus,
                                ConnectionLostCallback onConnectionLost,
                                ResponseCallback onResponse) {
  if (_running.load(std::memory_order_acquire)) {
    SPDLOG_WARN("Transport worker thread already running.");
    return;
  }
  if (_thread.joinable()) {
    _thread.join();
  }
  _onStatus = std::move(onStatus);
  _onConnectionLost = std::move(onConnectionLost);
  _onResponse = std::move(onResponse);
  _running.store(true, std::memory_order_release);
  _thread = std::thread(&RimoTransportWorker::runner, this);
}

void RimoTransportWorker::stop() {
  _running.store(false, std::memory_order_release);
  if (_thread.joinable()) {
    _thread.join();
  }
}

void RimoTransportWorker::enqueue(Request request) {
  std::lock_guard<std::mutex> lock(_queueMutex);
  _pendingRequests.push(std::move(request));
}

void RimoTransportWorker::runner() {
  _client->init();
  while (_running.load(std::memory_order_acquire)) {
    processPendingCommands();
    auto status = _client->receiveRobotStatus();
    if (!status) {
      if (_onConnectionLost) {
        _onConnectionLost();
      }
      continue;
    }
    if (_onStatus) {
      _onStatus(*status);
    }
    processPendingCommands();
  }
  processPendingCommands();
}

void RimoTransportWorker::processPendingCommands() {
  while (true) {
    Request request;
    {
      std::lock_guard<std::mutex> lock(_queueMutex);
      if (_pendingRequests.empty()) {
        return;
      }
      request = std::move(_pendingRequests.front());
      _pendingRequests.pop();
    }

    const auto jsonCommand = GuiCommandJsonAdapter::toJson(request.command);
    const auto rawResult = _client->sendCommand(jsonCommand);
    ResponseEvent responseEvent;
    responseEvent.id = request.id;
    if (rawResult) {
      responseEvent.response = GuiCommandJsonAdapter::fromJson(*rawResult);
    } else {
      responseEvent.response = std::nullopt;
    }
    if (_onResponse) {
      _onResponse(responseEvent);
    }
  }
}
