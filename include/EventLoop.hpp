#pragma once
#include <functional>
#include <atomic>
#include <mutex>

// 事件循环类 主要包含两大模块 Channel Poller
// Channel封装了sockfd和感兴趣的事件以及发生的事件
// Poller封装了IO多路复用epoll
class EventLoop
{
public:
    using Functor = std::function<void()>;

    EventLoop() = default;
    ~EventLoop() = default;

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行
    void runInLoop(Functor cb);
    // 把上层注册的回调函数cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 通过eventfd唤醒loop所在的线程
    void wakeup();

    // EentLoop的方法 => Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里
    // threadId_为EentLoop创建时的线程id，CurrentThread::tid()为当前线程id
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    /**
     * 定时任务相关函数
     */
    // 在某个时间点执行回调
    void runAt(Timestamp timestamp, Functor &&cb)
    {
        timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
    }
    // 多少秒后执行回调
    void runAfter(double waitTime, Functor &&cb)
    {
        Timestamp timestamp(addTIme(Timestamp::now(), waitTime));
        timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
    }
    // 每隔多少秒执行一次回调
    void runEvery(double interval, Functor &&cb)
    {
        Timestamp timestamp(addTIme(Timestamp::now(), interval));
        timerQueue_->addTimer(std::move(cb), timestamp, interval);
    }

private:
    void handleRead(); // 给eventfd返回的文件描述符wakeupFd_绑定的事件回调
    // 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
    void doPendingFunctors(); // 执行上层回调函数

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_; // 原子操作 事件循环是否启动 底层通过CAS实现
    std::atomic_bool quit_;    // 标识退出loop循环

    Timestamp pollReturnTime_;

    pid_t threadId_; // 记录当前loop所在线程的id

    std::unique_ptr<Poller> poller_;         // IO多路复用器
    std::unique_ptr<TimerQueue> timerQueue_; // 定时器
    int wakeupFd_;                           // eventfd文件描述符 用于唤醒loop所在的线程
    std::unique_ptr<Channel> wakeupChannel_; // 封装wakeupFd_的channel

    ChanelList activeChannels_; // poller返回的活跃事件列表

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有回调操作
    std::mutex mutex_;                        // 互斥锁 用于保护上面vector容器的线程安全
};