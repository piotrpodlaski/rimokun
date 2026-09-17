#pragma once
#include <AxisHomingManager.hpp>
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
#include <vector>
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
  ~Machine();

  void wire();

  std::optional<utl::SignalMap> readInputSignals();
  void setOutputs(const utl::SignalMap& signals);
  std::optional<utl::SignalMap> readOutputSignals();
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
  void contecThread();
 protected:
  virtual void controlLoopTasks();
  virtual void updateStatus();
  virtual void handleToolChangerCommand(const cmd::ToolChangerCommand& c);
  virtual void handleReconnectCommand(const cmd::ReconnectCommand& c);
  virtual nlohmann::json handleMotorDiagnosticsCommand(
      const cmd::MotorDiagnosticsCommand& c);
  virtual void handleResetMotorAlarmCommand(const cmd::ResetMotorAlarmCommand& c);
  virtual void handleSetMotorEnabledCommand(const cmd::SetMotorEnabledCommand& c);
  virtual void handleSetMotorFreeCommand(const cmd::SetMotorFreeCommand& c);
  virtual void handleSetAllMotorsEnabledCommand(
      const cmd::SetAllMotorsEnabledCommand& c);
  virtual void handleHomingCommand(const cmd::HomingCommand& c);
  virtual void handleCancelHomingCommand(const cmd::CancelHomingCommand& c);
  virtual nlohmann::json handleContecDiagnosticsCommand(
      const cmd::ContecDiagnosticsCommand& c);
  virtual void handleEmergencyStopCommand(const cmd::EmergencyStopCommand& c);
  virtual void handleSafetyResetCommand(const cmd::SafetyResetCommand& c);
 private:
  struct IoSignalCache {
    std::uint64_t cycle{0};
    bool valid{false};
    std::optional<utl::SignalMap> value;
  };

  void cacheInputSignals(std::optional<utl::SignalMap> value);
  void cacheOutputSignals(std::optional<utl::SignalMap> value);
  void runContecCycle();
  void updateHeartBeatOutput(utl::SignalMap& desiredOutputs);
  void setDesiredOutputSignal(std::string_view signal, bool value);
  [[nodiscard]] bool isCpuOverrideActive() const;
  void requireOperationAllowed() const;

  void initializeComponents();
  std::string reconnectComponent(utl::ERobotComponent component);
  void startHomingAxis(cmd::HomingTarget axis, const AxisHomingManager::AxisConfig& config);

  void makeDummyStatus();

  Contec _contec;
  ControlPanel _controlPanel;
  MotorControl _motorControl;
  std::map<utl::ERobotComponent, MachineComponent*> _components;
  std::map<std::string, unsigned int, std::less<>> _inputMapping;
  std::map<std::string, unsigned int, std::less<>> _outputMapping;
  utl::RobotStatus _robotStatus;
  cmd::CommandQueue _commandQueue;
  std::atomic<bool> _isRunning{false};
  std::chrono::milliseconds _loopInterval{10};
  std::chrono::milliseconds _contecLoopInterval{10};
  std::chrono::milliseconds _updateInterval{50};
  bool _statusUpdatesEnabled{true};
  std::size_t _heartBeatPrescaleCycles{0};
  std::size_t _heartBeatCycleCounter{0};
  bool _heartBeatState{false};
  bool _heartBeatEnabled{true};
  std::atomic<bool> _motorAlarmActive{false};
  std::atomic<bool> _cpuOverrideActive{false};
  std::optional<IClock::time_point> _safetyResetTurnOnAt;
  std::optional<IClock::time_point> _safetyResetTurnOffAt;
  std::shared_ptr<IClock> _clock;
  utl::RimoServer<utl::RobotStatus> _robotServer;
  std::thread _commandServerThread;
  std::thread _processThread;
  std::thread _contecThread;
  std::mutex _lifecycleMutex;
  std::mutex _contecStateMutex;
  std::mutex _contecHardwareMutex;
  std::uint64_t _ioCacheCycle{0};
  std::map<utl::EMotor, bool> _homingRequested;
  IoSignalCache _inputSignalsCache;
  IoSignalCache _outputSignalsCache;
  utl::SignalMap _inputSignals;
  utl::SignalMap _outputSignals;
  utl::SignalMap _desiredOutputSignals;
  std::unique_ptr<ControlLoopRunner> _loopRunner;
  std::unique_ptr<MachineController> _controller;
  std::unique_ptr<MachineStatusBuilder> _statusBuilder;
  std::unique_ptr<MachineCommandServer> _commandServer;
  std::unique_ptr<AxisHomingManager> _axisHomingManager;
};
