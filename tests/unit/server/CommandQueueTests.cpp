#include <gtest/gtest.h>

#include <CommandInterface.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

namespace {
cmd::Command makeReconnectCommand() {
  cmd::Command command;
  command.payload = cmd::ReconnectCommand{utl::ERobotComponent::ControlPanel};
  return command;
}
}  // namespace

TEST(CommandQueueTests, PushThenTryPopReturnsCommand) {
  cmd::CommandQueue queue;
  auto command = makeReconnectCommand();

  ASSERT_TRUE(queue.push(std::move(command)));

  auto popped = queue.try_pop();
  ASSERT_TRUE(popped.has_value());
  EXPECT_FALSE(queue.try_pop().has_value());
}

TEST(CommandQueueTests, PopWaitForReturnsQueuedCommand) {
  cmd::CommandQueue queue;
  ASSERT_TRUE(queue.push(makeReconnectCommand()));

  auto popped = queue.pop_wait_for(20ms);
  ASSERT_TRUE(popped.has_value());
}

TEST(CommandQueueTests, ShutdownUnblocksWaiter) {
  cmd::CommandQueue queue;
  std::atomic<bool> waiterReturned{false};

  std::thread waiter([&]() {
    auto popped = queue.pop_wait_for(5s);
    EXPECT_FALSE(popped.has_value());
    waiterReturned.store(true, std::memory_order_release);
  });

  std::this_thread::sleep_for(30ms);
  queue.shutdown();
  waiter.join();

  EXPECT_TRUE(waiterReturned.load(std::memory_order_acquire));
}

TEST(CommandQueueTests, PushAfterShutdownIsRejected) {
  cmd::CommandQueue queue;
  queue.shutdown();

  EXPECT_FALSE(queue.push(makeReconnectCommand()));
}
