#include <gtest/gtest.h>

#include <VMotorStats.hpp>

namespace {
class FakeMotorStats final : public VMotorStats {
 public:
  void apply(const utl::SingleMotorStatus& status) { configure(status); }

  void setSpeed(double value) const override { speed = value; }
  void setSpeedRpm(double value) const override { speedRpm = value; }
  void setCurrentPosition(double value) const override { currentPosition = value; }
  void setTargetPosition(double value) const override { targetPosition = value; }
  void setBrake(utl::ELEDState value) const override { brake = value; }
  void setEnabled(utl::ELEDState value) const override { enabled = value; }
  void setStatus(utl::ELEDState value) const override { status = value; }
  void setMotorId(utl::EMotor value) override { motorId = value; }
  void setAxisState(utl::EAxisState value) override { axisState = value; }
  void setSpeedCommand(double pct, double maxMmPs) const override {
    speedCommandPercent = pct;
    modeMaxLinearSpeedMmPerSec = maxMmPs;
  }

  mutable double speed{0};
  mutable double speedRpm{0};
  mutable double currentPosition{0};
  mutable double targetPosition{0};
  mutable utl::ELEDState brake{utl::ELEDState::On};
  mutable utl::ELEDState enabled{utl::ELEDState::On};
  mutable utl::ELEDState status{utl::ELEDState::On};
  utl::EMotor motorId{utl::EMotor::XLeft};
  utl::EAxisState axisState{utl::EAxisState::Locked};
  mutable double speedCommandPercent{0};
  mutable double modeMaxLinearSpeedMmPerSec{0};
};
}  // namespace

TEST(VMotorStatsTests, ConfigureCopiesNumericFieldsAndFlags) {
  FakeMotorStats stats;
  utl::SingleMotorStatus input{
      .currentPosition = 101.0,
      .targetPosition = -22.0,
      .speed = 5.5,
      .speedRpm = 123.0,
      .torque = 17,
      .state = utl::ELEDState::Warning,
      .flags = {{utl::EMotorStatusFlags::BrakeApplied, utl::ELEDState::Error},
                {utl::EMotorStatusFlags::Enabled, utl::ELEDState::Off}},
      .speedCommandPercent = -42.0,
      .modeMaxLinearSpeedMmPerSec = 120.0};

  stats.apply(input);

  EXPECT_DOUBLE_EQ(stats.currentPosition, 101.0);
  EXPECT_DOUBLE_EQ(stats.targetPosition, -22.0);
  EXPECT_DOUBLE_EQ(stats.speed, 5.5);
  EXPECT_DOUBLE_EQ(stats.speedRpm, 123.0);
  EXPECT_EQ(stats.status, utl::ELEDState::Warning);
  EXPECT_EQ(stats.brake, utl::ELEDState::Error);
  EXPECT_EQ(stats.enabled, utl::ELEDState::Off);
  EXPECT_DOUBLE_EQ(stats.speedCommandPercent, -42.0);
  EXPECT_DOUBLE_EQ(stats.modeMaxLinearSpeedMmPerSec, 120.0);
}

TEST(VMotorStatsTests, ConfigureDefaultsFlagsToOffWhenMissing) {
  FakeMotorStats stats;
  stats.brake = utl::ELEDState::On;
  stats.enabled = utl::ELEDState::Error;
  stats.status = utl::ELEDState::Error;
  utl::SingleMotorStatus input{
      .currentPosition = 0,
      .targetPosition = 0,
      .speed = 0,
      .speedRpm = 0,
      .torque = 0,
      .state = utl::ELEDState::Off,
      .flags = {}};

  stats.apply(input);

  EXPECT_EQ(stats.status, utl::ELEDState::Off);
  EXPECT_EQ(stats.brake, utl::ELEDState::Off);
  EXPECT_EQ(stats.enabled, utl::ELEDState::Off);
}
