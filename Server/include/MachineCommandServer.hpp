#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include "CommandInterface.hpp"
#include "RimoServer.hpp"

class ICommandChannel {
 public:
  virtual ~ICommandChannel() = default;
  virtual std::optional<nlohmann::json> receiveCommand() = 0;
  virtual void sendResponse(const nlohmann::json& response) = 0;
};

class RimoServerCommandChannel final : public ICommandChannel {
 public:
  explicit RimoServerCommandChannel(utl::RimoServer<utl::RobotStatus>& server);

  std::optional<nlohmann::json> receiveCommand() override;
  void sendResponse(const nlohmann::json& response) override;

 private:
  utl::RimoServer<utl::RobotStatus>& _server;
};

class MachineCommandServer {
 public:
  explicit MachineCommandServer(utl::RimoServer<utl::RobotStatus>& server);
  explicit MachineCommandServer(ICommandChannel& channel);

  void runLoop(const std::atomic<bool>& running,
               const cmd::DispatchFn& dispatch) const;

 private:
  std::unique_ptr<ICommandChannel> _ownedChannel;
  ICommandChannel* _channel{nullptr};
};
