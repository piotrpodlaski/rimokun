#pragma once

#include <libserial/SerialPort.h>

#include <memory>
#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

#include "IControlPanelComm.hpp"

class SerialControlPanelComm final : public IControlPanelComm {
 public:
  explicit SerialControlPanelComm(const YAML::Node& commConfig);
  ~SerialControlPanelComm() override;

  void open() override;
  void closeNoThrow() noexcept override;
  [[nodiscard]] std::optional<std::string> readLine() override;
  [[nodiscard]] std::string describe() const override;

 private:
  static char parseLineTerminator(const std::string& token);

  std::unique_ptr<LibSerial::SerialPort> _serial;
  std::string _port;
  LibSerial::BaudRate _baudRate;
  LibSerial::CharacterSize _characterSize;
  LibSerial::FlowControl _flowControl;
  LibSerial::Parity _parity;
  LibSerial::StopBits _stopBits;
  std::size_t _readTimeoutMS;
  char _lineTerminator;
};
