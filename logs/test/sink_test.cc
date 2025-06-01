#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>
#include "format.hpp"
#include "sink.hpp"
#include "message.hpp"
#include "level.hpp"

namespace fs = std::filesystem;
using namespace windlog;

class SinkTest : public ::testing::Test {
protected:
    Formatter fmt{"[%p] %T[%d{%H:%M:%S}][%c][%t]%T[%f:%l]%T%m%n"};

    LogMsg createLogMsg() {
        return LogMsg(LogLevel::value::INFO, "test.cpp", 123, "sink_test", "test message");
    }
};

TEST_F(SinkTest, StdoutSinkLogsCorrectly) {
    auto sink = SinkFactory::create<StdoutSink>();
    auto msg = createLogMsg();
    std::string output = fmt.format(msg);

    testing::internal::CaptureStdout();
    sink->log(output.c_str(), output.size());
    sink->flush();  // 如果 StdoutSink 实现了 flush，这样保证输出刷新
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 等待输出刷新

    std::string captured = testing::internal::GetCapturedStdout();

    EXPECT_NE(captured.find("INFO"), std::string::npos);
    EXPECT_NE(captured.find("sink_test"), std::string::npos);
    EXPECT_NE(captured.find("test message"), std::string::npos);
    // 线程ID和时间简单检查
    EXPECT_NE(captured.find("["), std::string::npos);
    EXPECT_NE(captured.find("]"), std::string::npos);
}

TEST_F(SinkTest, FileSinkWritesToFile) {
    const std::string filename = "test_sink.log";
    if (fs::exists(filename)) fs::remove(filename);

    auto sink = SinkFactory::create<FileSink>(filename);
    auto msg = createLogMsg();
    std::string output = fmt.format(msg);

    sink->log(output.c_str(), output.size());
    sink->flush();  // 强制刷新缓冲区，确保写入磁盘
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 等待磁盘写入完成

    std::ifstream in(filename);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("INFO"), std::string::npos);
    EXPECT_NE(content.find("test message"), std::string::npos);

    in.close();
    fs::remove(filename);
}

TEST_F(SinkTest, RollBySizeSinkRollsFiles) {
    const size_t max_size = 100; // 100字节滚动
    const std::string basename = "rolltest";

    // 清理历史文件
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string name = entry.path().filename().string();
            if (name.compare(0, basename.size(), basename) == 0) {
                fs::remove(entry);
            }
        }
    }

    auto sink = SinkFactory::create<RollBySizeSink>(max_size, basename);
    auto msg = createLogMsg();

    for (int i = 0; i < 20; ++i) {
        std::string output = fmt.format(msg);
        sink->log(output.c_str(), output.size());
    }
    sink->flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 检查至少有两个 rolltest*.log 文件生成
    std::vector<fs::path> rolled_files;
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string name = entry.path().filename().string();
            if (name.compare(0, basename.size(), basename) == 0 &&
                name.size() >= 4 &&
                name.compare(name.size() - 4, 4, ".log") == 0) {
                rolled_files.push_back(entry.path());
            }
        }
    }

    EXPECT_GE(rolled_files.size(), 2); // 至少生成两个文件

    // 检查至少一个文件大小 <= max_size
    bool has_small_file = false;
    for (const auto& file : rolled_files) {
        if (fs::file_size(file) <= max_size) {
            has_small_file = true;
            break;
        }
    }
    EXPECT_TRUE(has_small_file);

    // 清理
    for (const auto& file : rolled_files) {
        fs::remove(file);
    }
}
