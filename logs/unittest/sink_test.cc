#include "sink.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "format.hpp"
#include "level.hpp"
#include "message.hpp"

namespace fs = std::filesystem;
using namespace windlog;

class SinkTest : public ::testing::Test
{
   protected:
    Formatter fmt{"[%p] %T[%d{%H:%M:%S}][%c][%t]%T[%f:%l]%T%m%n"};

    LogMsg createLogMsg()
    {
        return LogMsg(LogLevel::value::INFO, "test.cpp", 123, "sink_test", "test message");
    }

    void printCurrentDirFiles()
    {
        std::cout << "当前目录文件列表：" << std::endl;
        for (const auto& entry : fs::directory_iterator("."))
        {
            if (entry.is_regular_file())
            {
                std::cout << "  " << entry.path().filename().string() << std::endl;
            }
        }
        std::cout << "-------------------" << std::endl;
    }
};

TEST_F(SinkTest, StdoutSinkLogsCorrectly)
{
    auto sink = SinkFactory::create<StdoutSink>();
    auto msg = createLogMsg();
    std::string output = fmt.format(msg);

    testing::internal::CaptureStdout();
    sink->log(output.c_str(), output.size());
    sink->flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::string captured = testing::internal::GetCapturedStdout();

    EXPECT_NE(captured.find("INFO"), std::string::npos);
    EXPECT_NE(captured.find("sink_test"), std::string::npos);
    EXPECT_NE(captured.find("test message"), std::string::npos);
    EXPECT_NE(captured.find("["), std::string::npos);
    EXPECT_NE(captured.find("]"), std::string::npos);
}

TEST_F(SinkTest, FileSinkWritesToFile)
{
    const std::string filename = "test_sink.log";

    if (fs::exists(filename))
        fs::remove(filename);

    printCurrentDirFiles();  // 写文件前打印

    auto sink = SinkFactory::create<FileSink>(filename);
    auto msg = createLogMsg();
    std::string output = fmt.format(msg);

    sink->log(output.c_str(), output.size());
    sink->flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    printCurrentDirFiles();  // 写文件后打印

    std::ifstream in(filename);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("INFO"), std::string::npos);
    EXPECT_NE(content.find("test message"), std::string::npos);

    in.close();
    fs::remove(filename);

    printCurrentDirFiles();  // 清理后打印
}

TEST_F(SinkTest, RollBySizeSinkRollsFiles)
{
    const size_t max_size = 100;
    const std::string basename = "rolltest";

    // 清理旧文件
    for (const auto& entry : fs::directory_iterator("."))
    {
        if (entry.is_regular_file())
        {
            std::string name = entry.path().filename().string();
            if (name.compare(0, basename.size(), basename) == 0)
            {
                fs::remove(entry);
            }
        }
    }

    printCurrentDirFiles();  // 清理后

    auto sink = SinkFactory::create<RollBySizeSink>(max_size, basename);
    auto msg = createLogMsg();

    for (int i = 0; i < 20; ++i)
    {
        std::string output = fmt.format(msg);
        sink->log(output.c_str(), output.size());
    }
    sink->flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    printCurrentDirFiles();  // 写日志后

    std::vector<fs::path> rolled_files;
    for (const auto& entry : fs::directory_iterator("."))
    {
        if (entry.is_regular_file())
        {
            std::string name = entry.path().filename().string();
            if (name.compare(0, basename.size(), basename) == 0 && name.size() >= 4 &&
                name.compare(name.size() - 4, 4, ".log") == 0)
            {
                rolled_files.push_back(entry.path());
            }
        }
    }

    EXPECT_GE(rolled_files.size(), 2);

    bool has_small_file = false;
    for (const auto& file : rolled_files)
    {
        if (fs::file_size(file) <= max_size)
        {
            has_small_file = true;
            break;
        }
    }
    EXPECT_TRUE(has_small_file);

    for (const auto& file : rolled_files)
    {
        fs::remove(file);
    }

    printCurrentDirFiles();  // 清理后
}

/*
    测试日志落地的课扩展性
    实现一个以时间进行文件滚动的落地方向进行测试
*/

namespace windlog {
/*
    主体思想, 以上一个时间好当前时间的间隔最为研究对象
    看他被时间间隔除以之后, 结果是否大于零
*/

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

    RollByTimeSink(TimeGap gap, std::string& basename)
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

TEST_F(SinkTest, RollByTimeSinkRollsFiles)
{
    using TimeGap = RollByTimeSink::TimeGap;
    const std::string basename = "rolltime";

    // 清理旧文件
    for (const auto& entry : fs::directory_iterator("."))
    {
        if (entry.is_regular_file())
        {
            std::string name = entry.path().filename().string();
            if (name.find(basename) == 0 && name.size() >= 4 &&
                name.compare(name.size() - 4, 4, ".log") == 0)
            {
                fs::remove(entry);
            }
        }
    }

    printCurrentDirFiles();  // 清理后

    std::string base_copy = basename;
    auto sink = SinkFactory::create<RollByTimeSink>(TimeGap::GAP_SECOND, base_copy);
    auto msg = createLogMsg();

    for (int i = 0; i < 3; ++i)
    {
        std::string output = fmt.format(msg);
        sink->log(output.c_str(), output.size());
        sink->flush();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    printCurrentDirFiles();  // 写日志后

    std::vector<fs::path> rolled_files;
    for (const auto& entry : fs::directory_iterator("."))
    {
        if (entry.is_regular_file())
        {
            std::string name = entry.path().filename().string();
            if (name.find(basename) == 0 && name.size() >= 4 &&
                name.compare(name.size() - 4, 4, ".log") == 0)
            {
                rolled_files.push_back(entry.path());
            }
        }
    }

    EXPECT_GE(rolled_files.size(), 2);

    for (const auto& file : rolled_files)
    {
        fs::remove(file);
    }

    printCurrentDirFiles();  // 清理后
}