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
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "buffer.hpp"
#include "format.hpp"
#include "level.hpp"
#include "looper.hpp"
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

    const std::string name() { return _logger_name; }

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
    异步日志器的设计:
    1. 仍旧继承于抽象基类Logger, 并对其虚函数接口, log 和 析构函数
    2. 通过异步消息处理器, 配合回调函数, 实现对消息的异步落地

    管理的特有成员包括
    1. 异步处理器
    2. 异步处理器缓冲区满时的处理策略
*/

class AsyncLogger : public Logger
{
    // 异步线程的落地策略
    void realLog(Buffer& buffer)
    {
        // 这个也不需要加锁, 因为_looper天然自带线程安全保护, 本身就是串行的
        for (const auto& sink : _sinks)
        {
            sink->log(buffer.readAbleBegin(), buffer.readAbleSize());
        }
    }

   public:
    AsyncLogger(const std::string& logger_name, LogLevel::value lower_level,
                Formatter::ptr formatter, const std::vector<LogSink::ptr> sinks,
                AsyncLooper::mode mode)
        : Logger(logger_name, lower_level, formatter, sinks),
          _looper(std::make_shared<AsyncLooper>(
              std::bind(&AsyncLogger::realLog, this, std::placeholders::_1), mode))
    {
    }

    ~AsyncLogger() = default;

    void log(const char* data, size_t len) override
    {
        // 无需加锁, _looper::push中的锁足以保证线程安全
        _looper->push(data, len);
    }

   private:
    AsyncLooper::ptr _looper;
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
        默认缓冲区满时, 业务线程主动阻塞
    */
    LoggerBuilder()
        : _type(LoggerType::LOGGER_SYNC),
          _lower_level(LogLevel::value::DEBUG),
          _mode(AsyncLooper::mode::ON_BUFFER_FULL_BLOCK)
    {
    }

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

    void buildLoggerMode(AsyncLooper::mode mode) { _mode = mode; }

    virtual Logger::ptr build() = 0;

   protected:
    LoggerType _type;
    std::string _logger_name;

    LogLevel::value _lower_level;
    Formatter::ptr _formatter;

    std::vector<LogSink::ptr> _sinks;

    AsyncLooper::mode _mode;
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
        if (_formatter.get() == nullptr)
        {
            _formatter = std::make_shared<Formatter>();
        }
        if (_sinks.empty())
        {
            buildLoggerSink<StdoutSink>();
        }

        if (_type == LoggerBuilder::LoggerType::LOGGER_SYNC)
            return std::make_shared<SyncLogger>(_logger_name, _lower_level, _formatter, _sinks);
        else if (_type == LoggerBuilder::LoggerType::LOGGER_ASYNC)
            return std::make_shared<AsyncLogger>(_logger_name, _lower_level, _formatter, _sinks,
                                                 _mode);
        else
            throw std::runtime_error(
                "Construct a logger that does not yet exist within LocalLoggerBuilder");
    }
};

class LoggerManager
{
    /*
        构造函数一定要使用本地建造者, 因为如果用户不调用的话,
       日志管理器就应该在全局建造者里面实例化 如果使用全局建造者对默认日志器进行构造,
       就会相互依赖, 全局建造者为了建造一个日志器实例化 日志管理器,
       但日志管理又调用全局建造者实例化自己, 相互依赖, 逻辑死锁
    */

    LoggerManager()
    {
        std::unique_ptr<windlog::LoggerBuilder> builder(new windlog::LocalLoggerBuilder());
        builder->buildLoggerName("root");
        _root_logger = builder->build();
        _loggers[_root_logger->name()] = _root_logger;
    }

    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

   public:
    ~LoggerManager() = default;

    static LoggerManager& getInstance()
    {
        static LoggerManager _eton;
        return _eton;
    }

    void addLogger(Logger::ptr& logger)
    {
        if (hashLogger(logger->name()))
            return;

        std::unique_lock<std::mutex> lock(_mutex);
        _loggers[logger->name()] = logger;
    }

    bool hashLogger(const std::string& name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _loggers.find(name);
        return it != _loggers.end();
    }

    Logger::ptr getLogger(const std::string& name)
    {
        if (hashLogger(name) == false)
            return Logger::ptr();

        std::unique_lock<std::mutex> lock(_mutex);
        return _loggers[name];
    }

    Logger::ptr rootLogger() { return _root_logger; }

   private:
    std::mutex _mutex;
    Logger::ptr _root_logger;
    std::unordered_map<std::string, Logger::ptr> _loggers;
};

/*
    全局日志器建造者, 在把日志器建造完成之后, 会交由日志管理器进行管理
*/

class GlobalLoggerBuilder : public LoggerBuilder
{
   public:
    GlobalLoggerBuilder() = default;
    ~GlobalLoggerBuilder() override = default;

    // 建造前请确保已经为日志器命名
    Logger::ptr build() override
    {
        assert(!_logger_name.empty());
        if (_formatter.get() == nullptr)
        {
            _formatter = std::make_shared<Formatter>();
        }
        if (_sinks.empty())
        {
            buildLoggerSink<StdoutSink>();
        }

        if (LoggerManager::getInstance().hashLogger(_logger_name))
            return LoggerManager::getInstance().getLogger(_logger_name);

        Logger::ptr logger;
        if (_type == LoggerBuilder::LoggerType::LOGGER_SYNC)
            logger = std::make_shared<SyncLogger>(_logger_name, _lower_level, _formatter, _sinks);
        else if (_type == LoggerBuilder::LoggerType::LOGGER_ASYNC)
            logger = std::make_shared<AsyncLogger>(_logger_name, _lower_level, _formatter, _sinks,
                                                   _mode);
        else
            throw std::runtime_error(
                "Construct a logger that does not yet exist within LocalLoggerBuilder");

        LoggerManager::getInstance().addLogger(logger);
        return logger;
    }
};

}  // namespace windlog