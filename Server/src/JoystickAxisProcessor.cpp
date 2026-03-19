#include <JoystickAxisProcessor.hpp>

#include <algorithm>
#include <cmath>

JoystickAxisProcessor::JoystickAxisProcessor(const std::size_t movingAverageDepth,
                                             const std::size_t baselineSamples,
                                             const std::size_t buttonDebounceSamples)
    : _movingAverageDepth(std::max<std::size_t>(1u, movingAverageDepth)),
      _baselineSamples(std::max<std::size_t>(1u, baselineSamples)),
      _buttonDebounceSamples(std::max<std::size_t>(1u, buttonDebounceSamples)) {}

void JoystickAxisProcessor::reconfigure(const std::size_t movingAverageDepth,
                                        const std::size_t baselineSamples,
                                        const std::size_t buttonDebounceSamples) {
  _movingAverageDepth = std::max<std::size_t>(1u, movingAverageDepth);
  _baselineSamples = std::max<std::size_t>(1u, baselineSamples);
  _buttonDebounceSamples = std::max<std::size_t>(1u, buttonDebounceSamples);
  reset();
}

JoystickAxisProcessor::Output JoystickAxisProcessor::process(const double xRaw,
                                                              const double yRaw,
                                                              const bool buttonRaw) {
  // --- Baseline accumulation ---
  if (!_baselineReady) {
    _baselineXAcc += xRaw;
    _baselineYAcc += yRaw;
    ++_baselineCount;
    if (_baselineCount >= _baselineSamples) {
      _baselineX = _baselineXAcc / static_cast<double>(_baselineCount);
      _baselineY = _baselineYAcc / static_cast<double>(_baselineCount);
      _baselineReady = true;
    }
  }

  // --- Moving average ---
  _xWindow.push_back(xRaw);
  _xWindowSum += xRaw;
  if (_xWindow.size() > _movingAverageDepth) {
    _xWindowSum -= _xWindow.front();
    _xWindow.pop_front();
  }

  _yWindow.push_back(yRaw);
  _yWindowSum += yRaw;
  if (_yWindow.size() > _movingAverageDepth) {
    _yWindowSum -= _yWindow.front();
    _yWindow.pop_front();
  }

  const double xFiltered = _xWindowSum / static_cast<double>(_xWindow.size());
  const double yFiltered = _yWindowSum / static_cast<double>(_yWindow.size());

  // --- Baseline normalisation ---
  double xOut = 0.0;
  double yOut = 0.0;
  if (_baselineReady) {
    xOut = clipToUnitRange((xFiltered - _baselineX) / 512.0);
    yOut = clipToUnitRange((yFiltered - _baselineY) / 512.0);
  }

  // --- Button debounce ---
  if (buttonRaw == _bStable) {
    _bPending = _bStable;
    _bPendingCount = 0;
  } else {
    if (buttonRaw == _bPending) {
      ++_bPendingCount;
    } else {
      _bPending = buttonRaw;
      _bPendingCount = 1;
    }
    if (_bPendingCount >= _buttonDebounceSamples) {
      _bStable = _bPending;
      _bPendingCount = 0;
    }
  }

  return {.x = xOut, .y = yOut, .button = _bStable};
}

void JoystickAxisProcessor::reset() {
  _xWindow.clear();
  _yWindow.clear();
  _xWindowSum = 0.0;
  _yWindowSum = 0.0;
  _baselineX = 512.0;
  _baselineY = 512.0;
  _baselineXAcc = 0.0;
  _baselineYAcc = 0.0;
  _baselineCount = 0;
  _baselineReady = false;
  _bStable = false;
  _bPending = false;
  _bPendingCount = 0;
}

double JoystickAxisProcessor::clipToUnitRange(const double value) {
  return std::clamp(value, -1.0, 1.0);
}
