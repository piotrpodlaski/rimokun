#pragma once

#include <mutex>
#include <queue>
#include <optional>
#include <future>
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
  void push(Command cmd) {
    std::lock_guard<std::mutex> lock(_m);
    _q.push(std::move(cmd));
  }

  std::optional<Command> try_pop() {
    std::lock_guard<std::mutex> lock(_m);
    if (_q.empty()) return std::nullopt;
    Command c = std::move(_q.front());
    _q.pop();
    return c;
  }

private:
  std::mutex _m;
  std::queue<Command> _q;
};
}