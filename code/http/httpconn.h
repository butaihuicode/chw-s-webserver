//
// Created by challway on 2022/11/30.
//

#ifndef HTTPSERVER_FINAL_HTTPCONN_H
#define HTTPSERVER_FINAL_HTTPCONN_H

#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <atomic>



//一个socket对应一个httpconn类
class HttpConn{
public:
    HttpConn();
    ~HttpConn();

    //init意义在于，初始化连接，不需要重构对象也能重置初始化所有变量
    void InitConn(int sockfd,struct sockaddr_in& sockaddrIn);
    bool CloseConn();
    //主要流程有三个函数：
    //1.读数据到m_read_buf_，返回读到的大小
    ssize_t ReadBuffer(int* m_errno);
    //2.对m_read_buf_数据进行处理，生成响应报文,数据写到m_iov里
    bool HandleHttp();
    //3.把响应报文和需求资源从m_iov一共发到对端
    ssize_t WriteBuffer(int* m_errno);

    size_t WriteableBytes()const {return m_iov[0].iov_len+m_iov[1].iov_len;}

    inline int get_fd() const{return m_sockfd;};

    //是否ET,要由main决定，所以写成public，在main之前就初始化好，由main更改,所有connfd均由这个控制ET
    static bool isET;

    inline bool is_keep_alive ()const{return m_http_request_.is_keep_alive();}
    //连接的用户数量，是类的变量，可能有多个线程同时操作，设置为原子变量。
    static std::atomic<int> userCount;

//资源根目录,用静态变量表示确保访问域
static const char* m_url;
private:
    int m_sockfd;
    struct sockaddr_in m_addr_;
    struct iovec m_iov[2];
    //连接是否关闭
    bool is_close_;
    //负责解析请求
    HttpRequest m_http_request_;
    //负责生成响应
    HttpResponse m_http_response_;

    buffer m_read_buf_;
    buffer m_write_buf_;

};


#endif //HTTPSERVER_FINAL_HTTPCONN_H
