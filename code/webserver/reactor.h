//
// Created by challway on 2022/12/26.
//

#ifndef HTTPSERVER_FINAL_REACTOR_H
#define HTTPSERVER_FINAL_REACTOR_H

#include "httpserver.h"

class Reactor : public HttpServer
{

public:
    explicit Reactor(int port, int timeout = 60, int thread_num = 8, int event_mode = 1,int loglevel =1){};
    ~Reactor(){};

    void HandleRead(HttpConn *client) override;
    void HandleWrite(HttpConn *client) override;

    void OnRead(HttpConn *client) override;
    void OnWrite(HttpConn *client) override;
};

#endif // HTTPSERVER_FINAL_REACTOR_H
