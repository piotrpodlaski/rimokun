#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include "MachineCommandProcessor.hpp"
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
  MachineCommandServer(MachineCommandProcessor& processor,
                       utl::RimoServer<utl::RobotStatus>& server);
  MachineCommandServer(MachineCommandProcessor& processor, ICommandChannel& channel);

  void runLoop(const std::atomic<bool>& running,
               const MachineCommandProcessor::DispatchFn& dispatch) const;

 private:
  MachineCommandProcessor& _processor;
  std::unique_ptr<ICommandChannel> _ownedChannel;
  ICommandChannel* _channel{nullptr};
};
