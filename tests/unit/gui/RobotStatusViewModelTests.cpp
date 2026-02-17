#include <gtest/gtest.h>

#include <RobotStatusViewModel.hpp>

TEST(RobotStatusViewModelTests, ToolChangerMapsFlagsAndButtonState) {
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
      utl::ELEDState::On;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::OpenSen] =
      utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::ClosedSen] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::On;

  const auto vm = RobotStatusViewModel::toolChangerForArm(status, utl::EArm::Left);
  ASSERT_TRUE(vm.has_value());
  EXPECT_EQ(vm->prox, utl::ELEDState::On);
  EXPECT_EQ(vm->closedSensor, utl::ELEDState::On);
  EXPECT_TRUE(vm->openButtonEnabled);
  EXPECT_FALSE(vm->closeButtonEnabled);
}

TEST(RobotStatusViewModelTests, ResetControlsMapsServerAndComponentStates) {
  utl::RobotStatus status;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::Error;
  status.robotComponents[utl::ERobotComponent::MotorControl] = utl::ELEDState::On;
  status.robotComponents[utl::ERobotComponent::ControlPanel] = utl::ELEDState::Off;

  const auto vm =
      RobotStatusViewModel::resetControlsForStatus(std::make_optional(status));
  EXPECT_EQ(vm.server, utl::ELEDState::On);
  EXPECT_EQ(vm.contec, utl::ELEDState::Error);
  EXPECT_EQ(vm.motor, utl::ELEDState::On);
  EXPECT_EQ(vm.controlPanel, utl::ELEDState::Off);
  EXPECT_TRUE(vm.resetContecEnabled);
  EXPECT_FALSE(vm.resetMotorEnabled);
  EXPECT_FALSE(vm.resetControlPanelEnabled);
}
