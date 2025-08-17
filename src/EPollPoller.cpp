#include <EPollPoller.hpp>
#include <Logger.hpp>
#include <errno.h>

const int kNew = -1;    // 某个channel还没添加至Poller（channel的index_初始为-1）
const int kAdded = 1;   // 某个channel已添加至Poller
const int kDeleted = 2; // 某个channel已经从Poller中删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) // vector<epoll_event>(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL << "epoll_crate1 error:%d \n"
                  << errno;
    }
}

EPollPoller::~EPollPoller() { ::close(epollfd_); }

/**
 * @brief 等待并处理IO事件
 *
 * 使用epoll_wait等待IO事件的发生，并将活跃的通道添加到activeChannels列表中
 *
 * @param timeoutMs 等待超时时间，单位为毫秒
 * @param activeChannels 用于存储活跃通道的列表指针
 * @return Timestamp 返回当前的时间戳
 */
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 由于频繁调用poll 实际上应该用LOG_DEBUG输出日志更为合理 当遇到并发场景
    // 关闭DEBUG日志提升效率
    // LOG_INFO << "fd total count: %d" << channels_.size();
    LOG_DEBUG << "fd total count: %d" << channels_.size();

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        // LOG_INFO << "events happened" << numEvents;
        LOG_DEBUG << "events happened" << numEvents;
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) // 扩容操作
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG << "timeout!";
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR << "EPollPoller::poll() error!";
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller
// updateChannel removeChannel
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO << "func => "
             << " fd " << channel->fd() << " events " << channel->events()
             << " index = " << index;
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        else // index == kDeleted
        {
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel已经在Poller中注册过了
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从Poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);
    LOG_INFO << "removeChannel fd = " << fd;

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(
            channel); // EventLoop就拿到了它的Poller给它返回的所有发生事件的channel列表了
    }
}

// 更新channel通道 其实就是调用epoll_ctl add/mod/del
void EPollPoller::updateChannel(int operation, Channel *channel)
{
    epoll_event event;
    ::memset(&event, 0, sizeof(event));

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR << "EPollPoller::updateChaanel epoll_ctl del error:" << errno;
        }
        else
        {
            LOG_ERROR << "EPollPoller::updateChannel epoll_ctl add/mod error:" << errno;
        }
    }
}
