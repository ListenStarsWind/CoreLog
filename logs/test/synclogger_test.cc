#include <gtest/gtest.h>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <limits.h>
#include <iostream>
#include <filesystem>

#include "logger.hpp"

// 获取文件大小
static size_t getFileSize(const std::string& filename) {
    struct stat st {};
    if (stat(filename.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

// 获取当前工作目录
static std::string getCurrentDir() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return std::string(cwd);
    }
    return "";
}

// 确保目录存在，递归创建
static void ensureDirExists(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
}

// 输出滚动目录下所有文件的大小（辅助观察滚动效果）
static void printRollFilesSize(const std::string& roll_dir) {
    std::cout << "滚动目录文件大小列表：" << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(roll_dir)) {
        if (entry.is_regular_file()) {
            std::cout << "  " << entry.path().filename().string()
                      << ": " << getFileSize(entry.path().string()) << " 字节" << std::endl;
        }
    }
}

// 测试基类，统一准备路径和 logger 对象
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        cwd = getCurrentDir();

        // 定义日志文件路径和滚动目录
        file_path = cwd + "/logfile/test/log";
        roll_dir = cwd + "/logfile/roll";

        // 确保目录存在
        ensureDirExists(std::filesystem::path(file_path).parent_path().string());
        ensureDirExists(roll_dir);

        // 格式器
        fmt = std::make_shared<windlog::Formatter>("[%d{%H:%M:%S}][%c][%f:%l][%p]%T%m%n");

        // 创建两个日志sink
        file_sink = windlog::SinkFactory::create<windlog::FileSink>(file_path);
        roll_sink = windlog::SinkFactory::create<windlog::RollBySizeSink>(1024 * 1024, roll_dir);

        // 创建Logger，设定日志等级为 WARN，用于过滤测试
        logger = std::make_shared<windlog::SyncLogger>("root", windlog::LogLevel::value::WARN, fmt, std::vector{file_sink, roll_sink});
    }

    std::string cwd;
    std::string file_path;
    std::string roll_dir;
    windlog::Formatter::ptr fmt;
    windlog::LogSink::ptr file_sink;
    windlog::LogSink::ptr roll_sink;
    windlog::Logger::ptr logger;
};

// 测试日志等级过滤，只有 >= WARN 的日志会写入
TEST_F(LoggerTest, LevelFilterTest) {
    logger->debug(__FILE__, __LINE__, "%s", "debug 不应写入");
    logger->info(__FILE__, __LINE__, "%s", "info 不应写入");
    logger->warn(__FILE__, __LINE__, "%s", "warn 写入");
    logger->error(__FILE__, __LINE__, "%s", "error 写入");
    logger->fatal(__FILE__, __LINE__, "%s", "fatal 写入");

    logger->flush();  // 主动刷新，确保日志写入磁盘
    sleep(1);         // 等待写入完成

    size_t size = getFileSize(file_path);
    std::cout << "日志文件大小: " << size << " 字节" << std::endl;

    // 断言文件大小大于0，说明有日志写入
    EXPECT_GT(size, 0u);
}

// 测试日志文件大小限制和滚动
TEST_F(LoggerTest, LogFileSizeLimitTest) {
    const size_t target_size = 1024 * 1024 * 5; // 5MB 触发滚动阈值
    size_t count = 0;

    // 不断写入日志直到达到目标大小或写入次数达到限制
    while (getFileSize(file_path) < target_size && count < 100000) {
        std::string msg = "日志消息-" + std::to_string(count++);
        logger->fatal(__FILE__, __LINE__, "%s", msg.c_str());
    }

    logger->flush();
    sleep(1);

    size_t final_size = getFileSize(file_path);
    std::cout << "最终日志文件大小: " << final_size << " 字节" << std::endl;

    printRollFilesSize(roll_dir);

    // 断言最终文件大小达到目标值
    EXPECT_GE(final_size, target_size);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
