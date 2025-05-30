/*提供全局获取当前时间接口*/

#pragma once

#include <sys/types.h>
#include <time.h>

namespace windlog {
namespace util {
class Date
{
   public:
    static size_t now() { return static_cast<size_t>(time(nullptr)); }
};
}  // namespace util
}  // namespace windlog
