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
    _statusSocket = zmq::socket_t(_context, zmq::socket_type::sub);
    _statusSocket.connect("ipc:///tmp/rimoStatus");
    _statusSocket.set(zmq::sockopt::subscribe, "");
    _statusSocket.set(zmq::sockopt::rcvtimeo, 1000);

    _commandSocket = zmq::socket_t(_context, zmq::socket_type::req);
    _commandSocket.connect("ipc:///tmp/rimoCommand");

    YAML::Node n;
    n["type"] = "robot";
    sendCommand(n);
  }
  std::optional<T> receiveRobotStatus() {
    zmq::message_t message;
    if (const auto status = _statusSocket.recv(message); !status) {
      SPDLOG_WARN("RimoClient message receive timeout. Make sure rimoServer is running!");
      return std::nullopt;
    }
    const auto msg_str = message.to_string();
    SPDLOG_DEBUG("Received message from publisher!");
    SPDLOG_TRACE("message:\n{}", msg_str);
    return YAML::Load(msg_str).as<T>();
  }

  YAML::Node sendCommand(const YAML::Node& command) {
    auto commandStr = YAML::Dump(command);
    SPDLOG_INFO("Sending command:\n{}", commandStr);
    _commandSocket.send(zmq::buffer(commandStr));
    zmq::message_t response;
    _commandSocket.recv(response);
    SPDLOG_INFO("Got response:\n{}", response.to_string());
    return {};
  }

 private:
  bool m_running = false;
  zmq::context_t _context;
  zmq::socket_t _statusSocket;
  zmq::socket_t _commandSocket;
};

}  // namespace utl
