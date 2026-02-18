#pragma once

#include <Logger.hpp>
#include <optional>

#include "Config.hpp"
#include "JsonExtensions.hpp"
#include "zmq.hpp"

namespace utl {

template <typename T>
class RimoClient {
 public:
  RimoClient() = default;
  ~RimoClient() = default;
  void init() {
    SPDLOG_INFO("Initializing RimoClient");

    const YAML::Node configNode =
        Config::instance().getClassConfig("RimoClient");
    if (configNode.IsDefined()) {
      _statusAddress = configNode["statusAddress"].as<std::string>();
      _commandAddress = configNode["commandAddress"].as<std::string>();
      SPDLOG_INFO("Found entry for RimoClient in the config file");
    }

    SPDLOG_INFO("Starting status subscriver at '{}'", _statusAddress);
    _statusSocket = zmq::socket_t(_context, zmq::socket_type::sub);
    _statusSocket.connect(_statusAddress);
    _statusSocket.set(zmq::sockopt::subscribe, "");
    _statusSocket.set(zmq::sockopt::rcvtimeo, 1000);

    SPDLOG_INFO("Starting command client at '{}'", _commandAddress);
    _commandSocket = zmq::socket_t(_context, zmq::socket_type::req);
    _commandSocket.set(zmq::sockopt::sndtimeo, 1000);
    _commandSocket.set(zmq::sockopt::rcvtimeo, 1000);
    _commandSocket.set(zmq::sockopt::linger, 0);
    _commandSocket.connect(_commandAddress);
  }
  std::optional<T> receiveRobotStatus() {
    zmq::message_t message;
    if (const auto status = _statusSocket.recv(message); !status) {
      SPDLOG_WARN(
          "RimoClient message receive timeout. Make sure rimoServer is "
          "running!");
      return std::nullopt;
    }
    try {
      const auto json = nlohmann::json::from_msgpack(
          static_cast<const std::uint8_t*>(message.data()),
          static_cast<const std::uint8_t*>(message.data()) + message.size());
      return json.get<T>();
    } catch (const std::exception& e) {
      SPDLOG_WARN("Failed to decode status message: {}", e.what());
      return std::nullopt;
    }
  }

  [[nodiscard]] std::optional<YAML::Node> sendCommand(
      const YAML::Node& command) {
    const auto commandJson = yamlNodeToJson(command);
    const auto commandPayload = nlohmann::json::to_msgpack(commandJson);

    if (const auto status = _commandSocket.send(zmq::buffer(commandPayload));
        !status) {
      SPDLOG_ERROR("Communication with command server impossible!");
      return std::nullopt;
    }
    zmq::message_t response;
    if (const auto status = _commandSocket.recv(response); !status) {
      SPDLOG_ERROR(
          "Communication with command server impossible! Killing and "
          "respawning the socket!");
      _commandSocket.close();
      _commandSocket = zmq::socket_t(_context, zmq::socket_type::req);
      _commandSocket.set(zmq::sockopt::rcvtimeo, 1000);
      _commandSocket.set(zmq::sockopt::sndtimeo, 1000);
      _commandSocket.set(zmq::sockopt::linger, 0);
      _commandSocket.connect(_commandAddress);
      return std::nullopt;
    }
    try {
      const auto json = nlohmann::json::from_msgpack(
          static_cast<const std::uint8_t*>(response.data()),
          static_cast<const std::uint8_t*>(response.data()) + response.size());
      return jsonToYamlNode(json);
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Failed to decode command response: {}", e.what());
      return std::nullopt;
    }
  }

 private:
  bool m_running = false;
  zmq::context_t _context;
  zmq::socket_t _statusSocket;
  zmq::socket_t _commandSocket;
  std::string _statusAddress = "ipc:///tmp/rimoStatus";
  std::string _commandAddress = "ipc:///tmp/rimoCommand";
};

}  // namespace utl
