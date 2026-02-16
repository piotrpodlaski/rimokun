#include <gtest/gtest.h>

#include <VMotorStats.hpp>

namespace {
class FakeMotorStats final : public VMotorStats {
 public:
  void apply(const utl::SingleMotorStatus& status) { configure(status); }

  void setTorque(int value) const override { torque = value; }
  void setSpeed(double value) const override { speed = value; }
  void setCurrentPosition(double value) const override { currentPosition = value; }
  void setTargetPosition(double value) const override { targetPosition = value; }
  void setBrake(utl::ELEDState value) const override { brake = value; }
  void setEnabled(utl::ELEDState value) const override { enabled = value; }
  void setMotorId(utl::EMotor value) override { motorId = value; }

  mutable int torque{0};
  mutable double speed{0};
  mutable double currentPosition{0};
  mutable double targetPosition{0};
  mutable utl::ELEDState brake{utl::ELEDState::On};
  mutable utl::ELEDState enabled{utl::ELEDState::On};
  utl::EMotor motorId{utl::EMotor::XLeft};
};
}  // namespace

TEST(VMotorStatsTests, ConfigureCopiesNumericFieldsAndFlags) {
  FakeMotorStats stats;
  utl::SingleMotorStatus input{
      .currentPosition = 101.0,
      .targetPosition = -22.0,
      .speed = 5.5,
      .torque = 17,
      .flags = {{utl::EMotorStatusFlags::BrakeApplied, utl::ELEDState::Error},
                {utl::EMotorStatusFlags::Enabled, utl::ELEDState::Off}}};

  stats.apply(input);

  EXPECT_DOUBLE_EQ(stats.currentPosition, 101.0);
  EXPECT_DOUBLE_EQ(stats.targetPosition, -22.0);
  EXPECT_DOUBLE_EQ(stats.speed, 5.5);
  EXPECT_EQ(stats.torque, 17);
  EXPECT_EQ(stats.brake, utl::ELEDState::Error);
  EXPECT_EQ(stats.enabled, utl::ELEDState::Off);
}

TEST(VMotorStatsTests, ConfigureDefaultsFlagsToOffWhenMissing) {
  FakeMotorStats stats;
  stats.brake = utl::ELEDState::On;
  stats.enabled = utl::ELEDState::Error;
  utl::SingleMotorStatus input{
      .currentPosition = 0,
      .targetPosition = 0,
      .speed = 0,
      .torque = 0,
      .flags = {}};

  stats.apply(input);

  EXPECT_EQ(stats.brake, utl::ELEDState::Off);
  EXPECT_EQ(stats.enabled, utl::ELEDState::Off);
}
