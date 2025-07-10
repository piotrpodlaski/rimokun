#pragma once

#include <Logger.hpp>
#include <optional>

#include "Config.hpp"
#include "YamlExtensions.hpp"
#include "zmq.hpp"

namespace utl {

template <YAML::YamlConvertible T>
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
    const auto msg_str = message.to_string();
    SPDLOG_DEBUG("Received message from publisher!");
    SPDLOG_TRACE("message:\n{}", msg_str);
    return YAML::Load(msg_str).as<T>();
  }

  [[nodiscard]] std::optional<YAML::Node> sendCommand(
      const YAML::Node& command) {
    auto commandStr = YAML::Dump(command);
    SPDLOG_INFO("Sending command:\n{}", commandStr);

    if (const auto status = _commandSocket.send(zmq::buffer(commandStr));
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
    SPDLOG_INFO("Got response:\n{}", response.to_string());
    return YAML::Load(response.to_string());
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
