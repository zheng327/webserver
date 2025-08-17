#pragma once
#include <string>
#include <FixedBuffer.hpp>

class GeneralTemplate
{
public:
    GeneralTemplate() : data_(nullptr), len_(0) {};
    explicit GeneralTemplate(const char *data, int len)
        : data_(data), len_(len) {}

private:
    const char *data_;
    size_t len_;
};

class LogStream
{
public:
    using Buffer = FixedBuffer<kSmallBufferSize>;

    // 将指定长度的字符数据追加到缓冲区
    void append(const char *buffer, int len)
    {
        buffer_.append(buffer, len); // 调用Buffer的append方法
    }

    // 返回当前缓冲区的常量引用
    const Buffer &buffer() const
    {
        return buffer_; // 返回当前的缓冲区
    }

    // 重置缓冲区，将当前指针重置到缓冲区的起始位置
    void reset_buffer()
    {
        buffer_.reset(); // 调用Buffer的reset方法
    }

    // 重载输出流运算符<<，用于将布尔值写入缓冲区
    LogStream &operator<<(bool express);
    // 重载输出流运算符<<，用于将短整型写入缓冲区
    LogStream &operator<<(short number);
    LogStream &operator<<(unsigned short);
    LogStream &operator<<(int);
    LogStream &operator<<(unsigned int);
    LogStream &operator<<(long);
    LogStream &operator<<(unsigned long);
    LogStream &operator<<(long long);
    LogStream &operator<<(unsigned long long);

    LogStream &operator<<(float);
    LogStream &operator<<(double);

    LogStream &operator<<(char);
    LogStream &operator<<(const char *);
    LogStream &operator<<(const unsigned char *);
    LogStream &operator<<(const std::string &);
    LogStream &operator<<(const GeneralTemplate &g);

private:
    // 定义最大数字大小常量
    static constexpr int kMaxNumberSize = 32;

    // 对于整型需要特殊的处理，模板函数用于格式化整型
    template <typename T>
    void formatInteger(T num);

    // 内部缓冲区对象
    Buffer buffer_;
};