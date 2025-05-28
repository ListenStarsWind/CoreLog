// /*学习不定参宏函数的使用*/

// #include<stdio.h>

// #define LOG(fmt, ...) printf("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

// int main()
// {
//     LOG("可怜的小猪-458\n");
//     return 0;
// }


#define _GNU_SOURCE
#include<unistd.h>
#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>

// 仍是使用...表示不定参数
void printNum(int count, ...)
{
    va_list ap;
    va_start(ap, count);                                    // 参数列表初始化
    for(int i = 0; i < count; ++i)
    {
        int num = va_arg(ap, int);
        printf("param[%d]:%d\n", i + 1, num);       // 自动返回相应类型和向后迭代
    }
    printf("\n");
    va_end(ap);                                            // va_end将列表无效化
}

void my_printf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    char* buff = NULL;
    int res = vasprintf(&buff, format, ap);

    if(res > 0)
    {
        // 成功返回字符串长度
        write(0, buff, res);
        free(buff);
    }

   va_end(ap);
}

#define LOG(fmt, ...) my_printf("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

int main()
{
    LOG("%s-%d\n", "祖玛游戏", 488);
    LOG("可怜的小猪-458\n");
    return 0;
}
