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
  virtual void handleHomingCommand(const cmd::HomingCommand& c);
  virtual nlohmann::json handleContecDiagnosticsCommand(
      const cmd::ContecDiagnosticsCommand& c);
  virtual void handleEmergencyStopCommand(const cmd::EmergencyStopCommand& c);
 private:
  struct IoSignalCache {
    std::uint64_t cycle{0};
    bool valid{false};
    std::optional<utl::SignalMap> value;
  };

  struct HomingAxisConfig {
    utl::EMotor motorId;
    std::int32_t speedStepsPerSec{0};
    std::int32_t acceleration{0};
    std::int32_t startingDirection{0};
  };

  struct HomingSessionRuntime {
    bool active{false};
    std::size_t currentAxisIndex{0};
    IClock::time_point currentAxisStartedAt{};
  };

  void cacheInputSignals(std::optional<utl::SignalMap> value);
  void cacheOutputSignals(std::optional<utl::SignalMap> value);

  void initializeComponents();
  std::string reconnectComponent(utl::ERobotComponent component);
  std::vector<cmd::HomingTarget> loadHomingOrder() const;
  std::map<cmd::HomingTarget, HomingAxisConfig> loadHomingAxisConfigs() const;
  std::chrono::milliseconds loadHomingTimeout() const;
  void homeAxesInOrder(const std::vector<cmd::HomingTarget>& axes);
  void homeSingleAxis(cmd::HomingTarget axis);
  void beginHomingSession(const std::vector<cmd::HomingTarget>& axes);
  void startNextHomingAxis();
  void advanceHomingSessionFromStatus();
  void finishCurrentHomingAxis(utl::EHomingState state, std::string message = {});
  static utl::EHomingAxis homingAxisFromTarget(cmd::HomingTarget target);
  static std::optional<utl::EMotor> homingMotorFromAxis(utl::EHomingAxis axis);

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
  std::chrono::milliseconds _updateInterval{50};
  bool _statusUpdatesEnabled{true};
  std::shared_ptr<IClock> _clock;
  utl::RimoServer<utl::RobotStatus> _robotServer;
  std::thread _commandServerThread;
  std::thread _processThread;
  std::mutex _lifecycleMutex;
  std::uint64_t _ioCacheCycle{0};
  std::vector<cmd::HomingTarget> _homingOrder;
  std::map<cmd::HomingTarget, HomingAxisConfig> _homingAxisConfigs;
  std::chrono::milliseconds _homingTimeout{std::chrono::seconds{60}};
  std::map<utl::EMotor, bool> _homingRequested;
  HomingSessionRuntime _homingSessionRuntime;
  IoSignalCache _inputSignalsCache;
  IoSignalCache _outputSignalsCache;
  utl::SignalMap _inputSignals;
  utl::SignalMap _outputSignals;
  std::unique_ptr<ControlLoopRunner> _loopRunner;
  std::unique_ptr<MachineController> _controller;
  std::unique_ptr<MachineStatusBuilder> _statusBuilder;
  std::unique_ptr<MachineCommandServer> _commandServer;
};
