//
// Created by challway on 2022/11/28.
//
#include "epoller.h"

Epoller::Epoller(int max_event) : epfd_(epoll_create(114514)), m_events_(max_event) {}

bool Epoller::AddFd(int fd, __uint32_t event)
{
    if (fd < 0)
        return false;
    struct epoll_event events;
    events.data.fd = fd;
    events.events = event;
    return 0 == epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &events);
}

bool Epoller::DelFd(int fd)
{
    if (fd < 0)
        return false;
    return 0 == epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
}

bool Epoller::ModFd(int fd, __uint32_t event)
{
    if (fd < 0)
        return false;
    struct epoll_event events;
    events.data.fd = fd;
    events.events = event;
    return 0 == epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &events);
}

int Epoller::Wait(int timeout)
{
    return epoll_wait(epfd_, &m_events_[0], static_cast<int>(m_events_.size()), timeout);
};

int Epoller::GetSockFd(size_t i) const
{
    assert(i < m_events_.size() && i >= 0);
    return m_events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const
{
    assert(i < m_events_.size() && i >= 0);
    return m_events_[i].events;
}