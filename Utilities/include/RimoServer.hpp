//
// Created by piotrek on 6/10/25.
//

#pragma once

#include <thread>
#include <zmq.hpp>
#include "logger.hpp"
#include "YamlExtensions.hpp"

namespace utl {

template<YAML::YamlConvertible T>
class RimoServer {
 public:
  RimoServer(){
    _statusSocket = zmq::socket_t(_context, zmq::socket_type::pub);
    _statusSocket.bind("ipc:///tmp/rimoStatus");

    _commandSocket = zmq::socket_t(_context, zmq::socket_type::rep);
    _commandSocket.bind("ipc:///tmp/rimoCommand");
    _commandSocket.set(zmq::sockopt::rcvtimeo, 1000);
  }
  ~RimoServer() = default;
  void publish(const T &robot){
    const auto node = YAML::convert<T>::encode(robot);
    const auto yaml_str = YAML::Dump(node);
    zmq::message_t msg(yaml_str);
    _statusSocket.send(msg, zmq::send_flags::none);
    SPDLOG_DEBUG("Published new RobotStatus");
    SPDLOG_TRACE("RobotStatus: \n {}", yaml_str);
  }

  std::optional<YAML::Node> receiveCommand() {
    zmq::message_t msg;
    if (const auto status = _commandSocket.recv(msg); !status) {
      SPDLOG_DEBUG("Timeout waiting for message from RimoClient...");
      return std::nullopt;
    }
    return YAML::Load(msg.to_string());
  }

  void sendResponse(const YAML::Node &response) {
    _commandSocket.send(zmq::buffer(YAML::Dump(response)));
  }

 private:
  zmq::context_t _context;
  zmq::socket_t _statusSocket;
  zmq::socket_t _commandSocket;
};

}  // namespace utl