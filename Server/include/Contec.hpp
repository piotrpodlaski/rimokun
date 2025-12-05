#pragma once

#include <ModbusClient.hpp>

class Contec {
  typedef std::vector<bool> bitVector;
 public:
  Contec();
  ~Contec();
  void reset();
  bitVector readInputs();
  bitVector readOutputs();
  void setOutputs(bitVector outputs);
  unsigned int getNOutputs() {return _nDO;}
  unsigned int getNInputs() {return _nDI;}


private:
  ModbusClient& ensureModbusClient();
  std::optional<ModbusClient> _modbus;   // not initialized at startup
  std::string _ipAddress;
  unsigned int _port;
  unsigned int _slaveId;
  unsigned int _responseTimeoutMS;
  unsigned int _nDI;
  unsigned int _nDO;
};