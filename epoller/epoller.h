//
// Created by challway on 2022/11/28.
//
#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <vector>
#include <assert.h>

class epoller{
public:
    epoller(int max_events=1024);
    ~epoller()=default;

    bool addFd(int fd,__uint32_t event);
    bool delFd(int fd);
    bool modFd(int fd,__uint32_t event);

    int wait(int timeout);

    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;
private:
    int epfd;
    //epoll_wait检测到事件，会把所有就绪事件从内核事件表复制到它的第二个参数events指向的数组中，这里用vector接收
    std::vector<struct epoll_event> m_events;

};














#endif EPOLLER_H
