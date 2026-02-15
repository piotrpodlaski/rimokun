#pragma once
#include <Contec.hpp>
#include <ControlPanel.hpp>
#include <MachineComponent.hpp>
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

  std::optional<signalMap_t> readInputSignals();
  void setOutputs(signalMap_t signals);
  std::optional<signalMap_t> readOutputSignals();
  void initialize();
  void shutdown();

 private:
  void commandServerThread();
  void processThread();
  void controlLoopTasks();
  void handleToolChangerCommand(const cmd::ToolChangerCommand& c);
  void handleReconnectCommand(const cmd::ReconnectCommand& c);
  MachineComponent* getComponent(utl::ERobotComponent component);
  static utl::ELEDState stateToLed(MachineComponent::State state);
  void makeDummyStatus();
  void updateStatus();
  Contec _contec;
  ControlPanel _controlPanel;
  std::map<utl::ERobotComponent, MachineComponent*> _components;
  std::map<std::string, unsigned int> _inputMapping;
  std::map<std::string, unsigned int> _outputMapping;
  utl::RobotStatus _robotStatus;
  cmd::CommandQueue _commandQueue;
  std::atomic<bool> _isRunning;
  std::chrono::duration<double> _loopSleepTime;
  utl::RimoServer<utl::RobotStatus> _robotServer;
  std::thread _commandServerThread;
  std::thread _processThread;
};
