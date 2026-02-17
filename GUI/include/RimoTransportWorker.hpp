#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "ITransportWorker.hpp"
#include "GuiCommand.hpp"
#include "RimoClient.hpp"
#include "yaml-cpp/yaml.h"

class IRimoGuiClient {
 public:
  virtual ~IRimoGuiClient() = default;
  virtual void init() = 0;
  virtual std::optional<utl::RobotStatus> receiveRobotStatus() = 0;
  virtual std::optional<YAML::Node> sendCommand(const YAML::Node& command) = 0;
};

class RimoTransportWorker final : public ITransportWorker {
 public:
  explicit RimoTransportWorker(std::unique_ptr<IRimoGuiClient> client = nullptr);
  ~RimoTransportWorker();

  void start(StatusCallback onStatus, ConnectionLostCallback onConnectionLost,
             ResponseCallback onResponse) override;
  void stop() override;
  void enqueue(Request request) override;

 private:
  void runner();
  void processPendingCommands();

  std::atomic<bool> _running{false};
  std::mutex _queueMutex;
  std::queue<Request> _pendingRequests;
  std::thread _thread;
  std::unique_ptr<IRimoGuiClient> _client;

  StatusCallback _onStatus;
  ConnectionLostCallback _onConnectionLost;
  ResponseCallback _onResponse;
};
