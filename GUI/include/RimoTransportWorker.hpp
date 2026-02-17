  #pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "GuiCommand.hpp"
#include "RimoClient.hpp"

class RimoTransportWorker {
 public:
  struct Request {
    std::uint64_t id{0};
    GuiCommand command;
  };

  struct ResponseEvent {
    std::uint64_t id{0};
    std::optional<GuiResponse> response;
  };

  using StatusCallback = std::function<void(const utl::RobotStatus&)>;
  using ConnectionLostCallback = std::function<void()>;
  using ResponseCallback = std::function<void(const ResponseEvent&)>;

  RimoTransportWorker();
  ~RimoTransportWorker();

  void start(StatusCallback onStatus, ConnectionLostCallback onConnectionLost,
             ResponseCallback onResponse);
  void stop();
  void enqueue(Request request);

 private:
  void runner();
  void processPendingCommands();

  std::atomic<bool> _running{false};
  std::mutex _queueMutex;
  std::queue<Request> _pendingRequests;
  std::thread _thread;
  utl::RimoClient<utl::RobotStatus> _client;

  StatusCallback _onStatus;
  ConnectionLostCallback _onConnectionLost;
  ResponseCallback _onResponse;
};
