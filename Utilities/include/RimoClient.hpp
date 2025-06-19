//
// Created by piotrek on 6/10/25.
//

#pragma once

#include <optional>
#include <logger.hpp>
#include "zmq.hpp"
#include "YamlExtensions.hpp"

namespace utl {

template<YAML::YamlConvertible T>
class RimoClient {
 public:
  RimoClient() = default;
  ~RimoClient() = default;
  void init(){
    SPDLOG_INFO("Initializing RimoClient");
    _socket = zmq::socket_t(_context, zmq::socket_type::sub);
    _socket.connect("ipc:///tmp/rimo_server");
    _socket.set(zmq::sockopt::subscribe, "");
    _socket.set(zmq::sockopt::rcvtimeo, 1000);
  }
  std::optional<T> receiveRobotStatus() {
    zmq::message_t message;
    if (const auto status = _socket.recv(message); !status) {
      SPDLOG_WARN("RimoClient message receive timeout. Make sure rimoServer is running!");
      return std::nullopt;
    }
    const auto msg_str = message.to_string();
    SPDLOG_DEBUG("Received message from publisher!");
    SPDLOG_TRACE("message:\n{}", msg_str);
    return YAML::Load(msg_str).as<T>();
  }

 private:
  bool m_running = false;
  zmq::context_t _context;
  zmq::socket_t _socket;
};

}  // namespace utl
