#include <functional>

#include "Timestamp.hpp"

class EventLoop;

/**
 * 理清楚 EventLoop、Channel、Poller之间的关系  Reactor模型上对应多路事件分发器
 * Channel理解为通道 封装了sockfd和其感兴趣的event 如EPOLLIN、EPOLLOUT事件 还绑定了poller返回的具体事件
 **/
class Channel
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel() = default;

    // fd得到Poller通知后 处理事件 handleEvent在EventLoop::loop()中被调用
    void handleEvent(Timestamp recevieTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove时，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }                  // 获取文件描述符
    int events() const { return events_; }          // 获取感兴趣的事件
    void set_revents(int revt) { revents_ = revt; } // 设置实际发生的事件

    // 设置fd相应的事件状态 相当于epoll_ctl add delete
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    int index() { return index_; }            // 获取index_
    void set_index(int idx) { index_ = idx; } // index_用于标识channel的状态

    EventLoop *ownerLoop() { return loop_; } // 获取该channel所属的EventLoop
    void remove();                           // 从poller中删除该channel

private:
    void update();                                    // 更新channel所感兴趣的事件
    void handleEventWithGuard(Timestamp receiveTime); // 该函数用于处理在指定时间接收到的事件，并执行必要的防护逻辑

    static const int kNoneEvent;  // 无事件
    static const int kReadEvent;  // 可读事件
    static const int kWriteEvent; // 可写事件

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, Poller监听的对象
    int events_;      // 注册fd感兴趣的事件（如读/写/异常事件）
    int revents_;     // poller填充返回的具体发生的事件掩码（运行时状态反馈）
    int index_;       // 内部状态索引，用于标识该监控项在事件循环中的位置或状态机状态

    std::weak_ptr<void> tie_; // 弱引用指针，用于观察关联对象的生命周期状态
    bool tied_;               // 绑定状态标志，指示当前对象是否已绑定到目标资源

    // 因为channel通道里可获知fd最终发生的具体的事件events，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
}