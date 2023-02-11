//
// Created by challway on 2023/2/1.
//

#ifndef HTTPSERVER_PROACTOR_H
#define HTTPSERVER_PROACTOR_H
#include "httpserver.h"

class Proactor : public HttpServer
{

public:
    explicit Proactor(int port, int timeout = 60, int thread_num = 8, int event_mode = 1);
    ~Proactor(){};
    void HandleRead(HttpConn *client) override;
    void HandleWrite(HttpConn *client) override;

    void OnRead(HttpConn *client) override;
    void OnWrite(HttpConn *client) override;
};

#endif // HTTPSERVER_PROACTOR_H
