//
// Created by challway on 2022/11/28.
//
#ifndef Epoller_H
#define Epoller_H

#include <sys/epoll.h>
#include <vector>
#include <cassert>

class Epoller{
public:
    Epoller(int max_events=1024);
    ~Epoller()=default;

    bool AddFd(int fd,__uint32_t event);
    bool DelFd(int fd);
    bool ModFd(int fd,__uint32_t event);

    int Wait(int timeout);

    int GetSockFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;
private:
    int epfd_;
    //epoll_Wait检测到事件，会把所有就绪事件从内核事件表复制到它的第二个参数events指向的数组中，这里用vector接收
    std::vector<struct epoll_event> m_events_;
};




#endif 
