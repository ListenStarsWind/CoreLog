/*
    日志器模块:
    1. 抽象日志器基类
    2. 派生出不同的子类(同步日志器 & 异步日志器)
*/
#pragma once

#define LEVEL_debug (LogLevel::value::DEBUG)
#define LEVEL_info (LogLevel::value::INFO)
#define LEVEL_warn (LogLevel::value::WARN)
#define LEVEL_error (LogLevel::value::ERROR)
#define LEVEL_fatal (LogLevel::value::FATAL)

#define DEFINE_LOG_FUNC(level)                                                  \
    void level(const std::string& file, size_t line, const char* fmat, ...)     \
    {                                                                           \
        /*检查debug是否在下界等级之上, 或者就是他*/             \
        if (LEVEL_##level < _lower_level)                                       \
            return;                                                             \
                                                                                \
        /*构造出来一个Message供formatter使用*/                         \
        va_list ap;                                                             \
        va_start(ap, fmat);                                                     \
        char* buff = nullptr;                                                   \
        int res = vasprintf(&buff, fmat, ap);                                   \
        va_end(ap);                                                             \
        if (res == -1)                                                          \
            throw std::runtime_error("Logger " #level " failed to: vasprintf"); \
                                                                                \
        LogMsg msg(LEVEL_##level, file, line, _logger_name, buff);              \
        free(buff);                                                             \
                                                                                \
        /*格式化*/                                                           \
        std::stringstream oss;                                                  \
        _formatter->format(oss, msg);                                           \
                                                                                \
        /*实际落地*/                                                        \
        log(oss.str().c_str(), oss.str().size());                               \
    }

#include <atomic>
#include <cstdarg>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

#include "format.hpp"
#include "level.hpp"
#include "message.hpp"
#include "sink.hpp"

namespace windlog {
class Logger
{
   protected:
    /*
        完成实际的落地
    */
    virtual void log(const char* data, size_t len) = 0;

   public:
    using ptr = std::shared_ptr<Logger>;

    /*
         各自完成日志消息的格式化构造, 并将格式化结果传入真正落地的结口
    */

    DEFINE_LOG_FUNC(debug)
    DEFINE_LOG_FUNC(info)
    DEFINE_LOG_FUNC(warn)
    DEFINE_LOG_FUNC(error)
    DEFINE_LOG_FUNC(fatal)

    Logger(const std::string& logger_name, LogLevel::value lower_level, Formatter::ptr formatter,
           const std::vector<LogSink::ptr> sinks)
        : _logger_name(logger_name), _lower_level(lower_level), _formatter(formatter), _sinks(sinks)
    {
    }

    void flush()
    {
        for (const auto& sink : _sinks)
        {
            sink->flush();
        }
    }

    virtual ~Logger() = 0;

   protected:
    std::mutex _mutex;
    std::string _logger_name;

    // 因为下界等级会被频繁访问
    // 但每次访问可能都是加加减减
    // 所以使用原子化代理
    std::atomic<LogLevel::value> _lower_level;
    Formatter::ptr _formatter;

    // 支持多落地方向
    std::vector<LogSink::ptr> _sinks;
};
inline Logger::~Logger() = default;

class SyncLogger : public Logger
{
   public:
    SyncLogger(const std::string& logger_name, LogLevel::value lower_level,
               Formatter::ptr formatter, const std::vector<LogSink::ptr> sinks)
        : Logger(logger_name, lower_level, formatter, sinks)
    {
    }
    ~SyncLogger() override = default;

   private:
    // 同步日志器, 是将日志直接通过落地模块进行日志落地
    void log(const char* data, size_t len) override
    {
        std::unique_lock<std::mutex> lock(_mutex);
        for (const auto& sink : _sinks)
        {
            sink->log(data, len);
        }
    }
};
}  // namespace windlog