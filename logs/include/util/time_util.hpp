/*提供全局获取当前时间接口*/

#pragma once

#include <sys/types.h>
#include <time.h>

namespace windlog {
namespace util {
class Date
{
   public:
    static time_t now() { return time(nullptr);}
};
}  // namespace util
}  // namespace windlog
