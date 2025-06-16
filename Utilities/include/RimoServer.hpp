//
// Created by piotrek on 6/10/25.
//

#pragma once

#include <thread>
#include <zmq.hpp>
#include "logger.hpp"

#include "CommonDefinitions.hpp"

namespace utl {

template<typename T>
class RimoServer {
 public:
  RimoServer(){
    _socket = zmq::socket_t(_context, zmq::socket_type::pub);
    _socket.bind("ipc:///tmp/rimo_server");
  }
  ~RimoServer() = default;
  void publish(const T &robot){
    const auto node = YAML::convert<T>::encode(robot);
    const auto yaml_str = YAML::Dump(node);
    zmq::message_t msg(yaml_str);
    _socket.send(msg, zmq::send_flags::none);
    SPDLOG_DEBUG("Published new RobotStatus");
    SPDLOG_TRACE("RobotStatus: \n {}", yaml_str);
  }

 private:
  zmq::context_t _context;
  zmq::socket_t _socket;
};

}  // namespace utl