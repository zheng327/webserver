#include <LogStream.hpp>
#include <alogrithm>

static const char digits[] = "9876543210123456789";

template <typename T>
void LogStream::formatInteger(T num)
{
    if (buffer_.avail() >= kMaxNumberSize)
    {
        char *start = buffer_.current();
        char *cur = start;
        static const char *zero = digits + 9;
        bool negative = (num < 0); // 判断num是否是负数
        do
        {
            int remainder = static_cast<int>(num % 10);
            (*cur++) = zero[remainder];
            num /= 10;
        } while (num != 0);
        if (negative)
        {
            *cur++ = '-';
        }
        *cur = '\0';
        std::reverse(start, cur);
        int length = static_cast<int>(cur - start);
        buffer_.add(length);
    }
}

LogStream &LogStream::operator<<(bool express)
{
    buffer_.append(express ? "true" : "false", express ? 4 : 5);
    return *this;
}

LogStream &LogStream::operator<<(short num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(unsigned short num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(int num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(long num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(long long num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long long num)
{
    formatInteger(num);
    return *this;
}

LogStream &LogStream::operator<<(float num)
{
    *this << static_cast<double>(num);
    return *this;
}

LogStream &LogStream::operator<<(double num)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.12g", num);
    buffer_.append(buf, strlen(buf));
    return *this;
}

LogStream &LogStream::operator<<(char c)
{
    buffer_.append(&c, 1);
    return *this;
}

LogStream &LogStream::operator<<(const char *str)
{
    buffer_.append(str, strlen(str));
    return *this;
}

LogStream &LogStream::operator<<(const unsigned char *str)
{
    buffer_.append(reinterpret_cast<const char *>(str), strlen(reinterpret_cast<const char *>(str)));
    return *this;
}

LogStream &LogStream::operator<<(const std::string &str)
{
    buffer_.append(str.c_str(), str.size());
    return *this;
}

LogStream &LogStream::operator<<(const GeneralTemplate &g)
{
    buffer_.append(g.data_, g.len_);
    return *this;
}