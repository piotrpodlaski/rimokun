#pragma once

#include <map>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "CommonDefinitions.hpp"
#include "MachineComponent.hpp"
#include "MotorControl.hpp"

class IRobotControlPolicy {
 public:
  using SignalMap = std::map<std::string, bool>;

  struct MotorIntent {
    utl::EMotor motorId;
    std::optional<MotorControlMode> mode;
    std::optional<MotorControlDirection> direction;
    std::optional<std::int32_t> speed;
    std::optional<std::int32_t> position;
    bool startMovement{false};
    bool stopMovement{false};
  };

  struct ControlDecision {
    std::optional<SignalMap> outputs;
    std::vector<MotorIntent> motorIntents;
    bool setToolChangerErrorBlinking{false};
  };

  virtual ~IRobotControlPolicy() = default;

  virtual ControlDecision decide(const std::optional<SignalMap>& inputs,
                                 const std::optional<SignalMap>& outputs,
                                 MachineComponent::State contecState,
                                 const utl::RobotStatus& robotStatus) const = 0;
};

class DefaultRobotControlPolicy final : public IRobotControlPolicy {
 public:
  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         const std::optional<SignalMap>& outputs,
                         MachineComponent::State contecState,
                         const utl::RobotStatus& robotStatus) const override;
};

class RimoKunControlPolicy final : public IRobotControlPolicy {
 public:
  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         const std::optional<SignalMap>& outputs,
                         MachineComponent::State contecState,
                         const utl::RobotStatus& robotStatus) const override;
};
