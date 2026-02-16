#pragma once

#include <algorithm>

#include <IClock.hpp>

class FakeClock final : public IClock {
 public:
  [[nodiscard]] time_point now() const override { return _now; }

  void sleepUntil(const time_point target) const override {
    _now = std::max(_now, target);
  }

  void advanceBy(const duration delta) { _now += delta; }

 private:
  mutable time_point _now{};
};
