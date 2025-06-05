#define CONFIG_BUFFER_SIZE (1024 * 1024)

#include "buffer.hpp"

#include <gtest/gtest.h>

#include <fstream>

TEST(BufferTest, ReadWriteCorrectness)
{
    // 打开原始文件
    std::ifstream ifs("./logfile/test/log", std::ios::binary);
    ASSERT_TRUE(ifs.is_open()) << "无法打开原始日志文件";

    // 获取文件大小并读入内容
    ifs.seekg(0, std::ios::end);
    size_t fsize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<char> body(fsize);
    ifs.read(&body[0], fsize);
    ASSERT_TRUE(ifs.good()) << "读取原始日志内容失败";
    ifs.close();

    // 写入 buffer
    windlog::Buffer buffer;
    buffer.reserve(fsize);
    for (size_t i = 0; i < body.size(); ++i)
    {
        buffer.push(&body[i], 1);
    }

    // 输出为临时文件
    const char* temp_file = "./logfile/test/log.tmp";
    std::ofstream ofs(temp_file, std::ios::binary);
    ASSERT_TRUE(ofs.is_open()) << "无法创建临时文件";

    size_t size = buffer.readAbleSize();
    for (size_t i = 0; i < size; ++i)
    {
        ofs.write(buffer.readAbleBegin(), 1);
        buffer.moveReader(1);
    }
    ofs.close();

    // 再次读取两个文件，比较它们的内容是否一致
    std::ifstream ifs1("./logfile/test/log", std::ios::binary);
    std::ifstream ifs2(temp_file, std::ios::binary);
    ASSERT_TRUE(ifs1.is_open() && ifs2.is_open()) << "打开文件比较失败";

    std::vector<char> origin((std::istreambuf_iterator<char>(ifs1)),
                             std::istreambuf_iterator<char>());
    std::vector<char> copy((std::istreambuf_iterator<char>(ifs2)),
                           std::istreambuf_iterator<char>());
    ASSERT_EQ(origin, copy) << "缓冲区前后数据不一致";

    ifs1.close();
    ifs2.close();
}