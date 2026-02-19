#include <gtest/gtest.h>

#include <ArKd2RegisterMap.hpp>
#include <ModbusClient.hpp>
#include <Motor.hpp>

#include <cstdint>

#include "server/fakes/FakeModbus.hpp"

namespace {

ModbusClient makeBus() {
  auto busRes = ModbusClient::rtu("/dev/fake", 115200, 'N', 8, 1, 1);
  if (!busRes) {
    throw std::runtime_error(busRes.error().message);
  }
  auto bus = std::move(*busRes);
  auto connectRes = bus.connect();
  if (!connectRes) {
    throw std::runtime_error(connectRes.error().message);
  }
  return bus;
}

std::uint16_t operationBitsFromId(const std::uint8_t opId) {
  std::uint16_t raw = 0;
  if ((opId & 1u) != 0u) raw |= static_cast<std::uint16_t>(MotorInputFlag::M0);
  if ((opId & 2u) != 0u) raw |= static_cast<std::uint16_t>(MotorInputFlag::M1);
  if ((opId & 4u) != 0u) raw |= static_cast<std::uint16_t>(MotorInputFlag::M2);
  if ((opId & 8u) != 0u) raw |= static_cast<std::uint16_t>(MotorInputFlag::Ms0);
  if ((opId & 16u) != 0u)
    raw |= static_cast<std::uint16_t>(MotorInputFlag::Ms1);
  if ((opId & 32u) != 0u)
    raw |= static_cast<std::uint16_t>(MotorInputFlag::Ms2);
  return raw;
}

}  // namespace

TEST(MotorTests, DecodeOperationIdFromInputRawUsesAllSelectionBits) {
  const std::uint16_t raw = static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(MotorInputFlag::M0) |
      static_cast<std::uint16_t>(MotorInputFlag::M2) |
      static_cast<std::uint16_t>(MotorInputFlag::Ms1) |
      static_cast<std::uint16_t>(MotorInputFlag::Ms2));

  EXPECT_EQ(Motor::decodeOperationIdFromInputRaw(raw), 53);
}

TEST(MotorTests, SetSelectedOperationIdPreservesNonOperationBits) {
  fake_modbus::reset();
  auto map = makeArKd2RegisterMap();
  auto bus = makeBus();
  Motor motor(utl::EMotor::XLeft, 7, map);

  const auto keepBits = static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(MotorInputFlag::Start) |
      static_cast<std::uint16_t>(MotorInputFlag::Fwd));
  fake_modbus::setHoldingRegister(7, map.driverInputCommandLower,
                                  static_cast<std::uint16_t>(
                                      keepBits | operationBitsFromId(6)));

  motor.setSelectedOperationId(bus, 37);

  const auto raw = fake_modbus::getHoldingRegister(7, map.driverInputCommandLower);
  EXPECT_EQ(raw, static_cast<std::uint16_t>(keepBits | operationBitsFromId(37)));
  EXPECT_EQ(motor.readSelectedOperationId(bus), 37);
}

TEST(MotorTests, SetSelectedOperationIdRejectsOutOfRangeId) {
  fake_modbus::reset();
  auto map = makeArKd2RegisterMap();
  auto bus = makeBus();
  Motor motor(utl::EMotor::XLeft, 7, map);

  EXPECT_THROW(motor.setSelectedOperationId(bus, 64), std::runtime_error);
}

TEST(MotorTests, UpdateConstantSpeedBufferedWritesInactiveOperationAndSwitchesId) {
  fake_modbus::reset();
  auto map = makeArKd2RegisterMap();
  auto bus = makeBus();
  Motor motor(utl::EMotor::XLeft, 7, map);

  fake_modbus::setHoldingRegister(7, map.driverInputCommandLower, 0);

  motor.updateConstantSpeedBuffered(bus, 123456);

  const auto speedUpper = fake_modbus::getHoldingRegister(7, map.speedNo0 + 2);
  const auto speedLower = fake_modbus::getHoldingRegister(7, map.speedNo0 + 3);
  EXPECT_EQ(speedUpper, static_cast<std::uint16_t>(123456u >> 16));
  EXPECT_EQ(speedLower, static_cast<std::uint16_t>(123456u & 0xFFFFu));
  EXPECT_EQ(motor.readSelectedOperationId(bus), 1);
}

TEST(MotorTests, UpdateConstantSpeedBufferedUsesCachedSelectedOperationId) {
  fake_modbus::reset();
  auto map = makeArKd2RegisterMap();
  auto bus = makeBus();
  Motor motor(utl::EMotor::XLeft, 7, map);

  motor.configureConstantSpeedPair(bus, 1000, 1000, 2000, 2000);
  fake_modbus::failNext(fake_modbus::FailurePoint::ReadRegisters,
                        "read should not be needed for buffered update");

  EXPECT_NO_THROW(motor.updateConstantSpeedBuffered(bus, 654321));
  EXPECT_EQ(motor.readSelectedOperationId(bus), 1);
}

TEST(MotorTests, DecodeDirectIoAndBrakeStatusDecodesExpectedFlagNames) {
  Motor motor(utl::EMotor::XLeft, 7, makeArKd2RegisterMap());
  const std::uint32_t raw =
      (static_cast<std::uint32_t>(0x0121u) << 16) | 0x2C42u;

  const auto status = motor.decodeDirectIoAndBrakeStatus(raw);

  EXPECT_EQ(status.reg00D4, 0x0121u);
  EXPECT_EQ(status.reg00D5, 0x2C42u);
  const std::vector<std::string_view> expected{
      "OUT0", "OUT5", "MB", "IN7", "IN5", "IN4", "IN0", "-LS"};
  EXPECT_EQ(status.activeFlags, expected);
}

TEST(MotorTests, ResetAlarmDoesNothingWhenNoAlarmIsActive) {
  fake_modbus::reset();
  auto map = makeArKd2RegisterMap();
  auto bus = makeBus();
  Motor motor(utl::EMotor::XLeft, 7, map);

  fake_modbus::setHoldingRegister(7, map.presentAlarm, 0x0000u);
  fake_modbus::setHoldingRegister(7, map.presentAlarm + 1, 0x0000u);
  fake_modbus::setHoldingRegister(7, map.alarmResetCommand, 0xABCDu);

  motor.resetAlarm(bus);

  EXPECT_EQ(fake_modbus::getHoldingRegister(7, map.alarmResetCommand), 0xABCDu);
}

TEST(MotorTests, ResetAlarmPerformsZeroToOneTransitionWhenAlarmIsActive) {
  fake_modbus::reset();
  auto map = makeArKd2RegisterMap();
  auto bus = makeBus();
  Motor motor(utl::EMotor::XLeft, 7, map);

  fake_modbus::setHoldingRegister(7, map.presentAlarm, 0x0000u);
  fake_modbus::setHoldingRegister(7, map.presentAlarm + 1, 0x0005u);
  fake_modbus::setHoldingRegister(7, map.alarmResetCommand, 0xABCDu);

  motor.resetAlarm(bus);

  EXPECT_EQ(fake_modbus::getHoldingRegister(7, map.alarmResetCommand), 0x0001u);
}
