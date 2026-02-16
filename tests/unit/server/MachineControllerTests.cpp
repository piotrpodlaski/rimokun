#include <gtest/gtest.h>

#include <MachineController.hpp>

TEST(MachineControllerTests, ErrorStateSetsToolChangersToBlinkingError) {
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
      utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::Off;

  bool outputsCalled = false;
  MachineController controller(
      []() -> std::optional<signal_map_t> { return std::nullopt; },
      [&](const signal_map_t&) { outputsCalled = true; },
      []() -> std::optional<signal_map_t> { return std::nullopt; },
      []() { return MachineComponent::State::Error; }, status);

  controller.runControlLoopTasks();

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

  MachineController controller(
      []() -> std::optional<signal_map_t> { return signal_map_t{}; },
      [&](const signal_map_t& outputs) { seenOutputs = outputs; },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", true}};
      },
      []() { return MachineComponent::State::Normal; }, status);

  controller.handleToolChangerCommand(
      cmd::ToolChangerCommand{utl::EArm::Left, utl::EToolChangerAction::Open});

  ASSERT_TRUE(seenOutputs.contains("toolChangerLeft"));
  EXPECT_TRUE(seenOutputs.at("toolChangerLeft"));
}
