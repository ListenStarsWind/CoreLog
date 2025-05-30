#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdio> // for std::remove

#include "util/time_util.hpp"
#include "util/file_util.hpp"

namespace util = windlog::util;
namespace fs = std::filesystem;

TEST(TimeUtilTest, NowReturnsNonZeroTimestamp) {
    auto ts = util::Date::now();
    EXPECT_GT(ts, 0);
}

TEST(FileUtilTest, ExistsDetectsFilePresence) {
    const std::string file = "temp_file.log";

    // 确保文件不存在
    std::remove(file.c_str());
    EXPECT_FALSE(util::file::exists(file));

    // 创建文件
    std::ofstream out(file);
    out << "log test\n";
    out.close();

    EXPECT_TRUE(util::file::exists(file));

    std::remove(file.c_str());
}

TEST(FileUtilTest, PathReturnsParentDirectory) {
    EXPECT_EQ(util::file::path("/var/log/app.log"), "/var/log");
    EXPECT_EQ(util::file::path("relative/file.txt"), "relative");
    EXPECT_EQ(util::file::path("justfile"), ".");
    EXPECT_EQ(util::file::path("/tmp/"), "/tmp");
}

TEST(FileUtilTest, CreateDirectoryMakesNestedDirs) {
    std::string dir = "tmp/test/dir";

    // 先清理旧目录
    fs::remove_all("tmp");
    EXPECT_FALSE(fs::exists(dir));

    // 调用 createDirectory（返回 void）
    util::file::createDirectory(dir);

    // 用 filesystem 判断目录是否创建成功
    EXPECT_TRUE(fs::exists(dir));
    EXPECT_TRUE(fs::is_directory(dir));

    // 再次调用（应幂等，无异常）
    util::file::createDirectory(dir);

    // 清理
    fs::remove_all("tmp");
}

