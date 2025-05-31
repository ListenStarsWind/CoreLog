#pragma once

#include <sys/types.h>

#include <string>
#include <thread>

#include "level.hpp"
#include "util/time_util.hpp"

namespace windlog {
struct LogMsg
{
    time_t _ctime;
    LogLevel::value _level;
    std::string _file;
    size_t _line;
    std::thread::id _tid;
    std::string _logger;
    std::string _payload;

    LogMsg(LogLevel::value level, const std::string& file, size_t line, const std::string& logger,
           const std::string& payload)
        : _ctime(util::Date::now()),
          _level(level),
          _file(file),
          _line(line),
          _tid(std::this_thread::get_id()),
          _logger(logger),
          _payload(payload)
    {
    }
};
}  // namespace windlog