#include "MachineCommandServer.hpp"

#include <Logger.hpp>

RimoServerCommandChannel::RimoServerCommandChannel(
    utl::RimoServer<utl::RobotStatus>& server)
    : _server(server) {}

std::optional<YAML::Node> RimoServerCommandChannel::receiveCommand() {
  return _server.receiveCommand();
}

void RimoServerCommandChannel::sendResponse(const YAML::Node& response) {
  _server.sendResponse(response);
}

MachineCommandServer::MachineCommandServer(
    MachineCommandProcessor& processor, utl::RimoServer<utl::RobotStatus>& server)
    : _processor(processor),
      _ownedChannel(std::make_unique<RimoServerCommandChannel>(server)),
      _channel(_ownedChannel.get()) {}

MachineCommandServer::MachineCommandServer(MachineCommandProcessor& processor,
                                           ICommandChannel& channel)
    : _processor(processor), _channel(&channel) {}

void MachineCommandServer::runLoop(
    const std::atomic<bool>& running,
    const MachineCommandProcessor::DispatchFn& dispatch) const {
  SPDLOG_INFO("Command Server Thread Started!");
  while (running.load(std::memory_order_acquire)) {
    auto command = _channel->receiveCommand();
    if (!command) {
      continue;
    }
    SPDLOG_INFO("Received command:\n{}", YAML::Dump(*command));
    const auto response = _processor.processCommand(*command, dispatch);
    _channel->sendResponse(response);
  }
  SPDLOG_INFO("Command Server thread finished!");
}
