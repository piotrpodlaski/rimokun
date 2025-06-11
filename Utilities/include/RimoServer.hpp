//
// Created by piotrek on 6/10/25.
//

#pragma once

#include <thread>
#include <zmq.hpp>

#include "CommonDefinitions.hpp"

namespace utl {

class RimoServer {
 public:
  RimoServer();
  ~RimoServer() = default;
  void publish(const RobotStatus &robot);

 private:

  zmq::context_t _context;
  zmq::socket_t _socket;
};

}  // namespace utl