#include <unistd.h>

#include "windlog.hpp"

namespace windlog {
/*
    主体思想, 以上一个时间好当前时间的间隔最为研究对象
    看他被时间间隔除以之后, 结果是否大于零
*/

// 支持输出方向扩展, 可自定义其它的输出方向
class RollByTimeSink : public LogSink
{
   private:
    std::string generateLogFilename()
    {
        // 获取当前时间
        time_t ct = util::Date::now();
        struct tm t;
        memset(&t, 0, sizeof(t));
        localtime_r(&ct, &t);
        std::stringstream filename;
        filename << _basename << '-' << t.tm_year + 1900 << t.tm_mon + 1 << t.tm_mday << '-'
                 << t.tm_hour << '_' << t.tm_min << '_' << t.tm_sec << ".log";

        return filename.str();
    }

    void openfile(const std::string& filename)
    {
        // 创建/打开文件, 支持二进制数据
        _ofs.open(filename, std::ios::binary | std::ios::app);
        // 检测是否打开
        if (!_ofs.is_open())
        {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

   public:
    enum class TimeGap
    {
        GAP_SECOND,
        GAP_MINUTE,
        GAP_HOUR,
        GAP_DAY,
    };

    RollByTimeSink(TimeGap gap, const std::string& basename)
        : _basename(basename), _cur_fname(generateLogFilename()), _prev(util::Date::now())
    {
        switch (gap)
        {
            case RollByTimeSink::TimeGap::GAP_SECOND:
                _gap = 1;
                break;
            case RollByTimeSink::TimeGap::GAP_MINUTE:
                _gap = 60;
                break;
            case RollByTimeSink::TimeGap::GAP_HOUR:
                _gap = 60 * 60;
                break;
            case RollByTimeSink::TimeGap::GAP_DAY:
                _gap = 60 * 60 * 24;
                break;
        }

        util::file::createDirectory(util::file::path(_cur_fname));
        // 打开文件
        openfile(_cur_fname);
    }

    ~RollByTimeSink() override = default;

    void log(const char* data, size_t len) override
    {
        time_t now = util::Date::now();
        if ((now - _prev) / _gap)
        {
            _prev = now;
            _ofs.close();
            _cur_fname = generateLogFilename();
            openfile(_cur_fname);
        }

        _ofs.write(data, len);
        if (!_ofs)
        {
            std::cerr << "FileSink write failed to: " << _cur_fname << std::endl;
            throw std::runtime_error("Failed to write to log file: " + _cur_fname);
        }
    }

    void flush() override { _ofs.flush(); }

   private:
    std::ofstream _ofs;
    std::string _basename;
    std::string _cur_fname;
    time_t _prev;
    time_t _gap;
};
}  // namespace windlog

void buildSyncLogger()
{
    // 使用建造者实例化一个全局日志器

    std::unique_ptr<windlog::LoggerBuilder> builder(new windlog::GlobalLoggerBuilder());

    // 日志器的名字, 是必须调用的接口
    builder->buildLoggerName("async_logger");

    // 日志的过滤等级, 只有大于指定等级的日志会被输出, 默认debug
    // 提供五个等级, 按照从低到高的顺序分别是debug, info, warn, error, fatal
    builder->buildLoggerLevel(windlog::LogLevel::value::WARN);

    // 日志输出的格式化设置, 默认为[%p]%T[%d{%H:%M:%S}][%c][%t]%T[%f:%l]%T%m%n
    // %p表示日志级别, %T表示缩进 %d表示日期, 具体格式是时:分:秒, %c表示日志器名称
    // %t表示线程ID, %f表示文件名, %l表示行号, %m表示负载数据
    builder->buildLoggerFormatter("[%d{%H:%M:%S}][%c][%f:%l][%p]%T%m%n");

    // 支持同步日志器和异步日志器两种模式, 默认情况下使用同步日志器
    builder->buildLoggerType(windlog::LoggerBuilder::LoggerType::LOGGER_SYNC);

    // 本身支持三种日志输出方向, 分别是标准输出, 固定文件, 滚动文件
    // 这里的滚动文件, 按照文件大小滚动, 当文件大小达到指定大小, 自动进行切换
    // 滚动文件的第二参数表示文件的基础名, 实际文件会在基础名之上进行创建
    // 默认只有标准输出
    builder->buildLoggerSink<windlog::StdoutSink>();
    builder->buildLoggerSink<windlog::FileSink>("./logfile/test/log");
    builder->buildLoggerSink<windlog::RollBySizeSink>(1024 * 1024, "./logfile/rollBySize");

    // 扩展内容, 基于时间进行滚动的输出方向, 每一秒种, 更换一个文件
    builder->buildLoggerSink<windlog::RollByTimeSink>(windlog::RollByTimeSink::TimeGap::GAP_SECOND,
                                                      "./logfile/rollByTime");

    // 建造出一个日志器, 并将其添加到日志管理器中
    builder->build();
}

int main()
{
    // 构建异步日志器
    buildSyncLogger();

    std::cout << "[Info] async_logger built successfully.\n";

    // 对于日志器的使用, 存在两种接口可以调用,
    // 一是使用如下的LOG__DEBUG(fmt, ...), 调用默认日志器 进行日志输出
    // 二是使用WI__DEBUG(logger, fmt, ...)的形式, 使用指定日志器进行输出

    // 使用默认日志器输出日志（默认日志器下界等级为debug, 都会进行输出）
    LOG__DEBUG("日志测试 - debug");
    LOG__INFO("日志测试 - info");
    LOG__WARN("日志测试 - warn");
    LOG__ERROR("日志测试 - error");
    LOG__FATAL("日志测试 - fatal");

    windlog::rootLogger()->flush();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 使用指定日志器

    // 获取日志器句柄
    auto logger = windlog::getLogger("async_logger");

    time_t now = windlog::util::Date::now();
    while (windlog::util::Date::now() < now + 5)
    {
        WI__FATAL(logger, "这是一条测试日志");
        usleep(1000);
    }
    logger->flush();

    return 0;
}