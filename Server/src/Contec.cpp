#include <Config.hpp>
#include <Contec.hpp>
#include <Logger.hpp>

using namespace utl;

Contec::Contec() {
  auto& cfg = Config::instance();
  _ipAddress = cfg.getRequired<std::string>("Contec", "ipAddress");
  _port = cfg.getRequired<unsigned>("Contec", "port");
  _slaveId = cfg.getRequired<unsigned>("Contec", "slaveId");
  _nDI = cfg.getRequired<unsigned>("Contec", "nDI");
  _nDO = cfg.getRequired<unsigned>("Contec", "nDO");
  _responseTimeoutMS =
      cfg.getOptional<unsigned>("Contec", "responseTimeoutMS", 1000u);
}

ModbusClient& Contec::ensureModbusClient() {
  // Already have a client
  if (_modbus) {
    return *_modbus;
  }

  // Create client (factory returns std::expected)
  auto cli_res = ModbusClient::tcp(_ipAddress, static_cast<int>(_port),
                                   static_cast<int>(_slaveId));
  if (!cli_res) {
    auto msg = std::format("Failed to create Modbus TCP client: {}",
                           cli_res.error().message);
    SPDLOG_CRITICAL(msg);
    throw std::runtime_error(msg);
  }

  _modbus.emplace(std::move(*cli_res));

  // Connect
  if (auto res = _modbus->connect(); !res) {
    auto err = res.error();
    _modbus.reset();
    auto msg = std::format("Failed to connect to {}:{} (slave {}): {}",
                           _ipAddress, _port, _slaveId, err.message);
    SPDLOG_CRITICAL(msg);
    throw std::runtime_error(msg);
  }

  // Timeout
  if (auto r = _modbus->set_response_timeout(
          std::chrono::milliseconds{_responseTimeoutMS});
      !r) {
    auto err = r.error();
    _modbus.reset();
    auto msg =
        std::format("Failed to set response timeout: {}", err.message);
    SPDLOG_CRITICAL(msg);
    throw std::runtime_error(msg);
  }

  return *_modbus;
}

Contec::~Contec() { _modbus->close(); }

Contec::bitVector Contec::readInputs() {
  auto& client = ensureModbusClient();
  auto regs = client.read_input_bits(0, _nDI);
  if (!regs) {
    auto msg = std::format("read_input_bits({}, {}) failed: {}", 0, _nDI,
                           regs.error().message);
    SPDLOG_CRITICAL(msg);
    throw std::runtime_error(msg);
  }
  return *regs;
}

Contec::bitVector Contec::readOutputs() {
  auto& client = ensureModbusClient();
  auto regs = client.read_bits(0, _nDO);
  if (!regs) {
    auto msg = std::format("read_bits({}, {}) failed: {}", 0, _nDI,
                           regs.error().message);
    SPDLOG_CRITICAL(msg);
    throw std::runtime_error(msg);
  }
  return *regs;
}

void Contec::setOutputs(const bitVector& outputs) {
  if (outputs.size() != _nDO) {
    auto msg =
        std::format("Invalid number of outputs provided! {} instead of {}",
                    outputs.size(), _nDO);
    SPDLOG_CRITICAL(msg);
    throw std::runtime_error(msg);
  }
  auto& client = ensureModbusClient();
  auto ret = client.write_bits(0, outputs);
  if (!ret) {
    auto msg = std::format("write_bits({}, {}) failed: {}", 0, _nDI,
                           ret.error().message);
    SPDLOG_CRITICAL(msg);
    throw std::runtime_error(msg);
  }
}

void Contec::reset() {
  if (_modbus)
    _modbus->close();
  _modbus=std::nullopt;
}