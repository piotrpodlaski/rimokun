#include <gtest/gtest.h>

#include <MachineCommandProcessor.hpp>
#include <MachineCommandServer.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <queue>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace {
class FakeCommandChannel final : public ICommandChannel {
 public:
  explicit FakeCommandChannel(const std::chrono::milliseconds receiveWait)
      : _receiveWait(receiveWait) {}

  std::optional<nlohmann::json> receiveCommand() override {
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait_for(lock, _receiveWait, [this]() { return !_commands.empty(); });
    if (_commands.empty()) {
      return std::nullopt;
    }
    auto cmd = _commands.front();
    _commands.pop();
    return cmd;
  }

  void sendResponse(const nlohmann::json& response) override {
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _responses.push(response);
    }
    _cv.notify_all();
  }

  void enqueueCommand(const nlohmann::json& command) {
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _commands.push(command);
    }
    _cv.notify_all();
  }

  std::optional<nlohmann::json> popResponseWaitFor(
      const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait_for(lock, timeout, [this]() { return !_responses.empty(); });
    if (_responses.empty()) {
      return std::nullopt;
    }
    auto response = _responses.front();
    _responses.pop();
    return response;
  }

 private:
  std::chrono::milliseconds _receiveWait;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::queue<nlohmann::json> _commands;
  std::queue<nlohmann::json> _responses;
};
}  // namespace

TEST(MachineCommandServerTests, RespondsToCommandWithinDeadline) {
  MachineCommandProcessor processor;
  FakeCommandChannel channel(20ms);
  MachineCommandServer commandServer(processor, channel);
  std::atomic<bool> running{true};

  bool dispatched = false;
  std::chrono::milliseconds seenTimeout{0};
  std::thread serverThread([&]() {
    commandServer.runLoop(
        running, [&](cmd::Command command, const std::chrono::milliseconds timeout) {
          dispatched = true;
          seenTimeout = timeout;
          EXPECT_TRUE(std::holds_alternative<cmd::ReconnectCommand>(command.payload));
          return std::string{};
        });
  });

  nlohmann::json command{
      {"type", "reset"},
      {"system", "ControlPanel"},
  };

  const auto start = std::chrono::steady_clock::now();
  channel.enqueueCommand(command);
  const auto response = channel.popResponseWaitFor(500ms);
  const auto elapsed = std::chrono::steady_clock::now() - start;

  ASSERT_TRUE(response.has_value());
  EXPECT_EQ((*response)["status"].get<std::string>(), "OK");
  EXPECT_TRUE(dispatched);
  EXPECT_EQ(seenTimeout, 2s);
  EXPECT_LT(elapsed, 500ms);

  running.store(false, std::memory_order_release);
  serverThread.join();
}

TEST(MachineCommandServerTests, ShutdownWhileBlockedOnReceiveExitsPromptlyWhenWoken) {
  MachineCommandProcessor processor;
  FakeCommandChannel channel(1s);
  MachineCommandServer commandServer(processor, channel);
  std::atomic<bool> running{true};

  std::thread serverThread([&]() {
    commandServer.runLoop(running, [](cmd::Command, std::chrono::milliseconds) {
      return std::string{};
    });
  });

  std::this_thread::sleep_for(30ms);
  running.store(false, std::memory_order_release);

  std::thread wakeThread([&]() {
    std::this_thread::sleep_for(100ms);
    nlohmann::json command{
        {"type", "reset"},
        {"system", "ControlPanel"},
    };
    channel.enqueueCommand(command);
  });

  const auto start = std::chrono::steady_clock::now();
  serverThread.join();
  const auto elapsed = std::chrono::steady_clock::now() - start;
  wakeThread.join();

  EXPECT_LT(elapsed, 700ms);
}

TEST(MachineCommandServerTests, MalformedCommandsReturnErrorsAndLoopKeepsServing) {
  MachineCommandProcessor processor;
  FakeCommandChannel channel(20ms);
  MachineCommandServer commandServer(processor, channel);
  std::atomic<bool> running{true};

  int dispatchCalls = 0;
  std::thread serverThread([&]() {
    commandServer.runLoop(
        running, [&](cmd::Command, const std::chrono::milliseconds) {
          ++dispatchCalls;
          return std::string{};
        });
  });

  nlohmann::json malformedNoType{
      {"system", "ControlPanel"},
  };
  channel.enqueueCommand(malformedNoType);
  auto r1 = channel.popResponseWaitFor(500ms);
  ASSERT_TRUE(r1.has_value());
  EXPECT_EQ((*r1)["status"].get<std::string>(), "Error");

  nlohmann::json unknownType{
      {"type", "nonsense"},
  };
  channel.enqueueCommand(unknownType);
  auto r2 = channel.popResponseWaitFor(500ms);
  ASSERT_TRUE(r2.has_value());
  EXPECT_EQ((*r2)["status"].get<std::string>(), "Error");

  nlohmann::json malformedToolChanger{
      {"type", "toolChanger"},
      {"position", "Left"},
  };
  channel.enqueueCommand(malformedToolChanger);
  auto r3 = channel.popResponseWaitFor(500ms);
  ASSERT_TRUE(r3.has_value());
  EXPECT_EQ((*r3)["status"].get<std::string>(), "Error");

  nlohmann::json valid{
      {"type", "reset"},
      {"system", "ControlPanel"},
  };
  channel.enqueueCommand(valid);
  auto r4 = channel.popResponseWaitFor(500ms);
  ASSERT_TRUE(r4.has_value());
  EXPECT_EQ((*r4)["status"].get<std::string>(), "OK");
  EXPECT_EQ(dispatchCalls, 1);

  running.store(false, std::memory_order_release);
  serverThread.join();
}
