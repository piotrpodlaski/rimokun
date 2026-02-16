#include <gtest/gtest.h>

#include <MachineController.hpp>

namespace {
class FakeControlPolicy final : public IRobotControlPolicy {
 public:
  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         const MachineComponent::State contecState,
                         const utl::RobotStatus&) const override {
    ++decideCalls;
    seenInputs = inputs;
    seenState = contecState;
    return decisionToReturn;
  }

  mutable int decideCalls{0};
  mutable std::optional<SignalMap> seenInputs;
  mutable MachineComponent::State seenState{MachineComponent::State::Error};
  ControlDecision decisionToReturn;
};
}  // namespace

TEST(MachineControllerTests, ErrorStateSetsToolChangersToBlinkingError) {
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
      utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::Off;
  auto policy = std::make_unique<FakeControlPolicy>();
  policy->decisionToReturn.setToolChangerErrorBlinking = true;
  auto* policyPtr = policy.get();

  bool outputsCalled = false;
  MachineController controller(
      []() -> std::optional<signal_map_t> { return signal_map_t{{"button1", true}}; },
      [&](const signal_map_t&) { outputsCalled = true; },
      []() -> std::optional<signal_map_t> { return std::nullopt; },
      []() { return MachineComponent::State::Error; }, status, std::move(policy));

  controller.runControlLoopTasks();

  EXPECT_EQ(policyPtr->decideCalls, 1);
  EXPECT_FALSE(outputsCalled);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::ErrorBlinking);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::ErrorBlinking);
}

TEST(MachineControllerTests, ToolChangerCommandSetsExpectedOutputSignal) {
  utl::RobotStatus status;
  signal_map_t seenOutputs;
  auto policy = std::make_unique<FakeControlPolicy>();

  MachineController controller(
      []() -> std::optional<signal_map_t> { return signal_map_t{}; },
      [&](const signal_map_t& outputs) { seenOutputs = outputs; },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", true}};
      },
      []() { return MachineComponent::State::Normal; }, status, std::move(policy));

  controller.handleToolChangerCommand(
      cmd::ToolChangerCommand{utl::EArm::Left, utl::EToolChangerAction::Open});

  ASSERT_TRUE(seenOutputs.contains("toolChangerLeft"));
  EXPECT_TRUE(seenOutputs.at("toolChangerLeft"));
}

TEST(MachineControllerTests, PolicyOutputsAreForwardedToOutputsWriter) {
  utl::RobotStatus status;
  signal_map_t seenOutputs;
  auto policy = std::make_unique<FakeControlPolicy>();
  policy->decisionToReturn.outputs = signal_map_t{{"light1", true}, {"light2", false}};

  MachineController controller(
      []() -> std::optional<signal_map_t> { return signal_map_t{{"button1", true}}; },
      [&](const signal_map_t& outputs) { seenOutputs = outputs; },
      []() -> std::optional<signal_map_t> { return std::nullopt; },
      []() { return MachineComponent::State::Normal; }, status, std::move(policy));

  controller.runControlLoopTasks();

  ASSERT_TRUE(seenOutputs.contains("light1"));
  ASSERT_TRUE(seenOutputs.contains("light2"));
  EXPECT_TRUE(seenOutputs.at("light1"));
  EXPECT_FALSE(seenOutputs.at("light2"));
}
