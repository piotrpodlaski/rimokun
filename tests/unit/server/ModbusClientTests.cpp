#include <gtest/gtest.h>

#include <ModbusClient.hpp>

#include <vector>

#include "server/fakes/FakeModbus.hpp"

TEST(ModbusClientTests, RtuFactoryReturnsErrorWhenBackendCreationFails) {
  fake_modbus::reset();
  fake_modbus::failNext(fake_modbus::FailurePoint::NewRtu, "rtu create failed");

  const auto result = ModbusClient::rtu("/dev/fake", 115200, 'N', 8, 1, 1);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().message, "modbus_new_rtu failed");
}

TEST(ModbusClientTests, ConnectAndSetResponseTimeoutSucceed) {
  fake_modbus::reset();
  auto result = ModbusClient::rtu("/dev/fake", 115200, 'N', 8, 1, 1);
  ASSERT_TRUE(result.has_value());
  auto client = std::move(*result);

  EXPECT_TRUE(client.connect().has_value());
  EXPECT_TRUE(client.set_response_timeout(std::chrono::milliseconds{1234}).has_value());
}

TEST(ModbusClientTests, SetSlaveFailureIsPropagated) {
  fake_modbus::reset();
  auto result = ModbusClient::rtu("/dev/fake", 115200, 'N', 8, 1, 1);
  ASSERT_TRUE(result.has_value());
  auto client = std::move(*result);

  fake_modbus::failNext(fake_modbus::FailurePoint::SetSlave, "set slave failed");
  const auto setResult = client.set_slave(42);

  ASSERT_FALSE(setResult.has_value());
  EXPECT_EQ(setResult.error().message, "set slave failed");
}

TEST(ModbusClientTests, HoldingRegisterReadAndWriteRoundTripWorks) {
  fake_modbus::reset();
  auto result = ModbusClient::rtu("/dev/fake", 115200, 'N', 8, 1, 1);
  ASSERT_TRUE(result.has_value());
  auto client = std::move(*result);
  ASSERT_TRUE(client.set_slave(3).has_value());

  fake_modbus::setHoldingRegister(3, 0x0100, 0x1234);
  fake_modbus::setHoldingRegister(3, 0x0101, 0x5678);
  const auto readResult = client.read_holding_registers(0x0100, 2);
  ASSERT_TRUE(readResult.has_value());
  ASSERT_EQ(readResult->size(), 2u);
  EXPECT_EQ((*readResult)[0], 0x1234);
  EXPECT_EQ((*readResult)[1], 0x5678);

  ASSERT_TRUE(client.write_single_register(0x0110, 0xCAFE).has_value());
  const std::vector<std::uint16_t> values{0x1111, 0x2222, 0x3333};
  ASSERT_TRUE(client.write_multiple_registers(0x0120, values).has_value());

  EXPECT_EQ(fake_modbus::getHoldingRegister(3, 0x0110), 0xCAFE);
  EXPECT_EQ(fake_modbus::getHoldingRegister(3, 0x0120), 0x1111);
  EXPECT_EQ(fake_modbus::getHoldingRegister(3, 0x0121), 0x2222);
  EXPECT_EQ(fake_modbus::getHoldingRegister(3, 0x0122), 0x3333);
}

TEST(ModbusClientTests, BitReadApisReturnRequestedCount) {
  fake_modbus::reset();
  auto result = ModbusClient::rtu("/dev/fake", 115200, 'N', 8, 1, 1);
  ASSERT_TRUE(result.has_value());
  auto client = std::move(*result);

  const auto bits = client.read_bits(0x20, 5);
  const auto inputBits = client.read_input_bits(0x30, 4);

  ASSERT_TRUE(bits.has_value());
  ASSERT_TRUE(inputBits.has_value());
  EXPECT_EQ(bits->size(), 5u);
  EXPECT_EQ(inputBits->size(), 4u);
  EXPECT_EQ(*bits, std::vector<bool>({false, false, false, false, false}));
  EXPECT_EQ(*inputBits, std::vector<bool>({false, false, false, false}));
}

TEST(ModbusClientTests, WriteBitAndWriteBitsPropagateErrors) {
  fake_modbus::reset();
  auto result = ModbusClient::rtu("/dev/fake", 115200, 'N', 8, 1, 1);
  ASSERT_TRUE(result.has_value());
  auto client = std::move(*result);

  fake_modbus::failNext(fake_modbus::FailurePoint::WriteBit, "write bit failed");
  auto bitResult = client.write_bit(0x10, true);
  ASSERT_FALSE(bitResult.has_value());
  EXPECT_EQ(bitResult.error().message, "write bit failed");

  fake_modbus::failNext(fake_modbus::FailurePoint::WriteBits, "write bits failed");
  auto bitsResult = client.write_bits(0x10, {true, false, true});
  ASSERT_FALSE(bitsResult.has_value());
  EXPECT_EQ(bitsResult.error().message, "write bits failed");
}

