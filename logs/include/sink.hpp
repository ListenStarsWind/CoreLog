/*
    日志落地模块的编写
    1. 抽象落地基类
    2. 依据不同的落地方向创建不同的具体子类
    3. 使用工厂模式进行创建与表示的分离
*/

#pragma once

#include <sys/types.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "util/file_util.hpp"
#include "util/time_util.hpp"

namespace windlog {
class LogSink
{
   public:
    using ptr = std::shared_ptr<LogSink>;
    virtual ~LogSink() = 0;
    virtual void log(const char* data, size_t len) = 0;
    virtual void flush() = 0;
};
inline LogSink::~LogSink() = default;

/*
    落地方向: 标准输出
*/
class StdoutSink : public LogSink
{
   public:
    ~StdoutSink() override = default;
    void log(const char* data, size_t len) override { std::cout.write(data, len); }
    void flush() override {std::cout.flush();}
};

/*
    落地方向: 指定文件
*/
class FileSink : public LogSink
{
   public:
    // 创建\打开指定文件
    FileSink(const std::string& pathname) : _filename(pathname)
    {
        // 创建文件所在路径
        util::file::createDirectory(util::file::path(_filename));
        // 创建/打开文件, 支持二进制数据
        _ofs.open(_filename, std::ios::binary | std::ios::app);
        // 检测是否打开
        if (!_ofs.is_open())
        {
            throw std::runtime_error("Failed to open file: " + _filename);
        }
    }
    ~FileSink() override = default;
    void log(const char* data, size_t len) override
    {
        _ofs.write(data, len);
        if (!_ofs)
        {
            // 写入失败，输出错误信息或抛异常
            std::cerr << "FileSink write failed to: " << _filename << std::endl;
            // 或者记录错误状态，防止继续写
            // 或者抛出异常终止程序
            throw std::runtime_error("Failed to write to log file: " + _filename);
        }
    }
    void flush() override {_ofs.flush();}

   private:
    std::ofstream _ofs;
    std::string _filename;
};

/*
    落地方向: 滚动文件(以大小进行滚动)
    同样是为了效率, 文件构造函数就打开
    而不是每次log再打开, 同时, 为了提高
    效率, 将自己维护一个文件的内容大小
    成员变量, 因为调用系统接口效率较低
*/
class RollBySizeSink : public LogSink
{
   private:
    // 生成一个日志文件名
    // 基础名 + 日期 + 编号
    std::string generateLogFilename()
    {
        // 获取当前时间
        time_t ct = util::Date::now();
        struct tm t;
        memset(&t, 0, sizeof(t));
        localtime_r(&ct, &t);
        std::stringstream filename;
        filename << _basename << '-' << t.tm_year + 1900 << t.tm_mon + 1 << t.tm_mday << '-'
                 << t.tm_hour << t.tm_min << t.tm_sec << _cur_fsize++ << ".log";

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
    RollBySizeSink(size_t max_fsize, const std::string& basename)
        : _max_fsize(max_fsize),
          _cur_fsize(0),
          _file_idx(0),
          _basename(basename),
          _cur_fname(generateLogFilename())
    {
        // 创建文件所在路径
        util::file::createDirectory(util::file::path(_cur_fname));
        // 打开文件
        openfile(_cur_fname);
    }
    ~RollBySizeSink() override = default;
    void log(const char* data, size_t len) override
    {
        if (_cur_fsize + len > _max_fsize) {
            _ofs.close();
            _file_idx++;
            _cur_fname = generateLogFilename();
            openfile(_cur_fname);
            _cur_fsize = 0;
        }

        _cur_fsize += len;
        _ofs.write(data, len);
        if (!_ofs)
        {
            std::cerr << "FileSink write failed to: " << _cur_fname << std::endl;
            throw std::runtime_error("Failed to write to log file: " + _cur_fname);
        }
    }

    void flush() override {_ofs.flush();}

   private:
    size_t _max_fsize;
    size_t _cur_fsize;
    size_t _file_idx;
    std::string _basename;  // 文件的基础名, 将在后面进行拼接时间编号作为日志的具体名
    std::string _cur_fname;
    std::ofstream _ofs;
};

class SinkFactory
{
   public:
    template <typename SinkType, typename... Args>
    static LogSink::ptr create(Args&&... args)
    {
        return std::make_shared<SinkType>(std::forward<Args>(args)...);
    }
};

}  // namespace windlog
