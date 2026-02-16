#include "RobotControlPolicy.hpp"

#include <stdexcept>
#include <utility>

IRobotControlPolicy::ControlDecision DefaultRobotControlPolicy::decide(
    const std::optional<SignalMap>& inputs,
    const MachineComponent::State contecState,
    const utl::RobotStatus& robotStatus) const {
  (void)robotStatus;
  if (!inputs || contecState == MachineComponent::State::Error) {
    return {.outputs = std::nullopt, .setToolChangerErrorBlinking = true};
  }
  if (!inputs->contains("button1") || !inputs->contains("button2")) {
    throw std::runtime_error(
        "Missing required input signals 'button1'/'button2' for control policy.");
  }

  SignalMap outputs;
  outputs["light1"] = inputs->at("button1");
  outputs["light2"] = inputs->at("button2");
  return {.outputs = std::move(outputs), .setToolChangerErrorBlinking = false};
}
