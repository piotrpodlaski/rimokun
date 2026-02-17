#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "CommonDefinitions.hpp"
#include "GuiCommand.hpp"

class ITransportWorker {
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

  virtual ~ITransportWorker() = default;
  virtual void start(StatusCallback onStatus, ConnectionLostCallback onConnectionLost,
                     ResponseCallback onResponse) = 0;
  virtual void stop() = 0;
  virtual void enqueue(Request request) = 0;
};
