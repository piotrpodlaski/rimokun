#include <gtest/gtest.h>

#include <ControlLoopRunner.hpp>

#include <chrono>

#include "server/fakes/FakeClock.hpp"

using namespace std::chrono_literals;

TEST(ControlLoopRunnerTests, RunsControlAndCommandEachCycleAndThrottlesUpdate) {
  FakeClock clock;
  ControlLoopRunner runner(clock, 10ms, 50ms);
  auto state = runner.makeInitialState();

  int controlCalls = 0;
  int commandCalls = 0;
  int updateCalls = 0;

  for (int i = 0; i < 12; ++i) {
    runner.runOneCycle([&]() { ++controlCalls; }, [&]() { ++commandCalls; },
                       [&]() { ++updateCalls; }, state);
  }

  EXPECT_EQ(controlCalls, 12);
  EXPECT_EQ(commandCalls, 12);
  EXPECT_EQ(updateCalls, 3);
}

TEST(ControlLoopRunnerTests, OverrunResynchronizesNextLoopTick) {
  FakeClock clock;
  ControlLoopRunner runner(clock, 10ms, 50ms);
  auto state = runner.makeInitialState();

  runner.runOneCycle([&]() { clock.advanceBy(25ms); }, []() {}, []() {}, state);
  EXPECT_EQ(state.nextLoopAt, IClock::time_point{30ms});

  runner.runOneCycle([&]() { clock.advanceBy(25ms); }, []() {}, []() {}, state);
  EXPECT_EQ(state.nextLoopAt, IClock::time_point{60ms});
}

TEST(ControlLoopRunnerTests, DutyCycleWindowResetsAndSchedulesNextLogSecond) {
  FakeClock clock;
  ControlLoopRunner runner(clock, 10ms, 50ms);
  auto state = runner.makeInitialState();
  state.nextDutyLogAt = IClock::time_point{0ms};

  runner.runOneCycle([&]() { clock.advanceBy(5ms); }, []() {}, []() {}, state);

  EXPECT_EQ(state.dutyCycleSamples, 0u);
  EXPECT_DOUBLE_EQ(state.dutyCycleSum, 0.0);
  EXPECT_EQ(state.nextDutyLogAt, IClock::time_point{1s});
}

TEST(ControlLoopRunnerTests, LargeOverrunTriggersAtMostOneUpdatePerCycle) {
  FakeClock clock;
  ControlLoopRunner runner(clock, 10ms, 50ms);
  auto state = runner.makeInitialState();
  int updateCalls = 0;

  runner.runOneCycle([&]() { clock.advanceBy(220ms); }, []() {},
                     [&]() { ++updateCalls; }, state);
  EXPECT_EQ(updateCalls, 1);
  EXPECT_EQ(state.nextUpdateAt, IClock::time_point{250ms});

  runner.runOneCycle([]() {}, []() {}, [&]() { ++updateCalls; }, state);
  EXPECT_EQ(updateCalls, 1);
}
