#pragma once

#include <MachineComponent.hpp>
#include <IControlPanelComm.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <thread>


class ControlPanel final : public MachineComponent {
 public:
  struct Snapshot {
    std::array<double, 3> x{};
    std::array<double, 3> y{};
    std::array<bool, 3> b{};
  };

  ControlPanel();
  ~ControlPanel() override;
  void initialize() override;
  void reset() override;
  [[nodiscard]] Snapshot getSnapshot() const;
  [[nodiscard]] utl::ERobotComponent componentType() const override {
    return utl::ERobotComponent::ControlPanel;
  }

 private:
  void readerLoop();
  void processLine(const std::string& line);
  void resetSignalProcessingState();
  static double clipToUnitRange(double value);

  std::unique_ptr<IControlPanelComm> _comm;
  std::size_t _movingAverageDepth;
  std::size_t _baselineSamples;
  std::size_t _buttonDebounceSamples;
  std::atomic<bool> _readerRunning{false};
  std::thread _readerThread;
  std::array<std::atomic<double>, 3> _x{};
  std::array<std::atomic<double>, 3> _y{};
  std::array<std::atomic<bool>, 3> _b{};
  std::array<std::deque<double>, 3> _xWindow;
  std::array<std::deque<double>, 3> _yWindow;
  std::array<double, 3> _xWindowSum{};
  std::array<double, 3> _yWindowSum{};
  std::array<double, 3> _baselineX{};
  std::array<double, 3> _baselineY{};
  std::array<double, 3> _baselineXAcc{};
  std::array<double, 3> _baselineYAcc{};
  std::array<bool, 3> _bStable{};
  std::array<bool, 3> _bPending{};
  std::array<std::size_t, 3> _bPendingCount{};
  std::size_t _baselineCount{0};
  bool _baselineReady{false};
};
