#include <CurrentThread.hpp>
#include <Logger.hpp>

namespace ThreadInfo
{
    thread_local char t_errnobuf[512]; // 每个线程独立的错误信息缓冲
    thread_local char t_timer[64];     // 每个线程独立的时间格式化缓冲区
    thread_local time_t t_lastSecond;  // 每个线程记录上次格式化的时间
} // namespace ThreadInfo

const char *getErrnoMsg(int savedErrno)
{
    return strerror_r(savedErrno, ThreadInfo::t_errnobuf,
                      sizeof(ThreadInfo::t_errnobuf));
}

// 根据Level 返回level_名字
const char *getLevelName[Logger::LogLevel::LEVEL_COUNT]{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
};

/**
 * 默认的日志输出函数
 * 将日志内容写入标准输出流(stdout)
 * @param data 要输出的日志数据
 * @param len 日志数据的长度W
 */
static void defalutOutput(const char *data, int len)
{
    fwrite(data, len, sizeof(char), stdout);
}

/**
 * 默认的刷新函数
 * 刷新标准输出流的缓冲区,确保日志及时输出
 * 在发生错误或需要立即看到日志时会被调用
 */
static void defaultFlush() { fflush(stdout); }

Logger::OutputFunc g_output = defalutOutput;
Logger::FlushFunc g_flush = defaultFlush;

Logger::Impl::Impl(LogLevel level, int savedErrno, const char *filename,
                   int line)
    : time_(Timestamp::now()), stream_(), level_(level), line_(line),
      basename_(filename)
{
    // 根据时区格式化当前时间字符串, 也是一条log消息的开头
    formatTime();
    // 写入日志等级
    stream_ << GeneralTemplate(getLevelName[level], 6);
    if (savedErrno != 0)
    {
        stream_ << getErrnoMsg(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}
// 根据时区格式化当前时间字符串, 也是一条log消息的开头
void Logger::Impl::formatTime()
{
    Timestamp now = Timestamp::now();
    // 计算秒数
    time_t seconds = static_cast<time_t>(now.secondsSinceEpoch());
    // 计算剩余微秒数
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() %
                                        Timestamp::kMicroSecondsPerSecond);
    struct tm *tm_time = localtime(&seconds);
    // 写入此线程存储的时间buf中
    snprintf(ThreadInfo::t_timer, sizeof(ThreadInfo::t_timer),
             "%4d/%02d/%02d %02d:%02d:%02d", tm_time->tm_year + 1900,
             tm_time->tm_mon + 1, tm_time->tm_mday, tm_time->tm_hour,
             tm_time->tm_min, tm_time->tm_sec);
    ThreadInfo::t_lastSecond = seconds;

    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%06d", microseconds);

    stream_ << GeneralTemplate(ThreadInfo::t_timer, 17)
            << GeneralTemplate(buf, 6);
}

void Logger::Impl::finish()
{
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_) << ':'
            << line_ << '\n';
}

Logger::Logger(const char *filename, int line, LogLevel level) : impl_(level, 0, filename, line)
{
}

Logger::~Logger()
{
    impl_.finish();
    const LogStream::Buffer &buffer = stream().buffer();
    // 输出(默认项终端输出)
    g_output(buffer.data(), buffer.length());
    // FATAL情况终止程序
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort(); // 该函数用于异常终止程序的执行,它会立即终止调用进程.
    }
}

void Logger::setOutput(OutputFunc output)
{
    g_output = output;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}