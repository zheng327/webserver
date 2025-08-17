#pragma once
#include <FixedBuffer.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

class AsynLogging
{
public:
    AsynLogging(const std::string &basename, off_t rollSize,
                int flushInterval = 3);
    ~AsynLogging()
    {
        if (running_)
        {
            stop();
        }
    }
    // 前端调用append写入日志
    void append(const char *logline, int len);

    void start()
    {
        running_ = true;
        thread_.start();
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
    }

private:
    using LargeBuffer = FixedBuffer<kLargeBufferSize>;
    using BufferVector = std::vector<std::unique_ptr<LargeBuffer>>;
    // BufferVector::value_type 是 std::vector<std::unique_ptr<Buffer>>
    // 的元素类型，也就是 std::unique_ptr<Buffer>。
    using BufferPtr = BufferVector::value_type;
    void threadFunc();
    const int flushInterval_; // 日志刷新时间
    std::atomic<bool> running_;
    const std::string basename_;
    off_t rollSize_;
    bool running_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};