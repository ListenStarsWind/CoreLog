/*实现异步缓冲区*/

#pragma once

#define DEFAULT_BUFFER_SIZE (10 * 1024 * 1024)

#ifdef CONFIG_BUFFER_SIZE
#define BUFFER_SIZE CONFIG_BUFFER_SIZE
#else
#define BUFFER_SIZE DEFAULT_BUFFER_SIZE
#endif

#define THRESHOULD_BUFFER_SIZE (8 * BUFFER_SIZE)
#define INCREMENT_BUFFER_SIZE (BUFFER_SIZE)

#include <algorithm>
#include <cassert>
#include <vector>

namespace windlog {

// 用户可通过定义 CONFIG_BUFFER_SIZE 来覆盖默认缓冲区大小
class Buffer
{
   public:
    Buffer() : _base(BUFFER_SIZE), _reader_idx(0), _writer_idx(0) {}
    ~Buffer() = default;

    // 从指定位置开始, 写入指定长度数据
    void push(const char* data, size_t len)
    {
        // 当缓冲区的剩余大小不够存储指定长度的数据 有两种做法, 一是扩容, 二是阻塞, 返回错误
        // 我们这里选择扩容,有两个方面的考虑, 一是在进行单元测试时, 为了测试极限情况,
        // 要往缓冲区上写入大量数据 不扩容没法做, 尽管对于日志来说,
        // 我们的默认大小应该不会让这种情况发生, 扩容的时间消耗还是挺大的, 应该 避免扩容, 另一方面,
        // 阻塞错误会增加业务逻辑的复杂度, 当初就是不想阻塞才异步的

        // 1. 确保空间足够
        reserve(len);

        // 2. 将数据拷贝到缓冲区
        // vector::data返回的是常指针
        std::copy(data, data + len, &_base[_writer_idx]);

        // 3. 移动写指针
        moveWriter(len);
    }

    // 获取可读数据的起始地址
    const char* readAbleBegin() { return &_base[_reader_idx]; }

    // 获取可读数据的长度
    size_t readAbleSize() { return _writer_idx - _reader_idx; }

    // 返回可写数据的长度
    // 辅助实现外部主动阻塞
    size_t writeAbleSize()
    {
        // 对于扩容方案来说, 严格意义上并不存在
        // 剩余大小这个概念, 毕竟可以扩容该接口
        // 的主要功能是让外部意识到剩余空间不够,
        //  主动放弃写操作从而避免扩容

        return _base.size() - _writer_idx;
    }

    // 预留至少指定长度的可写位置
    void reserve(size_t len)
    {
        size_t rema = writeAbleSize();
        size_t new_size = _base.size();

        while (len > rema)
        {
            // 到达阈值之前, 成倍增长
            // 到达之后, 线性增长
            if (new_size < THRESHOULD_BUFFER_SIZE)
            {
                rema += new_size;
                new_size *= 2;
            }
            else
            {
                rema += INCREMENT_BUFFER_SIZE;
                new_size += INCREMENT_BUFFER_SIZE;
            }
        }

        _base.resize(new_size);
    }

    // 移动读指针地址
    void moveReader(size_t len)
    {
        // 不可越过写指针
        assert(len <= readAbleSize());
        _reader_idx += len;
    }

    // 移动写指针地址
    void moveWriter(size_t len)
    {
        assert(len <= writeAbleSize());
        _writer_idx += len;
    }

    // 重置读写位置, 为交换进行准备
    void reset()
    {
        _reader_idx = 0;
        _writer_idx = 0;
    }

    void swap(Buffer& buffer)
    {
        _base.swap(buffer._base);
        std::swap(_reader_idx, buffer._reader_idx);
        std::swap(_writer_idx, buffer._writer_idx);
    }

    // 判断缓冲区是否为空
    bool empty() { return _reader_idx == _writer_idx; }

   private:
    std::vector<char> _base;  // 基础底层容器
    size_t _reader_idx;       // 指向下一个可读位置的指针
    size_t _writer_idx;       // 指向下一个可写位置的指针
};
}  // namespace windlog