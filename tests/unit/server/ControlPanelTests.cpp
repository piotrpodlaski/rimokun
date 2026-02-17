#include <gtest/gtest.h>

#include <ControlPanel.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;

class FakeControlPanelComm final : public IControlPanelComm {
 public:
  void open() override { _opened.store(true, std::memory_order_release); }

  void closeNoThrow() noexcept override {
    _closed.store(true, std::memory_order_release);
    _cv.notify_all();
  }

  std::optional<std::string> readLine() override {
    if (_throwOnRead.load(std::memory_order_acquire)) {
      throw std::runtime_error("forced read error");
    }
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait_for(lock, 5ms, [&] {
      return !_lines.empty() || _closed.load(std::memory_order_acquire);
    });
    if (_lines.empty()) {
      return std::nullopt;
    }
    auto line = std::move(_lines.front());
    _lines.pop();
    return line;
  }

  std::string describe() const override { return "fake-control-panel"; }

  void pushLine(std::string line) {
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _lines.push(std::move(line));
    }
    _cv.notify_all();
  }

  void setThrowOnRead(bool enabled) {
    _throwOnRead.store(enabled, std::memory_order_release);
    _cv.notify_all();
  }

 private:
  std::atomic<bool> _opened{false};
  std::atomic<bool> _closed{false};
  std::atomic<bool> _throwOnRead{false};
  mutable std::mutex _mutex;
  std::condition_variable _cv;
  std::queue<std::string> _lines;
};

bool waitUntil(const std::function<bool()>& pred,
               const std::chrono::milliseconds timeout = 300ms) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) {
      return true;
    }
    std::this_thread::sleep_for(5ms);
  }
  return pred();
}

}  // namespace

TEST(ControlPanelTests, BaselineAndNormalizationWorkForInjectedComm) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 2, 1);

  panel.initialize();
  fakeRaw->pushLine("512 512 0 512 512 0 512 512 0");
  fakeRaw->pushLine("512 512 0 512 512 0 512 512 0");
  fakeRaw->pushLine("1023 0 1 512 512 0 512 512 0");

  ASSERT_TRUE(waitUntil([&] {
    const auto snapshot = panel.getSnapshot();
    return snapshot.b[0];
  }));

  const auto snapshot = panel.getSnapshot();
  EXPECT_NEAR(snapshot.x[0], 0.998, 0.01);
  EXPECT_NEAR(snapshot.y[0], -1.0, 0.0001);
  EXPECT_TRUE(snapshot.b[0]);

  panel.reset();
}

TEST(ControlPanelTests, MovingAverageSmoothingUsesConfiguredDepth) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 3, 1, 1);

  panel.initialize();
  fakeRaw->pushLine("512 512 0 512 512 0 512 512 0");
  fakeRaw->pushLine("612 512 0 512 512 0 512 512 0");
  fakeRaw->pushLine("712 512 0 512 512 0 512 512 0");
  fakeRaw->pushLine("812 512 0 512 512 0 512 512 0");

  ASSERT_TRUE(waitUntil([&] {
    const auto snapshot = panel.getSnapshot();
    return snapshot.x[0] > 0.35;
  }));

  const auto snapshot = panel.getSnapshot();
  EXPECT_NEAR(snapshot.x[0], 200.0 / 512.0, 0.02);

  panel.reset();
}

TEST(ControlPanelTests, ButtonDebounceRequiresConfiguredConsecutiveSamples) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 3);

  panel.initialize();
  fakeRaw->pushLine("512 512 0 512 512 0 512 512 0");
  fakeRaw->pushLine("512 512 1 512 512 0 512 512 0");
  fakeRaw->pushLine("512 512 1 512 512 0 512 512 0");
  std::this_thread::sleep_for(30ms);
  EXPECT_FALSE(panel.getSnapshot().b[0]);
  fakeRaw->pushLine("512 512 1 512 512 0 512 512 0");

  ASSERT_TRUE(waitUntil([&] { return panel.getSnapshot().b[0]; }));
  EXPECT_TRUE(panel.getSnapshot().b[0]);

  panel.reset();
}

TEST(ControlPanelTests, MalformedLineIsIgnoredAndValidLineStillApplies) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 1);

  panel.initialize();
  fakeRaw->pushLine("512 512 0 512 512 0 512 512 0");
  fakeRaw->pushLine("invalid malformed line");
  std::this_thread::sleep_for(30ms);
  EXPECT_NEAR(panel.getSnapshot().x[0], 0.0, 0.0001);
  fakeRaw->pushLine("800 512 0 512 512 0 512 512 0");

  ASSERT_TRUE(waitUntil([&] {
    const auto snapshot = panel.getSnapshot();
    return snapshot.x[0] > 0.5;
  }));

  panel.reset();
}

TEST(ControlPanelTests, ReadFailureTransitionsToErrorState) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 1);

  panel.initialize();
  fakeRaw->setThrowOnRead(true);

  ASSERT_TRUE(waitUntil([&] {
    return panel.state() == MachineComponent::State::Error;
  }));

  panel.reset();
}
