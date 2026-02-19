#pragma once

#include <CommonDefinitions.hpp>
#include <atomic>

class MachineComponent {
 public:
  enum class State { Normal, Warning, Error };

  virtual ~MachineComponent() = default;

  virtual void initialize() = 0;
  virtual void reset() = 0;
  [[nodiscard]] virtual utl::ERobotComponent componentType() const = 0;

  [[nodiscard]] State state() const {
    return _state.load(std::memory_order_acquire);
  }

 protected:
  void setState(State state) { _state.store(state, std::memory_order_release); }

 private:
  std::atomic<State> _state{State::Error};
};
