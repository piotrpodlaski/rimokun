#include <gtest/gtest.h>

#include <ArKd2Diagnostics.hpp>

TEST(ArKd2DiagnosticsTests, KnownAlarmCodeReturnsDetailedInfo) {
  const auto info = arKd2FindAlarm(0x20);
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->code, 0x20);
  EXPECT_EQ(info->type, "Overcurrent");
  EXPECT_FALSE(info->cause.empty());
  EXPECT_FALSE(info->remedialAction.empty());
}

TEST(ArKd2DiagnosticsTests, KnownWarningCodeReturnsDetailedInfo) {
  const auto info = arKd2FindWarning(0x31);
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->code, 0x31);
  EXPECT_EQ(info->type, "Overspeed");
}

TEST(ArKd2DiagnosticsTests, KnownCommunicationErrorCodeReturnsDetailedInfo) {
  const auto info = arKd2FindCommunicationError(0x8C);
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(info->code, 0x8C);
  EXPECT_EQ(info->type, "Outside setting range");
}

TEST(ArKd2DiagnosticsTests, UnknownCodesReturnNullopt) {
  EXPECT_FALSE(arKd2FindAlarm(0x01).has_value());
  EXPECT_FALSE(arKd2FindWarning(0x01).has_value());
  EXPECT_FALSE(arKd2FindCommunicationError(0x01).has_value());
}

TEST(ArKd2DiagnosticsTests, DomainDispatchFindsEntriesForEachDomain) {
  const auto alarm = arKd2FindCode(ArKd2CodeDomain::Alarm, 0x10);
  const auto warning = arKd2FindCode(ArKd2CodeDomain::Warning, 0x10);
  const auto comm = arKd2FindCode(ArKd2CodeDomain::CommunicationError, 0x84);

  ASSERT_TRUE(alarm.has_value());
  ASSERT_TRUE(warning.has_value());
  ASSERT_TRUE(comm.has_value());
  EXPECT_EQ(alarm->type, "Excessive position deviation");
  EXPECT_EQ(warning->type, "Excessive position deviation");
  EXPECT_EQ(comm->type, "RS-485 communication error");
}

TEST(ArKd2DiagnosticsTests, DomainNamesAreStable) {
  EXPECT_EQ(arKd2DomainName(ArKd2CodeDomain::Alarm), "alarm");
  EXPECT_EQ(arKd2DomainName(ArKd2CodeDomain::Warning), "warning");
  EXPECT_EQ(arKd2DomainName(ArKd2CodeDomain::CommunicationError),
            "communication error");
}

