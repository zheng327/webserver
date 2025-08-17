#pragma once
#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread
{
    extern thread_local int t_cachedTid;
    void cacheTid();
    inline int tid() // 内联函数只在当前文件中起作用
    {
        // __builtin_expect 是一种底层优化 此语句意思是如果还未获取tid 进入if
        // 通过cacheTid()系统调用获取tid
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
} // namespace CurrentThread