#include "windlog.hpp"
#include<chrono>

void bench(const std::string& logger_name, size_t thr_count, size_t msg_count, size_t msg_len,
           bool flag)
{
    // 获取日志器句柄
    windlog::Logger::ptr logger = windlog::getLogger(logger_name);
    if (logger.get() == nullptr)
    {
        std::cerr << "日志器不存在" << std::endl;
        return;
    }

    // 组织指定大小的单位日志消息
    // 还有一个换行, 由日志器建造时的格式化控制
    std::string mes(msg_len - 1, 'a');

    // 创建指定数目的日志
    std::vector<std::thread> threads;
    std::vector<double> costs(thr_count);

    LOG__INFO("测试日志 总条数: %zu, 总大小%zuKB", msg_count, msg_count * msg_len / 1024);

    // 负载均衡
    size_t msg_prt_thr = msg_count / thr_count;
    for(size_t i = 0; i < thr_count; ++i)
    {
        threads.emplace_back([&, i](){
            // 线程创建成功, 开始计时
            auto start = std::chrono::high_resolution_clock::now();

            // 执行任务
            for(int j = 0; j < msg_prt_thr; ++j)
            {
                WI__FATAL(logger, "%s", mes.c_str());
            }

            // 如果是同步, 要确保刷新到外设上
            if(flag == true)
                logger->flush();

            // 任务执行完毕, 统计耗时
            auto end= std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> cost = end - start;
            costs[i] = cost.count();

            LOG__INFO("线程%zu:\t输出日志数量:%zu条, 耗时:%lfs", i, msg_prt_thr, cost.count());
        });
    }

    for(int i = 0; i < thr_count; ++i)
    {
        threads[i].join();
    }

    // 计算相关指标


    // 线程是并发的, 我们选择其中耗费时间最长的那一个作为任务总耗时
    double max_cost = costs[0];
    for(int i = 1; i < thr_count; ++i)
        max_cost = std::max(max_cost, costs[i]);

    size_t msg_per_sec = msg_count / max_cost;
    size_t size_per_sec = msg_count * msg_len / max_cost;


    LOG__INFO("每秒输出日志条数: %zu条", msg_per_sec);
    LOG__INFO("每秒输出日志大小: %.2fMB", size_per_sec / 1024.0 / 1024.0);
}

// 同步测试
void sync_bench() {
    std::unique_ptr<windlog::LoggerBuilder> builder(new windlog::GlobalLoggerBuilder());
    builder->buildLoggerName("sync_logger");
    builder->buildLoggerFormatter("%m%n");
    builder->buildLoggerType(windlog::LoggerBuilder::LoggerType::LOGGER_SYNC);
    builder->buildLoggerSink<windlog::FileSink>("./logfile/sync.log");

    builder->build();

    bench("sync_logger", 3, 1000000, 100, true);
}

// 异步测试
void async_bench() {
    std::unique_ptr<windlog::LoggerBuilder> builder(new windlog::GlobalLoggerBuilder());
    builder->buildLoggerName("async_logger");
    builder->buildLoggerFormatter("%m%n");
    builder->buildLoggerType(windlog::LoggerBuilder::LoggerType::LOGGER_ASYNC);
    builder->buildLoggerMode(windlog::AsyncLooper::mode::ON_BUFFER_FULL_EXPAND);
    builder->buildLoggerSink<windlog::FileSink>("./logfile/sync.log");

    builder->build();

    bench("async_logger", 3, 1000000, 100, false);
}

int main()
{
    async_bench();
    return 0;
}