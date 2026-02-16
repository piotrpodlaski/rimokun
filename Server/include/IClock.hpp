#pragma once

#include <chrono>

class IClock {
 public:
  using duration = std::chrono::steady_clock::duration;
  using time_point = std::chrono::steady_clock::time_point;

  virtual ~IClock() = default;
  [[nodiscard]] virtual time_point now() const = 0;
  virtual void sleepUntil(time_point target) const = 0;
};
