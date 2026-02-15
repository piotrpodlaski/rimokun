#pragma once

#include <MachineComponent.hpp>
#include <ModbusClient.hpp>

class Contec final : public MachineComponent {
  typedef std::vector<bool> bitVector;
 public:
  Contec();
  ~Contec();
  void initialize() override;
  void reset() override;
  [[nodiscard]] utl::ERobotComponent componentType() const override {
    return utl::ERobotComponent::Contec;
  }
  bitVector readInputs();
  bitVector readOutputs();
  void setOutputs(const bitVector& outputs);
  [[nodiscard]] unsigned int getNOutputs() const {return _nDO;}
  [[nodiscard]] unsigned int getNInputs() const {return _nDI;}


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
