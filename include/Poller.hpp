#pragma once
#include <vector>
#include <unordered_map>

#include "Timestamp.hpp"

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心 IO复用模块
class Poller
{
public:
    using ChannelList = std::vector<Channel *>;
    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前的Poller当中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获得默认的IO复用的实现
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // map的key:socktfd,value:socktfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_; // Poller所属的事件循环EventLoop
};