#pragma once

#include <map>
#include <optional>
#include <string>

#include "CommonDefinitions.hpp"
#include "MachineComponent.hpp"

class IRobotControlPolicy {
 public:
  using SignalMap = std::map<std::string, bool>;

  struct ControlDecision {
    std::optional<SignalMap> outputs;
    bool setToolChangerErrorBlinking{false};
  };

  virtual ~IRobotControlPolicy() = default;

  virtual ControlDecision decide(const std::optional<SignalMap>& inputs,
                                 MachineComponent::State contecState,
                                 const utl::RobotStatus& robotStatus) const = 0;
};

class DefaultRobotControlPolicy final : public IRobotControlPolicy {
 public:
  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         MachineComponent::State contecState,
                         const utl::RobotStatus& robotStatus) const override;
};
