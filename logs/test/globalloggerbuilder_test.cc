#include <gtest/gtest.h>
#include "logger.hpp"
#include <thread>
#include <chrono>

void buildAsyncLogger() {
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

    for (int i = 0; i < 1000000; ++i) {
        logger->fatal(__FILE__, __LINE__, "%d", i);
    }
    logger->flush();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    SUCCEED();  // 如果不中断、无崩溃，就认为通过
}
