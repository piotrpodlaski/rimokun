#include <Config.hpp>
#include <Contec.hpp>
#include <Logger.hpp>

using namespace utl;

Contec::Contec() {
  auto& cfg = Config::instance();
  _ipAddress         = cfg.getRequired<std::string>("Contec", "ipAddress");
  _port              = cfg.getRequired<unsigned>("Contec", "port");
  _slaveId           = cfg.getRequired<unsigned>("Contec", "slaveId");
  _responseTimeoutMS = cfg.getOptional<unsigned>("Contec", "responseTimeoutMS", 1000u);
}

Contec::ClientResult Contec::ensureModbusClient() {
  // Already have a client
  if (_modbus) {
    return std::ref(*_modbus);
  }

  // Create TCP client
  auto cli_res = ModbusClient::tcp(_ipAddress, _port, _slaveId);
  if (!cli_res) {
    return std::unexpected(cli_res.error());
  }

  _modbus.emplace(std::move(*cli_res));

  // Connect
  if (auto ok = _modbus->connect(); !ok) {
    auto err = ok.error();
    _modbus.reset();
    return std::unexpected(err);
  }

  _modbus->set_response_timeout(std::chrono::milliseconds{1000});

  return std::ref(*_modbus);
}

Contec::~Contec() {
  _modbus->close();
}