#include <gtest/gtest.h>
#include "logger.hpp"  // 你实际的 logger 接口头文件路径

TEST(AsyncLoggerTest, HighVolumeLogging)
{
    std::unique_ptr<windlog::LoggerBuilder> builder(new windlog::LocalLoggerBuilder());
    builder->buildLoggerName("async_logger");
    builder->buildLoggerLevel(windlog::LogLevel::value::WARN);
    builder->buildLoggerFormatter("[%d{%H:%M:%S}][%c][%f:%l][%p]%T%m%n");
    builder->buildLoggerType(windlog::LoggerBuilder::LoggerType::LOGGER_ASYNC);
    // builder->buildLoggerMode(windlog::AsyncLooper::mode::ON_BUFFER_FULL_EXPAND);
    builder->buildLoggerSink<windlog::FileSink>("./logfile/test/log");
    builder->buildLoggerSink<windlog::RollBySizeSink>(1024 * 1024, "./logfile/roll");

    auto logger = builder->build();

    for (int i = 0; i < 1000000; ++i)
    {
        logger->fatal(__FILE__, __LINE__, "%d", i);
    }

    // 给异步线程一点时间落地日志（重要）
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 理论上可以加日志文件检查，例如检查文件是否存在、大小等
    SUCCEED();  // 如果没有崩溃，就算通过
}