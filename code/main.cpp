//
// Created by challway on 2022/11/27.
//
#include "webserver/httpserver.h"
#include "webserver/reactor.h"
#include "webserver/proactor.h"
#include "locker.h"
#include "epoller/epoller.h"
#include "http/httpconn.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "timer/timer.h"
#include "threadpool.h"
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <libgen.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        // basename截取path中的去目录部分的最后的文件或路径名
        std::cout << "请按如下方式运行程序: ./" << basename(argv[0]) << "portname ServerMode(1:Reactor 2:模拟Proactor\n";
        exit(-1);
    }
    // 获取端口号
    int port = atoi(argv[1]);
    int ServerMode = atoi(argv[2]);
    if (ServerMode != 1 && ServerMode != 2)
    {
        std::cout << "Wrong Mode" << std::endl;
        return 0;
    }
    // 通过终端输入定时器定时时间
    int timeout = -1;
    int triMode = 1;
    int threadNum = 8;
    int loglevel=1;

    if(argc > 6)
    {
        loglevel=atoi(argv[6]);
    }
    if (argc > 5)
    {
        triMode = atoi(argv[5]);
    }
    else if (argc > 4)
    {
        threadNum = atoi(argv[4]);
    }
    else if (argc > 3)
    {
        timeout = atoi(argv[3]);
    }
    if (ServerMode == 1)
    {
        Reactor httpServer(port, timeout, threadNum, triMode,loglevel);
        httpServer.start();
    }
    else if (ServerMode == 2)
    {
        Proactor httpServer(port, timeout, threadNum, triMode,loglevel);
        httpServer.start();
    }
    return 0;
}
