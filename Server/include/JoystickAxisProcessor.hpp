#pragma once

#include <cstddef>
#include <deque>

// Encapsulates per-joystick signal processing for a single joystick unit:
//   1. Moving-average filter on X and Y axes
//   2. Baseline calibration (averages the first N samples as the neutral position)
//   3. Button debounce
//
// One instance per physical joystick; ControlPanel holds an array of three.
class JoystickAxisProcessor {
 public:
  struct Output {
    double x{0.0};
    double y{0.0};
    bool button{false};
  };

  JoystickAxisProcessor() = default;

  JoystickAxisProcessor(std::size_t movingAverageDepth,
                        std::size_t baselineSamples,
                        std::size_t buttonDebounceSamples);

  // Update processing parameters and reset accumulated state.
  void reconfigure(std::size_t movingAverageDepth,
                   std::size_t baselineSamples,
                   std::size_t buttonDebounceSamples);

  // Feed one raw sample (xRaw and yRaw in [0, 1023]; buttonRaw true/false).
  // Returns the processed output. Before baseline is ready, x/y are 0.
  Output process(double xRaw, double yRaw, bool buttonRaw);

  void reset();

  [[nodiscard]] bool isBaselineReady() const { return _baselineReady; }

 private:
  static double clipToUnitRange(double value);

  std::size_t _movingAverageDepth;
  std::size_t _baselineSamples;
  std::size_t _buttonDebounceSamples;

  // Moving average
  std::deque<double> _xWindow;
  std::deque<double> _yWindow;
  double _xWindowSum{0.0};
  double _yWindowSum{0.0};

  // Baseline calibration
  double _baselineX{512.0};
  double _baselineY{512.0};
  double _baselineXAcc{0.0};
  double _baselineYAcc{0.0};
  std::size_t _baselineCount{0};
  bool _baselineReady{false};

  // Button debounce
  bool _bStable{false};
  bool _bPending{false};
  std::size_t _bPendingCount{0};
};
