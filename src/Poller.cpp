#include <Poller.hpp>
#include <Channel.hpp>
#include <EPollPoller.hpp>

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}
bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        return nullptr;
    }
    else
    {
        return new EPollPoller(loop);
    }
}