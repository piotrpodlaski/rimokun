#include "SerialControlPanelComm.hpp"

#include <Logger.hpp>
#include <YamlExtensions.hpp>

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <format>
#include <stdexcept>
#include <unistd.h>

SerialControlPanelComm::SerialControlPanelComm(const YAML::Node& commConfig) {
  const auto serialNode = commConfig["serial"];
  if (!serialNode || !serialNode.IsMap()) {
    throw std::runtime_error(
        "ControlPanel comm.type='serial' requires map 'comm.serial'.");
  }

  auto requireNode = [&](const std::string& key) -> YAML::Node {
    const auto node = serialNode[key];
    if (!node || !node.IsDefined()) {
      throw std::runtime_error(
          std::format("Missing required ControlPanel comm.serial.{}", key));
    }
    return node;
  };
  auto optionalNode = [&](const std::string& key) -> YAML::Node {
    return serialNode[key];
  };

  _serial = std::make_unique<LibSerial::SerialPort>();
  _port = requireNode("port").as<std::string>();
  _baudRate = requireNode("baudRate").as<LibSerial::BaudRate>();
  _characterSize = optionalNode("characterSize")
                       ? optionalNode("characterSize")
                             .as<LibSerial::CharacterSize>()
                       : LibSerial::CharacterSize::CHAR_SIZE_8;
  _flowControl = optionalNode("flowControl")
                     ? optionalNode("flowControl").as<LibSerial::FlowControl>()
                     : LibSerial::FlowControl::FLOW_CONTROL_NONE;
  _parity = optionalNode("parity")
                ? optionalNode("parity").as<LibSerial::Parity>()
                : LibSerial::Parity::PARITY_NONE;
  _stopBits = optionalNode("stopBits")
                  ? optionalNode("stopBits").as<LibSerial::StopBits>()
                  : LibSerial::StopBits::STOP_BITS_1;
  _readTimeoutMS =
      std::max<std::size_t>(1u, optionalNode("readTimeoutMS")
                                    ? optionalNode("readTimeoutMS")
                                          .as<std::size_t>()
                                    : 200u);
  _lineTerminator = parseLineTerminator(
      optionalNode("lineTerminator")
          ? optionalNode("lineTerminator").as<std::string>()
          : "\\n");
}

SerialControlPanelComm::~SerialControlPanelComm() { closeNoThrow(); }

void SerialControlPanelComm::open() {
  closeNoThrow();
  _serial->Open(_port);
  _serial->SetBaudRate(_baudRate);
  _serial->SetCharacterSize(_characterSize);
  _serial->SetFlowControl(_flowControl);
  _serial->SetParity(_parity);
  _serial->SetStopBits(_stopBits);
  _serial->FlushIOBuffers();
}

void SerialControlPanelComm::closeNoThrow() noexcept {
  int fd = -1;
  bool closeViaLibSerialFailed = false;
  try {
    fd = _serial->GetFileDescriptor();
  } catch (const std::exception&) {
    fd = -1;
  }

  try {
    if (_serial->IsOpen()) {
      _serial->Close();
    }
  } catch (const std::exception& e) {
    SPDLOG_WARN("ControlPanel serial close via LibSerial failed: {}", e.what());
    closeViaLibSerialFailed = true;
  }

  if (closeViaLibSerialFailed && fd >= 0 && ::fcntl(fd, F_GETFD) != -1) {
    if (::close(fd) == -1 && errno != EBADF) {
      SPDLOG_WARN("ControlPanel forced close(fd={}) failed, errno={}", fd, errno);
    }
  }

  if (closeViaLibSerialFailed) {
    SPDLOG_WARN("ControlPanel keeping poisoned SerialPort instance leaked to avoid "
                "terminate on destructor after close failure.");
    (void)_serial.release();
    _serial = std::make_unique<LibSerial::SerialPort>();
    return;
  }
  _serial = std::make_unique<LibSerial::SerialPort>();
}

std::optional<std::string> SerialControlPanelComm::readLine() {
  try {
    std::string line;
    _serial->ReadLine(line, _lineTerminator, _readTimeoutMS);
    return line;
  } catch (const LibSerial::ReadTimeout&) {
    try {
      if (!_serial->IsOpen()) {
        throw std::runtime_error("ControlPanel serial port is no longer open.");
      }
      (void)_serial->GetNumberOfBytesAvailable();
    } catch (const std::exception& e) {
      throw std::runtime_error(std::format(
          "ControlPanel serial health check failed: {}", e.what()));
    }
    return std::nullopt;
  } catch (const std::exception& e) {
    throw std::runtime_error(
        std::format("ControlPanel serial read failed: {}", e.what()));
  }
}

std::string SerialControlPanelComm::describe() const {
  return std::format("serial({})", _port);
}

char SerialControlPanelComm::parseLineTerminator(const std::string& token) {
  if (token == "\\n") {
    return '\n';
  }
  if (token == "\\r") {
    return '\r';
  }
  if (token == "\\0") {
    return '\0';
  }
  if (token.empty()) {
    return '\n';
  }
  return token.front();
}
