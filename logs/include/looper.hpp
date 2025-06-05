/*
    实现异步工作模块
*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "buffer.hpp"

namespace windlog {
class AsyncLooper
{
    // 异步线程入口函数
    void thredEntry()
    {
        // 在回调函数之上增加线程安全逻辑
        while (!_stop)
        {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                // 生产者无数据则进入休眠状态, 或者退出标记位被设置
                _cond_con.wait(lock, [&]() { return _stop || (!_produc_buff.empty()); });

                // 交换数据
                _consum_buff.swap(_produc_buff);

                // 唤醒生产者(可能因为满了而阻塞, 没阻塞也没影响)
                _cond_pro.notify_one();
            }

            // 日志数据落地
            _call_back(_consum_buff);

            // 复位等待交换
            _consum_buff.reset();
        }
    }

   public:
    // 缓冲区满时的处理策略
    enum class mode
    {
        ON_BUFFER_FULL_BLOCK,  // 当缓冲区满时阻塞, 直至其重新可写
        ON_BUFFER_FULL_EXPAND  // 当缓冲区满时扩容, 有资源过量风险
    };

    using ptr = std::shared_ptr<AsyncLooper>;
    using Functor = std::function<void(Buffer&)>;

    // 默认使用 ON_BUFFER_FULL_BLOCK
    AsyncLooper(const Functor& call_back, mode strategy)
        : _stop(false),
          _strategy(strategy),
          _call_back(call_back),
          _thread(&AsyncLooper::thredEntry, this)
    {
    }
    ~AsyncLooper() { stop(); }

    // 停止异步线程工作
    void stop()
    {
        // 标记位设置
        _stop = true;
        // 唤醒所有异步线程, 令其退出
        _cond_con.notify_all();
        // 与异步线程会和
        _thread.join();
    }

    void push(const char* data, size_t len)
    {
        /*
            存在两种方式,
            一是在压力测试环节数据空间不够主动扩容
            二是在平常业务处理时, 为了避免缓冲区
            空间过大, 资源占用过大, 允许业务线程
            进行一定程度上的轻度阻塞
            所以之前说, buff是由外部控制阻塞行为的
        */

        // 1. 加锁, 确保异步线程不干扰输入过程
        //          也可以确保外界logger的线程安全
        std::unique_lock<std::mutex> lock(_mutex);

        if (_strategy == mode::ON_BUFFER_FULL_BLOCK)
        {
            // 缓冲区满了进入阻塞队列, 在异步线程结束工作后将其唤醒
            _cond_pro.wait(lock, [&]() { return _produc_buff.writeAbleSize() >= len; });
        }

        // 缓冲区现在可以输入数据
        _produc_buff.push(data, len);

        // 唤醒一个消费者进行数据处理
        _cond_con.notify_one();

        auto c = !_produc_buff.empty();
    }

   private:
    // 判断异步线程是否继续的标记位
    // 临界资源, 使用原子化操作
    std::atomic<bool> _stop;
    mode _strategy;

    std::mutex _mutex;
    const Functor _call_back;

    Buffer _produc_buff;  // 生产缓冲区
    Buffer _consum_buff;  // 消费缓冲区

    std::condition_variable _cond_pro;
    std::condition_variable _cond_con;

    std::thread _thread;
};
}  // namespace windlog