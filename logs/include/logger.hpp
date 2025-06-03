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
#include<cassert>
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

/*
    使用建造者模式来建造日志器, 而不是让用户直接去构造日志器,简化用户的使用复杂度
    1. 抽象一个日志器建造者类(完成日志器对象所需零部件的构建  &  日志器的构建)
        1). 设置日志器类型(不是按照日志器类型进行派生的, 而是依据功能派生的, 日志器的类别,
   在基类中就以区分) 2). 将不同类型日志器的创建放到同一个日志器建造者类中完成
    2. 依据功能派生出两个具体的建造者, 局部日志器的建造者 & 全局的日志器建造者
   之后再引入全局单例管理器后, 回来写全局派生

   因为建造没有顺序要求, 所以指挥者省略了
*/

class LoggerBuilder
{
   public:
    enum class LoggerType
    {
        LOGGER_SYNC,
        LOGGER_ASYNC
    };

    /*
        默认使用同步日志器, 下界等级为debug
    */
    LoggerBuilder() : _type(LoggerType::LOGGER_SYNC), _lower_level(LogLevel::value::DEBUG) {}

    virtual ~LoggerBuilder() = 0;

    void buildLoggerType(LoggerType type) { _type = type; }
    void buildLoggerName(const std::string& name) { _logger_name = name; }
    void buildLoggerLevel(LogLevel::value level) { _lower_level = level; }

    void buildLoggerFormatter(const std::string& pattern)
    {
        _formatter = std::make_shared<Formatter>(pattern);
    }

    template <typename SinkType, typename... Args>
    void buildLoggerSink(Args&&... args)
    {
        auto sink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
        _sinks.emplace_back(std::move(sink));
    }

    virtual Logger::ptr build() = 0;

   protected:
    LoggerType _type;
    std::string _logger_name;

    LogLevel::value _lower_level;
    Formatter::ptr _formatter;

    std::vector<LogSink::ptr> _sinks;
};
inline LoggerBuilder::~LoggerBuilder() = default;

class LocalLoggerBuilder : public LoggerBuilder
{
   public:
    LocalLoggerBuilder() = default;
    ~LocalLoggerBuilder() override = default;
    Logger::ptr build() override
    {
        assert(!_logger_name.empty());
        if(_formatter.get() == nullptr)
        {
            _formatter = std::make_shared<Formatter>();
        }
        if(_sinks.empty())
        {
            buildLoggerSink<StdoutSink>();
        }

        
        if (_type == LoggerBuilder::LoggerType::LOGGER_SYNC)
            return std::make_shared<SyncLogger>(_logger_name, _lower_level, _formatter, _sinks);
        else if (_type == LoggerBuilder::LoggerType::LOGGER_ASYNC)
            return {};
        else
            throw std::runtime_error(
                "Construct a logger that does not yet exist within LocalLoggerBuilder");
    }
};
}  // namespace windlog