#pragma once

#include <JoystickAxisProcessor.hpp>
#include <MachineComponent.hpp>
#include <IControlPanelComm.hpp>

#include <array>
#include <atomic>
#include <cstdint>
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
  ControlPanel(std::unique_ptr<IControlPanelComm> comm,
               std::size_t movingAverageDepth,
               std::size_t baselineSamples,
               std::size_t buttonDebounceSamples);
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

  std::unique_ptr<IControlPanelComm> _comm;
  std::size_t _movingAverageDepth;
  std::size_t _baselineSamples;
  std::size_t _buttonDebounceSamples;
  std::atomic<bool> _readerRunning{false};
  std::thread _readerThread;
  std::array<std::atomic<double>, 3> _x{};
  std::array<std::atomic<double>, 3> _y{};
  std::array<std::atomic<bool>, 3> _b{};
  // One processor per physical joystick; lazily constructed in resetSignalProcessingState().
  std::array<JoystickAxisProcessor, 3> _processors;
};
