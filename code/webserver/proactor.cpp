//
// Created by challway on 2023/2/1.
//
#include "proactor.h"


void Proactor::HandleRead(HttpConn *client)
{
    int m_errno = -1;
    ssize_t ret = client->ReadBuffer(&m_errno);
    if (ret < 0 && m_errno != (EAGAIN | EWOULDBLOCK))
    {
        // log
        CloseConn(client);
    }
    // 客户端进行活动时应该更新定时器
    if (m_timeout_ms_ > 0)
        ExtentTime(client->get_fd());
    // 模拟proactor模式，主线程读好的数据在client.m_read_buf,处理请求报文和准备响应数据在子线程完成
    thread_pool_->Append(std::bind(&Proactor::OnRead, this, client));
}

void Proactor::HandleWrite(HttpConn *client)
{
    OnWrite(client);
}

void Proactor::OnRead(HttpConn *client)
{
    Process(client);
}

void Proactor::OnWrite(HttpConn *client)
{
    // 现在读好的数据在iov和mmap中
    // 模拟proactor模式，主线程处理，不需要再把写事件加入请求队列了，少了一次调用epoll_wait的开销
    int m_errno = -1;
    ssize_t ret = client->WriteBuffer(&m_errno);
    ExtentTime(client->get_fd());
    // 根据发送数据的长度ret来判断服务器发送数据是否正常
    /*
    1. 如果向客户端成功发送了数据：
        - 如果客户端请求保持连接keep-alive：
            - 保持连接。
            - 没有操作后m_timeoutMs后断开
        - 不保持连接：
            - 调用closeConn关闭连接
    2. 发送数据失败？
        - Httpconnection中没有装填上应有的数据：
            - 服务器自己的问题：
                - 可能是请求数据失败，导致接收buffer拿不到数据，因此发送数据也就失败了
                - 还可能是装填数据失败
                重新打开接收窗口，让读缓冲区接收I/O数据：
                    - 不一定能再次拿到数据。毕竟客户端发送了一遍数据后，就不再发送了
        - 服务器调用writev函数，发送数据过程中，产生了错误
            - 错误要根据错误号判断属于哪种错误。
                - EAGAIN：让服务器再一次发送数据
            - 其他错误就不用再重发了
    */
    if (ret > 0)
    {
        // 写成功了
        // 如果是长连接，应该看看缓冲区还有没有下一个请求，有继续响应，没有等待下一次发送重置EPOLLIN事件，即Process函数
        if (client->is_keep_alive())
        {
            Process(client);
        }
        // 非keepalive，一次连接只处理一个请求，不管怎样都要关闭连接。
        else
        {
            CloseConn(client);
        }
    }
    else if (client->WriteableBytes() == 0)
    {
        if (client->is_keep_alive())
        {
            Process(client);
        }
        else
        {
            CloseConn(client);
        }
    }
    else if (ret < 0)
    {
        // 写错误
        if (m_errno == (EAGAIN | EWOULDBLOCK))
        {
            // 写不出去，重写
            epoller_->ModFd(client->get_fd(), connect_event_ | EPOLLOUT);
        }
        else
        {
            return;
        }
    }
}
