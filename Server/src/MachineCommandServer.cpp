#include "MachineCommandServer.hpp"

#include <Logger.hpp>

MachineCommandServer::MachineCommandServer(
    MachineCommandProcessor& processor, utl::RimoServer<utl::RobotStatus>& server)
    : _processor(processor), _server(server) {}

void MachineCommandServer::runLoop(
    const std::atomic<bool>& running,
    const MachineCommandProcessor::DispatchFn& dispatch) const {
  SPDLOG_INFO("Command Server Thread Started!");
  while (running.load(std::memory_order_acquire)) {
    auto command = _server.receiveCommand();
    if (!command) {
      continue;
    }
    SPDLOG_INFO("Received command:\n{}", YAML::Dump(*command));
    const auto response = _processor.processCommand(*command, dispatch);
    _server.sendResponse(response);
  }
  SPDLOG_INFO("Command Server thread finished!");
}
