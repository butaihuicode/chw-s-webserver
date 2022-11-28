//
// Created by challway on 2022/11/28.
//
#include "epoller.h"

 epoller::epoller(int max_event):epfd(epoll_create(114514)),m_events(max_event){}

bool epoller::addFd(int fd,__uint32_t event){
    if(fd < 0) return false;
    struct epoll_event events;
    events.data.fd=fd;
    events.events=event;
    return 0 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &events);
}

bool epoller::delFd(int fd){
    if(fd < 0) return false;
    return 0 == epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
}

bool epoller::modFd(int fd,__uint32_t event){
    if(fd < 0) return false;
    struct epoll_event events;
    events.data.fd=fd;
    events.events=event;
    return 0 == epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &events);
}

int epoller::wait(int timeout){
    return epoll_wait(epfd,&m_events[0],static_cast<int>(m_events.size()),timeout);
};

int epoller::getEventFd(size_t i) const {
    assert(i < m_events.size() && i >= 0);
    return m_events[i].data.fd;
}

uint32_t epoller::getEvents(size_t i) const {
    assert(i < m_events.size() && i >= 0);
    return m_events[i].events;
}