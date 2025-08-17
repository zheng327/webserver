#include <CurrentThread.hpp>

namespace CurrentThread
{
    thread_local int t_cachedTid = 0;
    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            // Ensure syscall and SYS_gettid are defined
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}