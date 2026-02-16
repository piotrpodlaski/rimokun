#pragma once

#include <thread>

#include "IClock.hpp"

class SteadyClockAdapter final : public IClock {
 public:
  [[nodiscard]] time_point now() const override {
    return std::chrono::steady_clock::now();
  }

  void sleepUntil(const time_point target) const override {
    std::this_thread::sleep_until(target);
  }
};
