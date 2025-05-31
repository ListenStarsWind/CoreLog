//format_test.cc

/*
     单元测试, Formatter的接口正确性, 已知, struct LogMsg定义在message.hpp中:
     namespace windlog {
    struct LogMsg
    {
        .......
        LogMsg(LogLevel::value level, const std::string& file, size_t line, const std::string& logger,
            const std::string& payload)
          .....
        {
        }
    };

    class Formatter 的构造函数签名为 Formatter(const std::string& pattern = "[%p]%T[%d{%H:%M:%S}][%c][%t]%T[%f:%l]%T%m%n")
    支持这些格式

    %p              日志等级
    %T              制表符缩进
    %d{%H:%M:%S}    日期, 默认形式是时:分:秒, 也可以使用系统默认的其他格式, 使用的是strftime
    %c              日志器名称
    %t              线程ID
    %f              源代码文件名
    %l              源代码行号
    %m              主体信息
    %n              换行

    你可以使用 void format(std::ostream& out, const LogMsg& msg)或者std::string format(const LogMsg& msg)来查看输出
*/

#include <gtest/gtest.h>
#include "format.hpp"
#include "message.hpp"
#include "level.hpp"

using namespace windlog;

TEST(FormatterTest, FormatGeneratesExpectedOutput) {
    LogMsg msg(LogLevel::value::INFO, "main.cpp", 42, "core", "hello log");

    Formatter fmt("[%p] %c - %m");
    std::string output = fmt.format(msg);

    // 因为线程ID和时间变化大，这里只检查固定字段
    EXPECT_NE(output.find("[INFO]"), std::string::npos);
    EXPECT_NE(output.find("core"), std::string::npos);
    EXPECT_NE(output.find("hello log"), std::string::npos);
}
