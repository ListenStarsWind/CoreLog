/*
    1. 判断文件是否存在
    2. 获取文件所在路径
    3. 创建目录结构
*/

#pragma once

#include <sys/stat.h>

#include <string>

namespace windlog {
namespace util {
class file
{
   public:
    static bool exists(const std::string& pathname)
    {
        /*
            方案一, 使用int access(const char *pathname, int mode); <unistd.h>
            access核心功能是判断用户对文件是否具有权限,但也可以用于判断文件是否存在
            只要将第二个参数设置成F_OK, 便能判断文件是否存在, 文件存在返回零, 否则-1

            缺点, windows不支持该接口, 更多人喜欢用 stat
        */

        /*
            return (access(pathname.c_str(), F_OK) == 0);
        */

        /*
            方案二, 使用int stat(const char *restrict pathname,
                struct stat *restrict statbuf); <sys/stat.h>

            用于获取文件状态, 当返回值小于零时, 即说明获取失败, 文件不存在
        */
        struct stat st;
        if (stat(pathname.c_str(), &st) < 0)
            return false;
        else
            return true;
    }

    static std::string path(const std::string& pathname)
    {
        size_t pos = pathname.find_last_of("/\\");
        if (pos == std::string::npos)
            return ".";
        else
            return pathname.substr(0, pos);
    }

    static void createDirectory(const std::string& pathname)
    {
        /*
            我们先要确保目录存在, 之后再去考虑文件本身的创建
            例如对于./abc/bc/a.txt, 我们要以循环的方式, 分别
            将./abc, .abc/bc这两个目录创建出来

            不过注意的是,这里的pathname是经过path()接口过滤过的
            所以只会剩下纯路径, 没有文件

            注意, 对于windows来说, 路径分隔符是'\', 并且要加
            转义字符`\`转义

            int mkdir(const char *pathname, mode_t mode);<sys/stat.h>
            创建目录, mode一般设计成0777, 0是文件类型, 777表示对所有人可读可写
        */
        size_t pos = 0;
        while (pos < pathname.size())
        {
            std::string parent;
            pos = pathname.find_first_of("/\\", pos);
            if (pos == std::string::npos)
            {
                parent = pathname.substr(0);
            }
            else
            {
                parent = pathname.substr(0, pos + 1);
                ++pos;
            }
            if (exists(parent) == false)
            {
                mkdir(parent.c_str(), 0777);
            }
        }
    }
};
}  // namespace util
}  // namespace windlog
