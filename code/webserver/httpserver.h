//
// Created by challway on 2022/12/22.
//

#ifndef HTTPSERVER_FINAL_HTTPSERVER_H
#define HTTPSERVER_FINAL_HTTPSERVER_H

#include "../buffer/buffer.h"
#include "../database/database.h"
#include "../database/user.h"
#include "../database/usermodel.h"
#include "../epoller/epoller.h"
#include "../http/httpconn.h"
#include "../timer/timer.h"
#include "../threadpool.h"
#include "../locker.h"
#include <sys/socket.h>
#include <cerrno>
#include <arpa/inet.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <sys/fcntl.h>
#include <unistd.h>


class HttpServer{
public:
    HttpServer(){};
    explicit HttpServer(int port,int timeout=60,int thread_num=8,int event_mode=1);
    virtual ~HttpServer()=default;

    //启动服务器
    void start();

protected:
    void CloseConn(HttpConn* client);
    void Process(HttpConn* client);
    //刷新定时器时间
    void ExtentTime(int sockfd);

    std::unique_ptr<Epoller> epoller_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::shared_ptr<TimeManager> timer_;
    std::unordered_map<int,HttpConn> m_clients_;
    //预先设置的事件模式
    uint32_t listen_event_;
    uint32_t connect_event_;

    //定时器的超时时间
    int m_timeout_ms_;
protected:
    //创建listenfd
    bool InitSocket();
    //封装ET的各个选项
    void InitEvents(int mode);

    //epoll同时监听listenfd和connfd，因此要采取的行动也会有所不同
    void HandleConn();
    virtual void HandleRead(HttpConn* client)=0;
    virtual void HandleWrite(HttpConn* client)=0;
    //线程主控函数
    virtual void OnWrite(HttpConn* client)=0;
    virtual void OnRead(HttpConn* client)=0;
    //添加客户端连接
    void AddClientConnect(int sockfd,struct sockaddr_in& sockaddrIn);

    void SetNonBlock(int fd);

    inline int GetErrNo(){return errno;}
    void SendError(int sockfd,const char* info) const;

    const static int MAX_FD=65535;
    const static int LFD_ET=0;
    const static int CFD_ET=1;
    const static int ALL_ET=2;

    int port_;
    int listenfd_;

    char m_src_dir_[200];

    bool is_close_;




};
#endif


/*
class HttpServer{
public:
    HttpServer(){};
    explicit HttpServer(int port,int timeout=60,int thread_num=8,int event_mode=1);
    ~HttpServer() {
        close(listenfd_);
        delete epoller_;
        delete timer_;
        delete thread_pool_;
    }

    //启动服务器
    void start();

private:
    //创建listenfd
    bool InitSocket();
    //封装ET的各个选项
    void InitEvents(int mode);

    //epoll同时监听listenfd和connfd，因此要采取的行动也会有所不同
    void HandleConn();
    virtual bool HandleRead(HttpConn* client);
    virtual bool HandleWrite(HttpConn* client);
    virtual void CloseConn(HttpConn* client);
    virtual void Process(HttpConn* client);
    //添加客户端连接
    void AddClientConnect(int sockfd,struct sockaddr_in& sockaddrIn);

    void SetNonBlock(int fd);

    //刷新定时器时间
    void ExtentTime(int sockfd);

    inline int GetErrNo(){return errno;}
    void SendError(int sockfd,const char* info) const;




    const static int MAX_FD=65535;
    const static int LFD_ET=0;
    const static int CFD_ET=1;
    const static int ALL_ET=2;


    int port_;
    int listenfd_;
    //定时器的超时时间
    int m_timeout_ms_;

    char m_src_dir_[200];

    bool is_close_;

    //预先设置的事件模式
    uint32_t listen_event_;
    uint32_t connect_event_;

    //std::unique_ptr<Epoller> epoller_;
    Epoller* epoller_;
    TimeManager* timer_;
    //std::unique_ptr<TimeManager> timer_;
    ThreadPool* thread_pool_;
    //std::unique_ptr<ThreadPool> thread_pool_;
    std::unordered_map<int,HttpConn> m_clients_;
};
*/

