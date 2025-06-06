#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "windlog.hpp"

void buildAsyncLogger()
{
    std::unique_ptr<windlog::LoggerBuilder> builder(new windlog::GlobalLoggerBuilder());
    builder->buildLoggerName("async_logger");
    builder->buildLoggerLevel(windlog::LogLevel::value::WARN);
    builder->buildLoggerFormatter("[%d{%H:%M:%S}][%c][%f:%l][%p]%T%m%n");
    builder->buildLoggerType(windlog::LoggerBuilder::LoggerType::LOGGER_ASYNC);
    builder->buildLoggerSink<windlog::FileSink>("./logfile/test/log");
    builder->buildLoggerSink<windlog::RollBySizeSink>(1024 * 1024, "./logfile/roll");
    builder->build();
}

TEST(LoggerIntegrationTest, BuildAndUseAcrossDomains)
{
    buildAsyncLogger();  // 明确先构建

    auto logger = windlog::LoggerManager::getInstance().getLogger("async_logger");
    ASSERT_NE(logger, nullptr) << "Logger not found after building";

    LOG__DEBUG("全局接口测试");
    LOG__INFO("全局接口测试");
    LOG__WARN("全局接口测试");
    LOG__ERROR("全局接口测试");

    for (int i = 0; i < 1000000; ++i)
    {
        LOG__FATAL("全局接口测试%d", i);
    }
    logger->flush();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    SUCCEED();  // 如果不中断、无崩溃，就认为通过
}
