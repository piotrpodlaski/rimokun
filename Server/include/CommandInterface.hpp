#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <CommonDefinitions.hpp>


template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

namespace cmd {

struct ToolChangerCommand {
  utl::EArm arm;
  utl::EToolChangerAction action;
};

struct ReconnectCommand {
  utl::ERobotComponent robotComponent;
};

struct MotorDiagnosticsCommand {
  utl::EMotor motor;
};

struct ResetMotorAlarmCommand {
  utl::EMotor motor;
};

struct SetMotorEnabledCommand {
  utl::EMotor motor;
  bool enabled;
};

struct SetAllMotorsEnabledCommand {
  bool enabled;
};

struct ContecDiagnosticsCommand {};

struct EmergencyStopCommand {};

struct Command {
  std::variant<ToolChangerCommand, ReconnectCommand, MotorDiagnosticsCommand,
               ResetMotorAlarmCommand, SetMotorEnabledCommand,
               SetAllMotorsEnabledCommand, ContecDiagnosticsCommand,
               EmergencyStopCommand>
      payload;
  std::promise<std::string> reply;
};

using DispatchFn = std::function<std::string(Command, std::chrono::milliseconds)>;

class CommandQueue {
public:
  explicit CommandQueue(std::size_t maxSize = 0) : _maxSize(maxSize) {}

  bool push(Command cmd) {
    std::lock_guard<std::mutex> lock(_m);
    if (_shutdown) {
      return false;
    }
    if (_maxSize > 0 && _q.size() >= _maxSize) {
      return false;
    }
    _q.push(std::move(cmd));
    _cv.notify_one();
    return true;
  }

  std::optional<Command> pop_wait_for(const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(_m);
    _cv.wait_for(lock, timeout, [this] { return _shutdown || !_q.empty(); });
    if (_q.empty()) return std::nullopt;
    Command c = std::move(_q.front());
    _q.pop();
    return c;
  }

  std::optional<Command> try_pop() {
    std::lock_guard<std::mutex> lock(_m);
    if (_q.empty()) {
      return std::nullopt;
    }
    Command c = std::move(_q.front());
    _q.pop();
    return c;
  }

  void setMaxSize(std::size_t maxSize) {
    std::lock_guard<std::mutex> lock(_m);
    _maxSize = maxSize;
  }

  void shutdown() {
    std::lock_guard<std::mutex> lock(_m);
    _shutdown = true;
    _cv.notify_all();
  }

private:
  std::mutex _m;
  std::condition_variable _cv;
  std::queue<Command> _q;
  std::size_t _maxSize{0};
  bool _shutdown{false};
};
}
