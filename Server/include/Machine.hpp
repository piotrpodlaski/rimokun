#pragma once
#include <Contec.hpp>
#include <ControlPanel.hpp>
#include <ControlLoopRunner.hpp>
#include <IClock.hpp>
#include <MachineComponent.hpp>
#include <MachineCommandProcessor.hpp>
#include <MachineCommandServer.hpp>
#include <MachineComponentService.hpp>
#include <MachineController.hpp>
#include <MachineStatusBuilder.hpp>
#include <MotorControl.hpp>
#include <SteadyClockAdapter.hpp>
#include <chrono>
#include <cstddef>
#include <map>
#include <atomic>
#include <future>
#include <memory>
#include <string_view>
#include <CommonDefinitions.hpp>
#include <CommandInterface.hpp>
#include <RimoServer.hpp>

typedef std::map<std::string, bool> signalMap_t;

class Machine {
  friend class MachineRuntime;
 public:
  using LoopState = ControlLoopRunner::State;

  Machine();
  explicit Machine(std::shared_ptr<IClock> clock);
  ~Machine() = default;

  std::optional<signalMap_t> readInputSignals();
  void setOutputs(const signalMap_t& signals);
  std::optional<signalMap_t> readOutputSignals();
  [[nodiscard]] LoopState makeInitialLoopState() const;
  void runOneCycle(LoopState& state);
  bool submitCommand(cmd::Command command);
  std::string dispatchCommandAndWait(cmd::Command command,
                                     std::chrono::milliseconds timeout);
  static void validateMappedIndex(std::string_view signal,
                                  unsigned int index,
                                  std::size_t ioSize,
                                  std::string_view ioKind);
  void initialize();
  void shutdown();

 private:
  void commandServerThread();
  void processThread();
 protected:
  virtual void controlLoopTasks();
  virtual void updateStatus();
  virtual void handleToolChangerCommand(const cmd::ToolChangerCommand& c);
  virtual void handleReconnectCommand(const cmd::ReconnectCommand& c);
 private:
  void makeDummyStatus();

  Contec _contec;
  ControlPanel _controlPanel;
  MotorControl _motorControl;
  std::map<utl::ERobotComponent, MachineComponent*> _components;
  std::map<std::string, unsigned int> _inputMapping;
  std::map<std::string, unsigned int> _outputMapping;
  utl::RobotStatus _robotStatus;
  cmd::CommandQueue _commandQueue;
  std::atomic<bool> _isRunning{false};
  std::chrono::milliseconds _loopInterval{10};
  std::chrono::milliseconds _updateInterval{50};
  std::shared_ptr<IClock> _clock;
  utl::RimoServer<utl::RobotStatus> _robotServer;
  std::thread _commandServerThread;
  std::thread _processThread;
  std::unique_ptr<ControlLoopRunner> _loopRunner;
  std::unique_ptr<MachineController> _controller;
  std::unique_ptr<MachineComponentService> _componentService;
  std::unique_ptr<MachineStatusBuilder> _statusBuilder;
  std::unique_ptr<MachineCommandProcessor> _commandProcessor;
  std::unique_ptr<MachineCommandServer> _commandServer;
};
