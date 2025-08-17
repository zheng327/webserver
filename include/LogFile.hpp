#pragma once
#include <string>
#include <sys/types.h>
#include <mutex>
#include <memory>
#include <FileUtil.hpp>
#include <ctime>

class LogFile
{
public:
    /**
     * @brief 构造函数
     * @param basename 日志文件基本名称
     * @param rollsize 日志文件大小达到多少字节时滚动，单位:字节
     * @param flushInterval 日志刷新间隔时间，默认3秒
     * @param checkEveryN_ 写入checkEveryN_次后检查是否需要滚动，默认1024次
     */
    LogFile(const std::string &basename, off_t rollSize, int flushInterval = 3,
            int checkEveryN_ = 1024);
    ~LogFile() = default;

    /**
     * @brief 追加数据到日志文件
     * @param data 要写入的数据
     * @param len 数据长度
     */
    void append(const char *data, int len);

    /**
     * @brief 强制将缓冲区数据刷新到磁盘
     */
    void flush();

    /**
     * @brief 滚动日志文件
     * 当日志文件大小超过rollsize_或时间超过一天时，创建新的日志文件
     * @return 是否成功滚动日志文件
     */
    bool rollFile();

private:
    /**
     * @brief 生成日志文件名
     * @param basename 日志文件基本名称
     * @param now 当前时间指针
     * @return 完整的日志文件名，格式为:basename.YYYYmmdd-HHMMSS.log
     */
    static std::string getLogFileName(const std::string &basename, time_t *now);

    /**
     * @brief 在已加锁的情况下追加数据
     * @param data 要写入的数据
     * @param len 数据长度
     */
    void appendInlock(const char *data, int len);

    const std::string basename_; // 文件基本名称（不带日期）
    const off_t rollSize_;       // 滚动文件大小
    const int flushInterval_;    // 日志刷新间隔，默认3s
    const int checkEveryN_;      // 写数据次数限制，默认1024

    int count_; // 写入次数计数，超过限制值checkEveryN_时清除，然后重新计数

    std::mutex mutex_;
    time_t startOfPeriod_; // 本次写log周期的起始时间（秒）
    time_t lastRoll_;      // 上次roll日志文件的时间（秒）
    time_t lastFlush_;     // 上次flush日志文件的时间（秒）
    std::unique_ptr<FileUtil> file_;
    const static int kRollPerSeconds_ = 60 * 60 * 24;
};