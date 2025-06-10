//
// Created by piotrek on 6/10/25.
//

#pragma once

#include <thread>
#include <zmq.hpp>

namespace utl {

class RimoServer {
 public:
  RimoServer()=default;
  ~RimoServer()=default;
  void spawn();

 private:
  void publisherThread();
  zmq::context_t _context;
  std::thread _publisherThread;
};

}  // namespace utl