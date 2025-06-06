#pragma once

/*
    全局用户接口
    对日志组件的接口进行一定封装, 特别的, 让用户不用再写__FILE__, __LINE__
*/

#include "logger.hpp"

namespace windlog {
inline Logger::ptr getLogger(const std::string& name)
{
    return LoggerManager::getInstance().getLogger(name);
}

inline Logger::ptr rootLogger()
{
    return LoggerManager::getInstance().rootLogger();
}

#define DEBUG(fmt, ...) debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...) fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

// WI__LOG 负责依据日志等级, 使用指定日志器进行日志输出
#define WI__DEBUG(logger, fmt, ...) logger->DEBUG(fmt, ##__VA_ARGS__)
#define WI__INFO(logger, fmt, ...) logger->INFO(fmt, ##__VA_ARGS__)
#define WI__WARN(logger, fmt, ...) logger->WARN(fmt, ##__VA_ARGS__)
#define WI__ERROR(logger, fmt, ...) logger->ERROR(fmt, ##__VA_ARGS__)
#define WI__FATAL(logger, fmt, ...) logger->FATAL(fmt, ##__VA_ARGS__)

// LOG使用默认日志器进行输出
#define LOG__DEBUG(fmt, ...) WI__DEBUG(windlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOG__INFO(fmt, ...) WI__INFO(windlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOG__WARN(fmt, ...) WI__WARN(windlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOG__ERROR(fmt, ...) WI__ERROR(windlog::rootLogger(), fmt, ##__VA_ARGS__)
#define LOG__FATAL(fmt, ...) WI__FATAL(windlog::rootLogger(), fmt, ##__VA_ARGS__)

}  // namespace windlog