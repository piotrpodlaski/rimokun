#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Logger.hpp"

#ifndef RIMOKUN_TIMING_ENABLED
#define RIMOKUN_TIMING_ENABLED 0
#endif

namespace utl {

#if RIMOKUN_TIMING_ENABLED
class TimingMetricsRegistry {
 public:
  static TimingMetricsRegistry& instance() {
    static TimingMetricsRegistry registry;
    return registry;
  }

  void record(const char* name, const std::chrono::nanoseconds duration) {
    const auto nowNs = nowInNs();
    {
      std::lock_guard<std::mutex> lock(_mutex);
      auto& stat = _stats[std::string(name)];
      stat.count += 1;
      stat.totalNs += static_cast<std::uint64_t>(duration.count());
      stat.maxNs = std::max(stat.maxNs, static_cast<std::uint64_t>(duration.count()));
    }
    maybeReport(nowNs);
  }

 private:
  struct Stat {
    std::uint64_t count{0};
    std::uint64_t totalNs{0};
    std::uint64_t maxNs{0};
  };

  static std::uint64_t nowInNs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
  }

  void maybeReport(const std::uint64_t nowNs) {
    if (nowNs < _nextReportNs) {
      return;
    }
    std::lock_guard<std::mutex> lock(_reportMutex);
    if (nowNs < _nextReportNs) {
      return;
    }
    _nextReportNs = nowNs + kReportEveryNs;
    reportAndReset();
  }

  void reportAndReset() {
    std::vector<std::pair<std::string, Stat>> snapshot;
    {
      std::lock_guard<std::mutex> lock(_mutex);
      snapshot.reserve(_stats.size());
      for (const auto& [name, stat] : _stats) {
        snapshot.emplace_back(name, stat);
      }
      _stats.clear();
    }
    if (snapshot.empty()) {
      return;
    }
    std::sort(snapshot.begin(), snapshot.end(),
              [](const auto& lhs, const auto& rhs) {
                return lhs.second.totalNs > rhs.second.totalNs;
              });
    SPDLOG_INFO("Timing report (top {}):", std::min<std::size_t>(snapshot.size(), kTopN));
    const auto entries = std::min<std::size_t>(snapshot.size(), kTopN);
    for (std::size_t i = 0; i < entries; ++i) {
      const auto& [name, stat] = snapshot[i];
      const auto totalMs = static_cast<double>(stat.totalNs) / 1'000'000.0;
      const auto avgUs = stat.count == 0
                             ? 0.0
                             : static_cast<double>(stat.totalNs) /
                                   static_cast<double>(stat.count) / 1'000.0;
      const auto maxUs = static_cast<double>(stat.maxNs) / 1'000.0;
      SPDLOG_INFO("  {:<40} count={:<8} total={:>10.3f} ms avg={:>10.3f} us max={:>10.3f} us",
                  name, stat.count, totalMs, avgUs, maxUs);
    }
  }

  TimingMetricsRegistry()
      : _nextReportNs(nowInNs() + kReportEveryNs) {}

  static constexpr std::uint64_t kReportEveryNs = 5ull * 1'000'000'000ull;
  static constexpr std::size_t kTopN = 32;

  std::mutex _mutex;
  std::unordered_map<std::string, Stat> _stats;
  std::mutex _reportMutex;
  std::uint64_t _nextReportNs{0};
};

class ScopedTiming {
 public:
  explicit ScopedTiming(const char* name)
      : _name(name), _start(std::chrono::steady_clock::now()) {}

  ~ScopedTiming() {
    TimingMetricsRegistry::instance().record(
        _name, std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now() - _start));
  }

 private:
  const char* _name;
  std::chrono::steady_clock::time_point _start;
};
#endif

}  // namespace utl

#if RIMOKUN_TIMING_ENABLED
#define RIMO_TIMING_CONCAT_IMPL(a, b) a##b
#define RIMO_TIMING_CONCAT(a, b) RIMO_TIMING_CONCAT_IMPL(a, b)
#define RIMO_TIMED_SCOPE(name_literal) \
  ::utl::ScopedTiming RIMO_TIMING_CONCAT(_rimo_timed_scope_, __COUNTER__)(name_literal)
#else
#define RIMO_TIMED_SCOPE(name_literal) ((void)0)
#endif
