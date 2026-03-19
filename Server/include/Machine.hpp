#pragma once
#include <Contec.hpp>
#include <ControlPanel.hpp>
#include <ControlLoopRunner.hpp>
#include <IClock.hpp>
#include <MachineComponent.hpp>
#include <MachineCommandServer.hpp>
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
#include <mutex>
#include <string_view>
#include <CommonDefinitions.hpp>
#include <CommandInterface.hpp>
#include <RimoServer.hpp>
#include <nlohmann/json.hpp>

class Machine {
 public:
  using LoopState = ControlLoopRunner::State;

  Machine();
  explicit Machine(std::shared_ptr<IClock> clock);
  ~Machine() = default;

  void wire();

  std::optional<signal_map_t> readInputSignals();
  void setOutputs(const signal_map_t& signals);
  std::optional<signal_map_t> readOutputSignals();
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
  virtual nlohmann::json handleMotorDiagnosticsCommand(
      const cmd::MotorDiagnosticsCommand& c);
  virtual void handleResetMotorAlarmCommand(const cmd::ResetMotorAlarmCommand& c);
  virtual void handleSetMotorEnabledCommand(const cmd::SetMotorEnabledCommand& c);
  virtual void handleSetAllMotorsEnabledCommand(
      const cmd::SetAllMotorsEnabledCommand& c);
  virtual nlohmann::json handleContecDiagnosticsCommand(
      const cmd::ContecDiagnosticsCommand& c);
  virtual void handleEmergencyStopCommand(const cmd::EmergencyStopCommand& c);
 private:
  struct IoSignalCache {
    std::uint64_t cycle{0};
    bool valid{false};
    std::optional<signal_map_t> value;
  };

  void cacheInputSignals(std::optional<signal_map_t> value);
  void cacheOutputSignals(std::optional<signal_map_t> value);

  void initializeComponents();
  std::string reconnectComponent(utl::ERobotComponent component);

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
  bool _statusUpdatesEnabled{true};
  std::shared_ptr<IClock> _clock;
  utl::RimoServer<utl::RobotStatus> _robotServer;
  std::thread _commandServerThread;
  std::thread _processThread;
  std::mutex _lifecycleMutex;
  std::uint64_t _ioCacheCycle{0};
  IoSignalCache _inputSignalsCache;
  IoSignalCache _outputSignalsCache;
  std::unique_ptr<ControlLoopRunner> _loopRunner;
  std::unique_ptr<MachineController> _controller;
  std::unique_ptr<MachineStatusBuilder> _statusBuilder;
  std::unique_ptr<MachineCommandServer> _commandServer;
};
