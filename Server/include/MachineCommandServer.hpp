#pragma once

#include <atomic>

#include "MachineCommandProcessor.hpp"
#include "RimoServer.hpp"

class MachineCommandServer {
 public:
  MachineCommandServer(MachineCommandProcessor& processor,
                       utl::RimoServer<utl::RobotStatus>& server);

  void runLoop(const std::atomic<bool>& running,
               const MachineCommandProcessor::DispatchFn& dispatch) const;

 private:
  MachineCommandProcessor& _processor;
  utl::RimoServer<utl::RobotStatus>& _server;
};
