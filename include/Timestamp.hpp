#ifndef _TIMESTAMP_HPP
#define _TIMESTAMP_HPP

#include <string>
#include <sys/time.h>

class Timestamp
{
public:
    Timestamp() : microSecondsSinceEpoch_(0) {}
    explicit Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

    // 获取当前时间辍
    static Timestamp now();
    std::string toString() const;

    // 格式
    std::string toFormattedString(bool showMicroseconds = false) const;
    // 返回当前时间戳的微秒数
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    // 返回当前时间戳的秒数
    time_t secondsSinceEpoch() const
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ /
                                   kMicroSecondsPerSecond);
    }
    // 失效的时间戳，返回一个值为0的Timestamp
    static Timestamp invalid() { return Timestamp(); }

    static const int kMicroSecondsPerSecond = 1000 * 1000; // 1秒=1000*1000微妙

private:
    int64_t
        microSecondsSinceEpoch_; // 表示时间戳的微秒数(自epoch开始经历的微妙数)
};

/**
 * 定时器需要比较时间戳，因此需要重载运算符
 */
inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}
inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    // 将延时的秒数转换为微妙
    int64_t delta =
        static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    // 返回新增时后的时间戳
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

#endif