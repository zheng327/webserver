#pragma once
#include <string>

class AsynLogging;
constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000 * 1000;

// 固定的缓冲区类，用于管理日志数据的存储
// 该类提供了一个固定大小的缓冲区，允许将数据追加到缓冲区中，并提供相关的操作方法
template <int buffer_size>
class FixedBuffer
{
public:
    FixedBuffer() : cur_(data_), size_(0) {}
    // 将指定长度的数据追加到缓冲区
    void append(const char *buf, size_t len)
    {
        if (avail() > len)
        {
            memcpy(cur_, buf, len);
            add(len);
        }
    }
    // 返回缓冲区的起始地址
    const char *data() const { return data_; }
    // 返回缓冲区中当前有效数据的长度
    int length() const { return size_; }
    // 返回当前指针的位置
    char *current() { return cur_; }
    // 返回缓冲区中剩余可用空间的大小 缓冲区大小-当前数据的长度
    size_t avail() const { return static_cast<size_t>(buffer_size - size_); }
    // 更新当前指针和当前缓冲区中的数据长度
    void add(size_t len)
    {
        cur_ += len;
        size_ += len;
    }
    // 重置当前指针，回到缓冲区的起始位置
    void reset()
    {
        cur = data_;
        size_ = 0;
    }
    // 清空缓冲区中的数据
    void bzero()
    {
        ::bezero(data_, sizeof(data_));
    }
    // 将缓冲区中的数据转换成std：：string并返回
    std::string toString() const
    {
        return std::string(data_, length());
    }

private:
    char data_[buffer_size]; // 定义固定大小的缓冲区
    char *cur;               // 当前指针，指向缓冲区下一个可写入的位置
    int size_;               // 缓冲区的大小
};