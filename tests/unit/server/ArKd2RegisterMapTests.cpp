#include <gtest/gtest.h>

#include <ArKd2FullRegisterMap.hpp>
#include <ArKd2RegisterMap.hpp>

#include <algorithm>
#include <vector>

TEST(ArKd2RegisterMapTests, CoreAddressesMatchExpectedManualValues) {
  const auto map = makeArKd2RegisterMap();
  EXPECT_EQ(map.driverInputCommandLower, 0x007D);
  EXPECT_EQ(map.driverOutputCommandLower, 0x007F);
  EXPECT_EQ(map.presentAlarm, 0x0080);
  EXPECT_EQ(map.presentWarning, 0x0096);
  EXPECT_EQ(map.communicationErrorCode, 0x00AC);
  EXPECT_EQ(map.directIoAndBrakeStatus, 0x00D4);
  EXPECT_EQ(map.alarmResetCommand, 0x0180);
  EXPECT_EQ(map.positionNo0, 0x0400);
  EXPECT_EQ(map.speedNo0, 0x0480);
  EXPECT_EQ(map.operationModeNo0, 0x0500);
  EXPECT_EQ(map.accelerationNo0, 0x0600);
  EXPECT_EQ(map.decelerationNo0, 0x0680);
}

TEST(ArKd2RegisterMapTests, FullRegisterMapIsSortedAndUnique) {
  const auto regs = arKd2FullRegisterMap();
  ASSERT_FALSE(regs.empty());

  std::vector<std::uint16_t> addresses;
  addresses.reserve(regs.size());
  for (const auto& entry : regs) {
    addresses.push_back(entry.address);
  }

  EXPECT_TRUE(std::is_sorted(addresses.begin(), addresses.end()));
  const auto uniqueEnd = std::unique(addresses.begin(), addresses.end());
  EXPECT_EQ(uniqueEnd, addresses.end());
}

TEST(ArKd2RegisterMapTests, LookupReturnsKnownNamesAndUnknownWhenMissing) {
  const auto inputName = arKd2RegisterName(0x007D);
  const auto alarmName = arKd2RegisterName(0x0080);
  const auto absent = arKd2RegisterName(0x0001);

  ASSERT_TRUE(inputName.has_value());
  ASSERT_TRUE(alarmName.has_value());
  EXPECT_EQ(*inputName, "Driver input command (lower)");
  EXPECT_EQ(*alarmName, "Present alarm (upper)");
  EXPECT_FALSE(absent.has_value());
}

TEST(ArKd2RegisterMapTests, CompactMapAddressesExistInFullRegisterMap) {
  const auto map = makeArKd2RegisterMap();
  // Full table currently covers operational/diagnostic/manual-mapped registers.
  // Operation data base ranges (0x0400+) are validated separately by exact value
  // checks in CoreAddressesMatchExpectedManualValues.
  const std::vector<int> addresses{
      map.driverInputCommandLower,
      map.driverOutputCommandLower,
      map.presentAlarm,
      map.presentWarning,
      map.communicationErrorCode,
      map.directIoAndBrakeStatus,
      map.alarmResetCommand,
      map.commandPosition,
      map.commandSpeed,
      map.actualPosition,
      map.actualSpeed,
  };

  for (const auto addr : addresses) {
    EXPECT_TRUE(arKd2RegisterName(static_cast<std::uint16_t>(addr)).has_value())
        << "Missing address 0x" << std::hex << addr;
  }
}
