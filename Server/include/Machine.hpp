#pragma once
#include <Contec.hpp>
#include <map>
#include <atomic>
#include <CommonDefinitions.hpp>
#include <CommandInterface.hpp>
#include <RimoServer.hpp>

typedef std::map<std::string, bool> signalMap_t;

class Machine {
 public:
  Machine();
  ~Machine() = default;

  signalMap_t getInputSignals();
  void setOutputs(signalMap_t signals);
  signalMap_t readInputSignals();
  void init();

 private:
  void handleCommandsThread();
  void processThread();
  Contec _contec;
  std::map<std::string, unsigned int> _inputMapping;
  std::map<std::string, unsigned int> _outputMapping;
  utl::RobotStatus _robotStatus;
  cmd::CommandQueue _commandQueue;
  std::atomic<bool> _isRunning;
  std::chrono::duration<double> _loopSleepTime;
  utl::RimoServer<utl::RobotStatus> _robotServer;
};