#pragma once

#include <chrono>
#include <cstddef>
#include <functional>

#include "IClock.hpp"

class ControlLoopRunner {
 public:
  struct State {
    IClock::time_point nextLoopAt{};
    IClock::time_point nextUpdateAt{};
    IClock::time_point nextDutyLogAt{};
    double dutyCycleSum{0.0};
    std::size_t dutyCycleSamples{0};
    bool initialized{false};
  };

  ControlLoopRunner(IClock& clock,
                    std::chrono::milliseconds loopInterval,
                    std::chrono::milliseconds updateInterval);

  [[nodiscard]] State makeInitialState() const;
  void runOneCycle(const std::function<void()>& controlStep,
                   const std::function<void()>& commandStep,
                   const std::function<void()>& updateStep,
                   State& state) const;

 private:
  IClock& _clock;
  std::chrono::milliseconds _loopInterval;
  std::chrono::milliseconds _updateInterval;
};
