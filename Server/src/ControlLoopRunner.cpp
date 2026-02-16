#include "ControlLoopRunner.hpp"

#include <Logger.hpp>

using namespace std::chrono_literals;

ControlLoopRunner::ControlLoopRunner(IClock& clock,
                                     const std::chrono::milliseconds loopInterval,
                                     const std::chrono::milliseconds updateInterval)
    : _clock(clock), _loopInterval(loopInterval), _updateInterval(updateInterval) {}

ControlLoopRunner::State ControlLoopRunner::makeInitialState() const {
  State state;
  const auto now = _clock.now();
  state.nextLoopAt = now;
  state.nextUpdateAt = now;
  state.nextDutyLogAt = now + 1s;
  state.initialized = true;
  return state;
}

void ControlLoopRunner::runOneCycle(const std::function<void()>& controlStep,
                                    const std::function<void()>& commandStep,
                                    const std::function<void()>& updateStep,
                                    State& state) const {
  if (!state.initialized) {
    state = makeInitialState();
  }

  const auto nowBefore = _clock.now();
  if (nowBefore < state.nextLoopAt) {
    _clock.sleepUntil(state.nextLoopAt);
  }
  const auto loopStart = _clock.now();

  controlStep();
  commandStep();

  const auto now = _clock.now();
  if (now >= state.nextUpdateAt) {
    updateStep();
    do {
      state.nextUpdateAt += _updateInterval;
    } while (state.nextUpdateAt <= now);
  }

  const auto loopWork = now - loopStart;
  const auto dutyCycle = std::chrono::duration<double>(loopWork).count() /
                         std::chrono::duration<double>(_loopInterval).count();
  state.dutyCycleSum += dutyCycle;
  ++state.dutyCycleSamples;
  if (now >= state.nextDutyLogAt && state.dutyCycleSamples > 0) {
    const auto avgDutyCycle =
        state.dutyCycleSum / static_cast<double>(state.dutyCycleSamples);
    SPDLOG_DEBUG("Machine loop duty cycle avg {:.3f}% (last {:.3f}%)",
                 avgDutyCycle * 100.0, dutyCycle * 100.0);
    state.dutyCycleSum = 0.0;
    state.dutyCycleSamples = 0;
    do {
      state.nextDutyLogAt += 1s;
    } while (state.nextDutyLogAt <= now);
  }

  state.nextLoopAt += _loopInterval;
  if (state.nextLoopAt <= now) {
    const auto overrun = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - state.nextLoopAt + _loopInterval);
    SPDLOG_WARN("Machine loop overrun by {} ms", overrun.count());
    do {
      state.nextLoopAt += _loopInterval;
    } while (state.nextLoopAt <= now);
  }
}
