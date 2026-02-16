#include <gtest/gtest.h>

#include <ArKd2RegisterMap.hpp>
#include <Config.hpp>
#include <MotorControl.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "server/fakes/FakeModbus.hpp"

namespace {

std::filesystem::path writeMotorControlConfig(const std::string& motorAddressValue) {
  const auto stamp =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  const auto path =
      std::filesystem::temp_directory_path() /
      ("rimokun_motor_control_test_" + std::to_string(stamp) + ".yaml");

  std::ofstream out(path);
  out << "classes:\n";
  out << "  MotorControl:\n";
  out << "    model: \"AR-KD2\"\n";
  out << "    device: \"/dev/fake\"\n";
  out << "    baud: 115200\n";
  out << "    parity: \"N\"\n";
  out << "    dataBits: 8\n";
  out << "    stopBits: 1\n";
  out << "    responseTimeoutMS: 1000\n";
  out << "    motorAddresses:\n";
  out << "      XLeft: " << motorAddressValue << "\n";
  out.close();

  return path;
}

std::uint8_t readSelectedOperationIdForMotor(const int slave) {
  return Motor::decodeOperationIdFromInputRaw(
      fake_modbus::getHoldingRegister(slave, makeArKd2RegisterMap().driverInputCommandLower));
}

}  // namespace

TEST(MotorControlTests, InitializeSetsNormalStateAndResetReturnsToError) {
  fake_modbus::reset();
  const auto configPath = writeMotorControlConfig("5");
  utl::Config::instance().setConfigPath(configPath.string());

  MotorControl control;
  control.initialize();
  EXPECT_EQ(control.state(), MachineComponent::State::Normal);
  EXPECT_EQ(control.motors().size(), 1u);

  control.reset();
  EXPECT_EQ(control.state(), MachineComponent::State::Error);
  EXPECT_TRUE(control.motors().empty());

  std::filesystem::remove(configPath);
}

TEST(MotorControlTests, InitializeFailsAndSetsErrorWhenAddressOutOfRange) {
  fake_modbus::reset();
  const auto configPath = writeMotorControlConfig("248");
  utl::Config::instance().setConfigPath(configPath.string());

  MotorControl control;
  EXPECT_THROW(control.initialize(), std::runtime_error);
  EXPECT_EQ(control.state(), MachineComponent::State::Error);
  EXPECT_TRUE(control.motors().empty());

  std::filesystem::remove(configPath);
}

TEST(MotorControlTests, SpeedModeUsesAbsoluteSpeedAndBufferedUpdate) {
  fake_modbus::reset();
  const auto configPath = writeMotorControlConfig("5");
  utl::Config::instance().setConfigPath(configPath.string());

  MotorControl control;
  control.initialize();
  control.setMode(utl::EMotor::XLeft, MotorControlMode::Speed);
  control.setSpeed(utl::EMotor::XLeft, -2200);

  auto map = makeArKd2RegisterMap();
  const auto upper = fake_modbus::getHoldingRegister(5, map.speedNo0 + 2);
  const auto lower = fake_modbus::getHoldingRegister(5, map.speedNo0 + 3);
  EXPECT_EQ(upper, static_cast<std::uint16_t>(2200u >> 16));
  EXPECT_EQ(lower, static_cast<std::uint16_t>(2200u & 0xFFFFu));
  EXPECT_EQ(readSelectedOperationIdForMotor(5), 1);

  std::filesystem::remove(configPath);
}

TEST(MotorControlTests, PositionModeStartMovementSelectsOp2AndPulsesStart) {
  fake_modbus::reset();
  const auto configPath = writeMotorControlConfig("5");
  utl::Config::instance().setConfigPath(configPath.string());

  MotorControl control;
  control.initialize();
  control.setMode(utl::EMotor::XLeft, MotorControlMode::Position);
  control.setPosition(utl::EMotor::XLeft, 777);
  control.startMovement(utl::EMotor::XLeft);

  auto map = makeArKd2RegisterMap();
  const auto upper = fake_modbus::getHoldingRegister(5, map.positionNo0 + 4);
  const auto lower = fake_modbus::getHoldingRegister(5, map.positionNo0 + 5);
  EXPECT_EQ(upper, static_cast<std::uint16_t>(777u >> 16));
  EXPECT_EQ(lower, static_cast<std::uint16_t>(777u & 0xFFFFu));
  EXPECT_EQ(readSelectedOperationIdForMotor(5), 2);

  const auto inputRaw = fake_modbus::getHoldingRegister(5, map.driverInputCommandLower);
  const auto startBit = static_cast<std::uint16_t>(MotorInputFlag::Start);
  EXPECT_EQ(inputRaw & startBit, 0u);

  std::filesystem::remove(configPath);
}

TEST(MotorControlTests, UnknownMotorIdIsRejectedByCommandMethods) {
  fake_modbus::reset();
  const auto configPath = writeMotorControlConfig("5");
  utl::Config::instance().setConfigPath(configPath.string());

  MotorControl control;
  control.initialize();

  EXPECT_THROW(control.setSpeed(utl::EMotor::YLeft, 1000), std::runtime_error);
  EXPECT_THROW(control.startMovement(utl::EMotor::YLeft), std::runtime_error);

  std::filesystem::remove(configPath);
}

