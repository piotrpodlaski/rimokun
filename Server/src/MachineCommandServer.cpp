#include "MachineCommandServer.hpp"
#include "MachineCommandProcessor.hpp"

#include <Logger.hpp>

RimoServerCommandChannel::RimoServerCommandChannel(
    utl::RimoServer<utl::RobotStatus>& server)
    : _server(server) {}

std::optional<nlohmann::json> RimoServerCommandChannel::receiveCommand() {
  return _server.receiveCommand();
}

void RimoServerCommandChannel::sendResponse(const nlohmann::json& response) {
  _server.sendResponse(response);
}

MachineCommandServer::MachineCommandServer(utl::RimoServer<utl::RobotStatus>& server)
    : _ownedChannel(std::make_unique<RimoServerCommandChannel>(server)),
      _channel(_ownedChannel.get()) {}

MachineCommandServer::MachineCommandServer(ICommandChannel& channel)
    : _channel(&channel) {}

void MachineCommandServer::runLoop(
    const std::atomic<bool>& running,
    const cmd::DispatchFn& dispatch) const {
  SPDLOG_INFO("Command Server Thread Started!");
  while (running.load(std::memory_order_acquire)) {
    auto command = _channel->receiveCommand();
    if (!command) {
      continue;
    }
    SPDLOG_INFO("Received command: {}", command->dump());
    const auto response = cmd::processCommand(*command, dispatch);
    _channel->sendResponse(response);
  }
  SPDLOG_INFO("Command Server thread finished!");
}
