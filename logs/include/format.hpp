/*
    格式化类簇, 核心功能, 依据用户的格式化字符串构造出一条日志字符串
    利用工厂模式让各个子项与对应的占位符建立映射关系
    格式化子数组确定构造顺序
*/

#pragma once

#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "level.hpp"
#include "message.hpp"

namespace windlog {

/*
    抽象格式化基类
    对于基类析构函数, 为了确保派生重写, 必须使用虚函数
    但析构由于是自动调用, 所以又不能纯虚, 所以外面还要
    再定义一个定义, 这里加inline是为了防止多重定义, 这
    是inline的另一个用法, 当别的文件中也出现同种函数定
    义时, 如果它们相同, 会被编译器合并, 不相同, 编译器
    报错
*/

class FormatItem
{
   public:
    using ptr = std::shared_ptr<FormatItem>;
    virtual ~FormatItem() = 0;
    virtual void format(std::ostream& out, const LogMsg& msg) = 0;
};
inline FormatItem::~FormatItem() = default;

class MsgFormatItem : public FormatItem
{
   public:
    ~MsgFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override { out << msg._payload; }
};

class LevelFormatItem : public FormatItem
{
   public:
    ~LevelFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override
    {
        out << LogLevel::toString(msg._level);
    }
};

/*
    日期格式化子类存在子格式, 构造时需要添加额外信息进行辅助
    struct tm *localtime_r(const time_t *restrict timep,
                           struct tm *restrict result);<ctime>
    可以将时间戳转化成包含年月日时分秒, 微米纳秒的时间结构

    size_t strftime(char s[restrict .max], size_t max,
                       const char *restrict format,
                       const struct tm *restrict tm);<ctime>
    可以依据时间结构和格式化字符串输出一个对应的日期字符串
    它支持各种占位符, 其中就包括我们默认的%H, %M, %S
    具体可以查找手册进行了解
*/
class DateFormatItem : public FormatItem
{
   public:
    ~DateFormatItem() override = default;
    DateFormatItem(const std::string& time_fmt = "%H:%M:%S") : _time_fmt(time_fmt) {}

    void format(std::ostream& out, const LogMsg& msg) override
    {
        struct tm t;
        localtime_r(&msg._ctime, &t);
        char buff[64] = {0};
        strftime(buff, sizeof(buff) - 1, _time_fmt.c_str(), &t);  // 要为'\0'预留一个位置
        out << buff;
    }

   private:
    std::string _time_fmt;
};

class FileFormatItem : public FormatItem
{
   public:
    ~FileFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override { out << msg._file; }
};

class LineFormatItem : public FormatItem
{
   public:
    ~LineFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override { out << msg._line; }
};

class ThreadFormatItem : public FormatItem
{
   public:
    ~ThreadFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override { out << msg._tid; }
};

class LoggerFormatItem : public FormatItem
{
   public:
    ~LoggerFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override { out << msg._logger; }
};

class TabFormatItem : public FormatItem
{
   public:
    ~TabFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override { out << '\t'; }
};

class NLineFormatItem : public FormatItem
{
   public:
    ~NLineFormatItem() override = default;
    void format(std::ostream& out, const LogMsg& msg) override { out << '\n'; }
};

/*
    OtherFormatItem即使用原始字符串数据
*/
class OtherFormatItem : public FormatItem
{
   public:
    ~OtherFormatItem() override = default;
    OtherFormatItem(const std::string& str) : _str(str) {}

    void format(std::ostream& out, const LogMsg& msg) override { out << _str; }

   private:
    std::string _str;
};

/*
    %p              日志等级
    %T              制表符缩进
    %d{%H:%M:%S}    日期, 默认形式是时:分:秒
    %c              日志器名称
    %t              线程ID
    %f              源代码文件名
    %l              源代码行号
    %m              主体信息
    %n              换行
*/

class Formatter
{
   private:
    /*
        根据不同的占位符标志, 创建对应的格式化子类
    */

    FormatItem::ptr createItem(const std::string& key, const std::string& val)
    {
        if (key == "p")
            return std::make_shared<LevelFormatItem>();
        if (key == "T")
            return std::make_shared<TabFormatItem>();
        if (key == "d")
            return std::make_shared<DateFormatItem>(val);
        if (key == "c")
            return std::make_shared<LoggerFormatItem>();
        if (key == "t")
            return std::make_shared<ThreadFormatItem>();
        if (key == "f")
            return std::make_shared<FileFormatItem>();
        if (key == "l")
            return std::make_shared<LineFormatItem>();
        if (key == "m")
            return std::make_shared<MsgFormatItem>();
        if (key == "n")
            return std::make_shared<NLineFormatItem>();

        return std::make_shared<OtherFormatItem>(val);
    }

    /*
        对格式化规则字符串进行解析
        解析失败时返回false 并且_items为空
    */
    bool parsePattern()
    {
        std::vector<std::pair<std::string, std::string>> fmt_buff;

        size_t len = _pattern.size();

        std::string key, val;
        size_t start = 0, end = 0;
        while (start < len)
        {
            if (_pattern[start] != '%')
            {
                while (end < len && _pattern[end] != '%') ++end;
                key = "TEXT";
                val = _pattern.substr(start, end - start);
                fmt_buff.emplace_back(key, val);
                start = end;
            }
            else
            {
                if (start + 1 == len)
                    return false;
                if (_pattern[start + 1] == '%')
                {
                    key = "TEXT";
                    val = "%";
                    fmt_buff.emplace_back(key, val);
                    start += 2;
                    end = start;
                }
                else
                {
                    size_t pos = _formatKeys.find(_pattern[start + 1]);
                    if (pos == std::string::npos)
                        return false;
                    key = _pattern[start + 1];
                    val.clear();

                    start += 2;
                    end = start;
                    if (start < len && _pattern[start] == '{')
                    {
                        ++start;
                        end = start;
                        while (end < len && _pattern[end] != '}') ++end;
                        if (end == len)
                            return false;
                        val = _pattern.substr(start, end - start);
                        start = ++end;
                    }
                    fmt_buff.emplace_back(key, val);
                }
            }
        }

        _items.reserve(fmt_buff.size());
        for (const auto& [key, val] : fmt_buff)
        {
            auto item = createItem(key, val);
            if (!item)
            {
                _items.clear();
                return false;
            }
            _items.emplace_back(std::move(item));
        }

        return true;
    }

   public:
   using ptr = std::shared_ptr<Formatter>;
    Formatter(const std::string& pattern = "[%p]%T[%d{%H:%M:%S}][%c][%t]%T[%f:%l]%T%m%n")
        : _pattern(pattern), _formatKeys("pTdctflmn")
    {
        parsePattern();
    }

    /*
        对msg进行格式化
    */
    void format(std::ostream& out, const LogMsg& msg)
    {
        // 格式化规则字符串解析失败则舍弃日志
        if (!_items.empty())
        {
            for (auto item : _items)
            {
                item->format(out, msg);
            }
        }
        else
        {
            std::cerr<<"Log formatting failed in parsePattern()"<<std::endl;
        }
    }
    std::string format(const LogMsg& msg)
    {
        std::stringstream ss;
        format(ss, msg);
        return ss.str();
    }

   private:
    std::string _pattern;           // 用户输入的原始格式化字符串
    const std::string _formatKeys;  // 格式化检查辅助字符串
    std::vector<FormatItem::ptr> _items;
};

}  // namespace windlog
