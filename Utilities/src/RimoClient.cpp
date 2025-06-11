//
// Created by piotrek on 6/10/25.
//

#include "RimoClient.hpp"

#include <print>

#include "logger.hpp"

using namespace std::chrono_literals;

namespace utl {

void RimoClient::init() {
  SPDLOG_INFO("Initializing RimoClient");
  _socket = zmq::socket_t(_context, zmq::socket_type::sub);
  _socket.connect("ipc:///tmp/rimo_server");
  _socket.set(zmq::sockopt::subscribe, "");
}

std::optional<RobotStatus> RimoClient::receiveRobotStatus() {
  zmq::message_t message;
  if (!_socket.recv(message)) return std::nullopt;
  const auto msg_str = message.to_string();
  SPDLOG_DEBUG("Received message from publisher!");
  SPDLOG_TRACE("message:\n{}", msg_str);
  return YAML::Load(msg_str).as<RobotStatus>();
}

}  // namespace utl