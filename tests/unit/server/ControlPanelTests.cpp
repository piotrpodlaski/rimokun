#include <gtest/gtest.h>

#include <ControlPanel.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
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

std::string makeLine(const int x0, const int y0, const int b0, const int x1 = 512,
                     const int y1 = 512, const int b1 = 0, const int x2 = 512,
                     const int y2 = 512, const int b2 = 0) {
  return std::to_string(x0) + " " + std::to_string(y0) + " " +
         std::to_string(b0) + " " + std::to_string(x1) + " " +
         std::to_string(y1) + " " + std::to_string(b1) + " " +
         std::to_string(x2) + " " + std::to_string(y2) + " " +
         std::to_string(b2);
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

TEST(ControlPanelTests, ReinitializeResetsSignalProcessingState) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 1);

  panel.initialize();
  fakeRaw->pushLine(makeLine(512, 512, 0));
  fakeRaw->pushLine(makeLine(800, 512, 0));
  ASSERT_TRUE(waitUntil([&] { return panel.getSnapshot().x[0] > 0.5; }));
  panel.reset();

  panel.initialize();
  fakeRaw->pushLine(makeLine(800, 512, 0));
  fakeRaw->pushLine(makeLine(800, 512, 0));
  ASSERT_TRUE(waitUntil([&] {
    const auto snapshot = panel.getSnapshot();
    return std::abs(snapshot.x[0]) < 0.02;
  }));
  EXPECT_NEAR(panel.getSnapshot().x[0], 0.0, 0.02);

  panel.reset();
}

TEST(ControlPanelTests, BaselineNotReadyKeepsOutputsAtZero) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 3, 1);

  panel.initialize();
  fakeRaw->pushLine(makeLine(900, 100, 0));
  fakeRaw->pushLine(makeLine(900, 100, 0));
  std::this_thread::sleep_for(30ms);

  const auto snapshot = panel.getSnapshot();
  EXPECT_NEAR(snapshot.x[0], 0.0, 0.0001);
  EXPECT_NEAR(snapshot.y[0], 0.0, 0.0001);

  panel.reset();
}

TEST(ControlPanelTests, ClipToUnitRangeAndInvalidValuesDoNotCorruptState) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 1);

  panel.initialize();
  fakeRaw->pushLine(makeLine(512, 512, 0));
  fakeRaw->pushLine(makeLine(1023, 0, 0));
  ASSERT_TRUE(waitUntil([&] {
    const auto snapshot = panel.getSnapshot();
    return snapshot.x[0] > 0.95 && snapshot.y[0] < -0.95;
  }));
  const auto validSnapshot = panel.getSnapshot();
  EXPECT_LE(validSnapshot.x[0], 1.0);
  EXPECT_GE(validSnapshot.y[0], -1.0);

  fakeRaw->pushLine(makeLine(2048, 512, 0));
  std::this_thread::sleep_for(30ms);
  const auto afterInvalid = panel.getSnapshot();
  EXPECT_NEAR(afterInvalid.x[0], validSnapshot.x[0], 0.0001);
  EXPECT_NEAR(afterInvalid.y[0], validSnapshot.y[0], 0.0001);

  panel.reset();
}

TEST(ControlPanelTests, DebounceAppliesToBothRisingAndFallingEdges) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 3);

  panel.initialize();
  fakeRaw->pushLine(makeLine(512, 512, 0));
  fakeRaw->pushLine(makeLine(512, 512, 1));
  fakeRaw->pushLine(makeLine(512, 512, 1));
  fakeRaw->pushLine(makeLine(512, 512, 1));
  ASSERT_TRUE(waitUntil([&] { return panel.getSnapshot().b[0]; }));

  fakeRaw->pushLine(makeLine(512, 512, 0));
  fakeRaw->pushLine(makeLine(512, 512, 0));
  std::this_thread::sleep_for(30ms);
  EXPECT_TRUE(panel.getSnapshot().b[0]);
  fakeRaw->pushLine(makeLine(512, 512, 0));
  ASSERT_TRUE(waitUntil([&] { return !panel.getSnapshot().b[0]; }));

  panel.reset();
}

TEST(ControlPanelTests, DebounceRejectsAlternatingButtonNoise) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 3);

  panel.initialize();
  fakeRaw->pushLine(makeLine(512, 512, 0));
  for (int i = 0; i < 12; ++i) {
    fakeRaw->pushLine(makeLine(512, 512, i % 2));
  }
  std::this_thread::sleep_for(60ms);
  EXPECT_FALSE(panel.getSnapshot().b[0]);

  panel.reset();
}

TEST(ControlPanelTests, EmptyReadsDoNotChangeState) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  ControlPanel panel(std::move(fake), 1, 1, 1);

  panel.initialize();
  std::this_thread::sleep_for(60ms);
  EXPECT_EQ(panel.state(), MachineComponent::State::Normal);
  const auto snapshot = panel.getSnapshot();
  EXPECT_NEAR(snapshot.x[0], 0.0, 0.0001);
  EXPECT_NEAR(snapshot.y[0], 0.0, 0.0001);
  EXPECT_FALSE(snapshot.b[0]);

  panel.reset();
}

TEST(ControlPanelTests, ParserHandlesWhitespaceAndTrailingFields) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 1);

  panel.initialize();
  fakeRaw->pushLine("  512 512 0 512 512 0 512 512 0 \r\n");
  fakeRaw->pushLine("1023 512 1 512 512 0 512 512 0 999 888");
  ASSERT_TRUE(waitUntil([&] {
    const auto snapshot = panel.getSnapshot();
    return snapshot.b[0] && snapshot.x[0] > 0.9;
  }));

  panel.reset();
}

TEST(ControlPanelTests, PartiallyCorruptedNumericFieldRejectsWholeLine) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 1);

  panel.initialize();
  fakeRaw->pushLine(makeLine(512, 512, 0));
  fakeRaw->pushLine(makeLine(800, 512, 0));
  ASSERT_TRUE(waitUntil([&] { return panel.getSnapshot().x[0] > 0.5; }));
  const auto before = panel.getSnapshot();

  fakeRaw->pushLine("700 512 0 bad 512 0 512 512 0");
  std::this_thread::sleep_for(40ms);
  const auto after = panel.getSnapshot();
  EXPECT_NEAR(after.x[0], before.x[0], 0.0001);
  EXPECT_NEAR(after.y[0], before.y[0], 0.0001);
  EXPECT_EQ(after.b[0], before.b[0]);

  panel.reset();
}

TEST(ControlPanelTests, ConcurrentSnapshotReadsAreStableDuringUpdates) {
  auto fake = std::make_unique<FakeControlPanelComm>();
  auto* fakeRaw = fake.get();
  ControlPanel panel(std::move(fake), 1, 1, 1);
  panel.initialize();
  fakeRaw->pushLine(makeLine(512, 512, 0));

  std::atomic<bool> done{false};
  std::thread writer([&] {
    for (int i = 0; i < 400; ++i) {
      const int x = (i % 2 == 0) ? 0 : 1023;
      const int b = (i % 3 == 0) ? 1 : 0;
      fakeRaw->pushLine(makeLine(x, 1023 - x, b));
      std::this_thread::sleep_for(1ms);
    }
    done.store(true, std::memory_order_release);
  });

  std::atomic<bool> badValue{false};
  auto readerFn = [&] {
    while (!done.load(std::memory_order_acquire)) {
      const auto snapshot = panel.getSnapshot();
      for (int i = 0; i < 3; ++i) {
        if (std::isnan(snapshot.x[i]) || std::isnan(snapshot.y[i]) ||
            snapshot.x[i] < -1.0 || snapshot.x[i] > 1.0 ||
            snapshot.y[i] < -1.0 || snapshot.y[i] > 1.0) {
          badValue.store(true, std::memory_order_release);
          return;
        }
      }
    }
  };

  std::thread r1(readerFn);
  std::thread r2(readerFn);
  std::thread r3(readerFn);

  writer.join();
  r1.join();
  r2.join();
  r3.join();

  EXPECT_FALSE(badValue.load(std::memory_order_acquire));
  panel.reset();
}
