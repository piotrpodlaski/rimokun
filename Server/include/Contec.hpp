#pragma once

#include <ModbusClient.hpp>

class Contec {
  typedef std::vector<bool> bitVector;
  using ClientResult = std::expected<std::reference_wrapper<ModbusClient>, ModbusError>;
 public:
  Contec();
  ~Contec();
  bitVector readInputs();
  bitVector readOutputs();
  void setOutputs(bitVector outputs);


private:
  ClientResult ensureModbusClient();
  std::optional<ModbusClient> _modbus;   // not initialized at startup
  std::string _ipAddress;
  unsigned int _port;
  unsigned int _slaveId;
  unsigned int _responseTimeoutMS;
};