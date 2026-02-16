#include <gtest/gtest.h>

#include <CommandInterface.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

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

TEST(CommandQueueTests, MultipleProducersAllCommandsAreConsumed) {
  cmd::CommandQueue queue;
  constexpr int producerCount = 4;
  constexpr int commandsPerProducer = 200;
  constexpr int totalCommands = producerCount * commandsPerProducer;

  std::vector<std::thread> producers;
  producers.reserve(producerCount);
  for (int p = 0; p < producerCount; ++p) {
    producers.emplace_back([&]() {
      for (int i = 0; i < commandsPerProducer; ++i) {
        ASSERT_TRUE(queue.push(makeReconnectCommand()));
      }
    });
  }

  int consumed = 0;
  while (consumed < totalCommands) {
    auto popped = queue.pop_wait_for(500ms);
    ASSERT_TRUE(popped.has_value());
    ++consumed;
  }

  for (auto& producer : producers) {
    producer.join();
  }

  EXPECT_EQ(consumed, totalCommands);
  EXPECT_FALSE(queue.try_pop().has_value());
}

TEST(CommandQueueTests, ShutdownRejectsNewPushesUnderLoad) {
  cmd::CommandQueue queue;
  std::atomic<int> accepted{0};
  std::atomic<int> rejected{0};

  std::thread producer([&]() {
    for (int i = 0; i < 10000; ++i) {
      if (queue.push(makeReconnectCommand())) {
        accepted.fetch_add(1, std::memory_order_relaxed);
      } else {
        rejected.fetch_add(1, std::memory_order_relaxed);
        break;
      }
      std::this_thread::sleep_for(50us);
    }
  });

  std::this_thread::sleep_for(5ms);
  queue.shutdown();
  producer.join();

  EXPECT_GT(rejected.load(std::memory_order_relaxed), 0);

  int drained = 0;
  while (queue.try_pop().has_value()) {
    ++drained;
  }
  EXPECT_EQ(drained, accepted.load(std::memory_order_relaxed));
}
