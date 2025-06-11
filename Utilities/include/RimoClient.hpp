//
// Created by piotrek on 6/10/25.
//

#pragma once

#include <optional>

#include "zmq.hpp"
#include "VMotorStats.hpp"

namespace utl {

class RimoClient {
 public:
  RimoClient() = default;
  ~RimoClient() = default;
  void init();
  std::optional<RobotStatus> receiveRobotStatus();

 private:
  bool m_running = false;
  zmq::context_t _context;
  zmq::socket_t _socket;
};

}  // namespace utl
