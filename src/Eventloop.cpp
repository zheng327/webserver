#include <Eventloop.hpp>
#include <Logger.h>
#include <sys/eventfd.h>

// 防止一个线程创建多个Eventloop实例,如果一个线程以及经创建了一个Eventloop实例，那么这个值会被设置成this
thread_local Eventloop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

/* 创建线程之后主线程和子线程谁先运行是不确定的。
 * 通过一个eventfd在线程之间传递数据的好处是多个线程无需上锁就可以实现同步。
 * eventfd支持的最低内核版本为Linux 2.6.27,在2.6.26及之前的版本也可以使用eventfd，但是flags必须设置为0。
 * 函数原型：
 *     #include <sys/eventfd.h>
 *     int eventfd(unsigned int initval, int flags);
 * 参数说明：
 *      initval,初始化计数器的值。
 *      flags, EFD_NONBLOCK,设置socket为非阻塞。
 *             EFD_CLOEXEC，执行fork的时候，在父进程中的描述符会自动关闭，子进程中的描述符保留。
 * 场景：
 *     eventfd可以用于同一个进程之中的线程之间的通信。
 *     eventfd还可以用于同亲缘关系的进程之间的通信。
 *     eventfd用于不同亲缘关系的进程之间通信的话需要把eventfd放在几个进程共享的共享内存中（没有测试过）。
 */
// 创建wakeupfd用来notify唤醒subReactor处理新来的channel
int creatEventfd()
{
    int evtfd = ::evenfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0)
    {
        LOG_FATAL << “eventfd error : % d " << errno;
    }
    return evtfd;
}

// Eventloop类的构造函数
Eventloop::Eventloop()
    : looping_(false), quit_(false), callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(creatEventfd()), wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG << "Eventloop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another Eventloop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(
        std::bind(&Eventloop::handleRead,
                  this)); // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_
        ->enableReading(); // 每一个EventLoop都将监听wakeupChannel_的EPOLL读事件了
}
// Eventloop类的析构函数
Eventloop::~Eventloop()
{
    wakeupChannel_->disableAll(); // 给Channel移除所有感兴趣的事件
    wakeupChannel_->remove();     // 把Channel从Eventlopp中删除
    ::close(wakeupFd_);           // 关闭wakeupFd_文件描述符
    t_loopInThisThread = nullptr; // 清除当前线程的Eventloop实例
}

// 开启事件循环
void Eventloop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO << "Eventloop start looping";

    while (!quit_)
    {
        activeChannels_.clear(); // 清除上次poller返回的活跃事件列表
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生了事件
            // 然后上报给EventLoop通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        /**
         *执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程
         *mainloop(mainReactor) 主要工作： accept接收连接 =>
         *将accept返回的connfd打包为Channel =>
         *TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
         *
         *mainloop调用queueInLoop将回调加入subloop（该回调需要subloop执行
         *但subloop还在poller_->poll处阻塞） queueInLoop通过wakeup将subloop唤醒
         **/
        doPendingFunctors();
    }
    LOG_INFO << "Eventloop stop looping";
    looping_ = false;
}
/**
 * 退出事件循环
 * 1. 如果loop在自己的线程中调用quit成功了
 *说明当前线程已经执行完毕了loop()函数的poller_->poll并退出
 * 2. 如果不是当前EventLoop所属线程中调用quit退出EventLoop
 *需要唤醒EventLoop所属线程的epoll_wait
 *
 * 比如在一个subloop(worker)中调用mainloop(IO)的quit时
 *需要唤醒mainloop(IO)的poller_->poll 让其执行完loop()函数
 *
 * ！！！ 注意： 正常情况下 mainloop负责请求连接 将回调写入subloop中
 *通过生产者消费者模型即可实现线程安全的队列 ！！！ 但是muduo通过wakeup()机制
 *使用eventfd创建的wakeupFd_ notify 使得mainloop和subloop之间能够进行通信
 **/
void Eventloop::quit()
{
    quit_ = true;
    if (!isInloopThread())
    {
        wakeup();
    }
}

// 在当前loop中执行cb
void Eventloop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 如果当前线程是EventLoop所属线程
    {
        cb(); // 直接执行回调函数
    }
    else // 如果当前线程不是EventLoop所属线程
    {
        queueInLoop(cb); // 将回调函数放入队列中，唤醒EventLoop所在线程执行cb
    }
}
// 把上层注册的回调函数cb放入队列中，唤醒loop所在的线程，执行cb
void Eventloop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(
            mutex_);                       // 上锁，保护pendingFunctors_的线程安全
        pendingFunctors_.emplace_back(cb); // 将回调函数放入待执行队列
    }
    /**
     * || callingPendingFunctors的意思是 当前loop正在执行回调中
     *但是loop的pendingFunctors_中又加入了新的回调 需要通过wakeup写事件
     * 唤醒相应的需要执行上面回调操作的loop的线程
     *让loop()下一次poller_->poll()不再阻塞（阻塞的话会延迟前一次新加入的回调的执行），然后
     * 继续执行pendingFunctors_中的回调函数
     **/
    if (!isInloopThread() || callingPendingFunctors_)
    {
        wakeup(); // 唤醒loop所在的线程
    }
}

void Eventloop::handleRead()
{
    uint64_t one = 1;
    // 读取wakeupFd_的8字节数据，唤醒阻塞的epoll_wait
    // 这里的one是为了清除eventfd的计数器，防止重复唤醒
    // 读取的字节数应该是8字节，如果不是8字节，说明发生了错误
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        // 如果读取的字节数不是8字节，说明发生了错误
        LOG_ERROR << "Eventloop::handleRead() reads " << n << " bytes instead of 8";
    }
}

// 用来唤醒loop所在线程 向wakeupFd_写一个数据 wakeupChannel就发生读事件
// 当前loop线程就会被唤醒
void Eventloop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        // 如果写入的字节数不是8字节，说明发生了错误
        LOG_ERROR << "Eventloop::wakeup() writes " << n << " bytes instead of 8";
    }
}

// Eventloop的方法 => Poller的方法
void Eventloop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void Eventloop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool Eventloop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void Eventloop::doPendingFunctors() S
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true; // 标记当前loop正在执行回调操作
    {
        std::unique_lock<std::mutex> lock(
            mutex_); // 上锁，保护pendingFunctors_的线程安全
        functors.swap(
            pendingFunctors_); // 交换pendingFunctors_和functors，清空pendingFunctors_
                               // 交换的方式减少了锁的临界区范围 提升效率
                               // 同时避免了死锁 如果执行functor()在临界区内
                               // 且functor()中调用queueInLoop()就会产生死锁
    }
    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop所有待执行的回调函数
    }
    callingPendingFunctors_ = false; // 标记当前loop没有正在执行的回调操作
}