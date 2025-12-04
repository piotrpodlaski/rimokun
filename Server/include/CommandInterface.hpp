#pragma once

#include <mutex>
#include <queue>
#include <optional>
#include <future>
#include <CommonDefinitions.hpp>
#include <yaml-cpp/yaml.h>

namespace cmd {

struct ToolChangerCommand {
  utl::EArm arm;
  utl::EToolChangerAction action;
};

struct Command {
  std::variant<ToolChangerCommand> payload;
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