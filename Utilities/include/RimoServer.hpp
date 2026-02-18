#pragma once

#include <thread>
#include <vector>
#include <zmq.hpp>

#include "Config.hpp"
#include "JsonExtensions.hpp"
#include "Logger.hpp"

namespace utl {

template <typename T>
class RimoServer {
 public:
  RimoServer() {
    SPDLOG_INFO("Initializing RimoServer");
    const YAML::Node configNode =
        Config::instance().getClassConfig("RimoServer");
    if (configNode.IsDefined()) {
      _statusAddress = configNode["statusAddress"].as<std::string>();
      _commandAddress = configNode["commandAddress"].as<std::string>();
      SPDLOG_INFO("Found entry for RimoServer in the config file");
    }

    SPDLOG_INFO("Starting status publisher at '{}'", _statusAddress);
    _statusSocket = zmq::socket_t(_context, zmq::socket_type::pub);
    _statusSocket.bind(_statusAddress);

    SPDLOG_INFO("Starting command server at '{}'", _commandAddress);
    _commandSocket = zmq::socket_t(_context, zmq::socket_type::rep);
    _commandSocket.bind(_commandAddress);
    _commandSocket.set(zmq::sockopt::rcvtimeo, 1000);
  }
  ~RimoServer() = default;
  void publish(const T &robot) {
    const auto json = nlohmann::json(robot);
    const auto payload = nlohmann::json::to_msgpack(json);
    _statusSocket.send(zmq::buffer(payload), zmq::send_flags::none);
  }

  std::optional<YAML::Node> receiveCommand() {
    zmq::message_t msg;
    if (const auto status = _commandSocket.recv(msg); !status) {
      SPDLOG_TRACE("Timeout waiting for message from RimoClient...");
      return std::nullopt;
    }
    try {
      const auto json = nlohmann::json::from_msgpack(
          static_cast<const std::uint8_t*>(msg.data()),
          static_cast<const std::uint8_t*>(msg.data()) + msg.size());
      return jsonToYamlNode(json);
    } catch (const std::exception& e) {
      SPDLOG_WARN("Invalid command payload format: {}", e.what());
      return std::nullopt;
    }
  }

  void sendResponse(const YAML::Node &response) {
    try {
      const auto json = yamlNodeToJson(response);
      const auto payload = nlohmann::json::to_msgpack(json);
      _commandSocket.send(zmq::buffer(payload), zmq::send_flags::none);
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Failed to serialize command response: {}", e.what());
      _commandSocket.send(zmq::buffer(std::vector<std::uint8_t>{}),
                          zmq::send_flags::none);
    }
  }

 private:
  zmq::context_t _context;
  zmq::socket_t _statusSocket;
  zmq::socket_t _commandSocket;
  std::string _statusAddress = "ipc:///tmp/rimoStatus";
  std::string _commandAddress = "ipc:///tmp/rimoCommand";
};

}  // namespace utl
