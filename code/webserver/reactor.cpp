//
// Created by challway on 2023/2/1.
//
#include "reactor.h"
// 不能直接从初始化程序列表中初始化基类成员
Reactor::Reactor(int port, int timeout, int thread_num, int event_mode)
{
    port_ = port;
    m_timeout_ms_ = timeout;
    timer_ = std::make_unique<TimeManager>();
    thread_pool_ = std::make_unique<ThreadPool>(thread_num, 200);
    epoller_ = std::make_unique<Epoller>();
    // getcwd(m_src_dir_,sizeof (m_src_dir_));
    // strcat(m_src_dir_, "/resources");
    strcpy(m_src_dir_, "/media/psf/Home/Documents/vscode project/Httpserver_final/resources/");
    HttpConn::m_url = m_src_dir_;
    InitEvents(event_mode);
    is_close_ = (!InitSocket());
}

void Reactor::HandleRead(HttpConn *client)
{
    assert(client);
    ExtentTime(client->get_fd());
    thread_pool_->Append(std::bind(&Reactor::OnRead, this, client));
}

void Reactor::HandleWrite(HttpConn *client)
{
    assert(client);
    ExtentTime(client->get_fd());
    thread_pool_->Append(std::bind(&Reactor::OnWrite, this, client));
}

// reactor读操作由子线程完成，OnRead是线程的处理函数
void Reactor::OnRead(HttpConn *client)
{
    assert(client);
    ssize_t ret = -1;
    int readErrno = 0;
    ret = client->ReadBuffer(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN)
    {
        // std::cout<<"do not read data!"<<std::endl;
        CloseConn(client);
        return;
    }
    Process(client);
}

void Reactor::OnWrite(HttpConn *client)
{
    assert(client);
    ssize_t ret = -1;
    int writeErrno = 0;
    ret = client->WriteBuffer(&writeErrno);
    if (client->WriteableBytes() == 0)
    {
        /* 传输完成 */
        if (client->is_keep_alive())
        {
            Process(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)
        {
            /* 继续传输 */
            epoller_->ModFd(client->get_fd(), connect_event_ | EPOLLOUT);
            return;
        }
    }
    CloseConn(client);
}
