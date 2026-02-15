#pragma once

#include <chrono>
#include <condition_variable>
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

struct AuxCommand {
  utl::ERobotComponent robotComponent;
};


struct Command {
  std::variant<ToolChangerCommand, ReconnectCommand> payload;
  std::promise<std::string> reply;
};

class CommandQueue {
public:
  bool push(Command cmd) {
    std::lock_guard<std::mutex> lock(_m);
    if (_shutdown) {
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

  void shutdown() {
    std::lock_guard<std::mutex> lock(_m);
    _shutdown = true;
    _cv.notify_all();
  }

private:
  std::mutex _m;
  std::condition_variable _cv;
  std::queue<Command> _q;
  bool _shutdown{false};
};
}
