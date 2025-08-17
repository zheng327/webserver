#pragma once
#include <string>
#include <sys/types.h>
#include <stdio.h>

class FileUtil
{
public:
    FileUtil(std::string &file_name);
    ~FileUtil();
    // 向文件写入数据
    void append(const char *data, size_t len);
    // 刷新文件缓冲区,将缓冲区中的数据立即写入文件
    void flush();
    // 获取已写入的字节数,返回已写入文件的总字节数
    off_t writtenBytes() const { return writtenBytes_; }

private:
    size_t write(const char *data, size_t len);
    FILE *file_;             // 文件指针，用于操作文件
    char buffer_[64 * 1024]; // 文件操作的缓冲区，大小为64KB，用于提高写入效率
    off_t writtenBytes_;     // 记录已写入文件的总字节数，off_t类型用于大文件支持
};