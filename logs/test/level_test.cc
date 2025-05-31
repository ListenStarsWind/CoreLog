#include <gtest/gtest.h>
#include "level.hpp"

using windlog::LogLevel;

TEST(LogLevelTest, ToStringConversion)
{
    EXPECT_STREQ(LogLevel::toString(LogLevel::value::UNKNOW), "UNKNOW");
    EXPECT_STREQ(LogLevel::toString(LogLevel::value::DEBUG), "DEBUG");
    EXPECT_STREQ(LogLevel::toString(LogLevel::value::INFO), "INFO");
    EXPECT_STREQ(LogLevel::toString(LogLevel::value::WARN), "WARN");
    EXPECT_STREQ(LogLevel::toString(LogLevel::value::ERROR), "ERROR");
    EXPECT_STREQ(LogLevel::toString(LogLevel::value::FATAL), "FATAL");
    EXPECT_STREQ(LogLevel::toString(LogLevel::value::OFF), "OFF");
}