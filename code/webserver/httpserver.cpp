//
// Created by challway on 2022/12/22.
//
#include "httpserver.h"

HttpServer::HttpServer(int port,int timeout,int thread_num,int event_mode)
        :port_(port),m_timeout_ms_(timeout),timer_(new TimeManager()),thread_pool_(new ThreadPool(thread_num,200)),epoller_(new Epoller)
{
    //getcwd(m_src_dir_,sizeof (m_src_dir_));
    //strcat(m_src_dir_, "/resources");
    strcpy(m_src_dir_,"/media/psf/Home/Documents/vscode project/Httpserver_final/resources/");
    HttpConn::m_url=m_src_dir_;
    InitEvents(event_mode);
    is_close_=(!InitSocket());
}

void HttpServer::InitEvents(int mode) {
    // EPOLLRDHUP事件，底层处理socket连接断开的情况,是需要作为事件add进监听列表的
    listen_event_=EPOLLRDHUP;
    connect_event_=EPOLLRDHUP|EPOLLONESHOT;
    switch(mode){
        case LFD_ET:
            listen_event_|=EPOLLET;
            break;
        case CFD_ET:
            connect_event_|=EPOLLET;
            break;
        case ALL_ET:
            listen_event_|=EPOLLET;
            connect_event_|=EPOLLET;
            break;
        default:
            break;
    }
    HttpConn::isET=(mode==CFD_ET)||(mode==ALL_ET);
}


bool HttpServer::InitSocket() {
    listenfd_=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd_<0){
        //log
        return false;
    }
    struct sockaddr_in sockaddrIn;
    sockaddrIn.sin_family=AF_INET;
    sockaddrIn.sin_port=htons(port_);
    sockaddrIn.sin_addr.s_addr= htonl(INADDR_ANY);

    //优雅关闭

    //端口复用
    int reuse=1;
    if(setsockopt(listenfd_,SOL_SOCKET,SO_REUSEADDR,(const char*)&reuse,sizeof(reuse))<0){
        //log
        close(listenfd_);
        return false;
    }
    //绑定IP和port给sockfd
    int ret=bind(listenfd_,(struct sockaddr*)&sockaddrIn,sizeof(sockaddrIn));
    if(ret<0){
        //log
        close(listenfd_);
        return false;
    }
    //调用listenfd设置sockfd的监听状态,和最多允许几个客户端处于待连接状态
    ret= listen(listenfd_,6);
    if(ret<0){
        close(listenfd_);
        return false;
    }
    //将listenfd添加到epoll监听队列中,EPOLLIN监听lfd是否有数据写入（即连接进入）
    ret=epoller_->AddFd(listenfd_,listen_event_|EPOLLIN);
    if(!ret){
        close(listenfd_);
        return false;
    }
    //SetNonBlock(listenfd_);
    return true;
}

void HttpServer::start() {
    if(!is_close_)
    {
        std::cout<<"============================";
        std::cout<<"       Server Start         ";
        std::cout<<"============================";
        std::cout<<std::endl;
    }
    while(!is_close_)
    {
        int waitout=0;
        //清除过时连接，获取下次处理的时间
        //如果设定的定时器时间等于0，意味着不需要清除定时器，同时waitout=-1，epollwait阻塞
        if(m_timeout_ms_>0){
            //waitout=timer_->GetNextHandle();
        }
        int epoll_count=epoller_->Wait(waitout);
        for(int i=0;i<epoll_count;i++)
        {
            int cur_fd=epoller_->GetSockFd(i);
            auto cur_events=epoller_->GetEvents(i);

            if(listenfd_==cur_fd)
            {
                HandleConn();
            }
            else
            {
                //EPOLLRDHUP事件前面已经添加过监听，该事件既可以epollctl，也可以epollwait用结构体结果返回
                //其他两个事件则只能返回，不需要加入监听事件集合
                if(cur_events&(EPOLLRDHUP|EPOLLERR|EPOLLHUP))
                {
                    //close
                    CloseConn(&m_clients_[cur_fd]);
                }
                else if(cur_events&(EPOLLIN))
                {
                    //read
                    HandleRead(&m_clients_[cur_fd]);
                }
                else if(cur_events&(EPOLLOUT))
                {
                    //read
                    HandleWrite(&m_clients_[cur_fd]);
                }
                else
                {
                    //log error
                }
            }
        }
    }
}

void HttpServer::HandleConn() {
    struct sockaddr_in cli_addr{};
    socklen_t len=sizeof(cli_addr);
    do{
        //lfd ET accept block?
        int conn_fd= accept(listenfd_,(struct sockaddr*)&cli_addr,&len);
        if(conn_fd<0)
        {
            //log
            return;
        }
        //超过上限
        if(HttpConn::userCount>=MAX_FD)
        {
            SendError(conn_fd,"Server Busy!");
            return;
        }
        AddClientConnect(conn_fd,cli_addr);
    }
    while(listen_event_&EPOLLET);
}

void HttpServer::AddClientConnect(int sockfd,struct sockaddr_in& sockaddrIn) {
    if(sockfd<0){
        return ;
    }
    //初始化httpconn对象
    m_clients_[sockfd].InitConn(sockfd,sockaddrIn);
    //加入epoll监听列表
    epoller_->AddFd(sockfd,connect_event_|EPOLLIN);
    //设置定时器
    if(m_timeout_ms_>0)
    {
        //chuwenti
        timer_->AddTimer(sockfd,m_timeout_ms_,std::bind(&HttpServer::CloseConn,this,&m_clients_[sockfd]));
    }
    //设置成非阻塞，即使不使用ET模式也要防止特殊情况
    SetNonBlock(sockfd);
}

void HttpServer::SendError(int sockfd,const char* info) const {
    if(sockfd<0){
        //log
        return;
    }
    ssize_t ret=send(sockfd,(const char*)info,sizeof(info)+1,0);
    if(ret<0){
        //log
        return ;
    }
    close(sockfd);
}
void HttpServer::CloseConn(HttpConn* client) {

    if(!epoller_->DelFd(client->get_fd()))
    {
        //log
        return;
    }
    if(m_timeout_ms_>0)
    {
        timer_->DelTimer(client->get_fd());
    }
    client->CloseConn();
}
void HttpServer::SetNonBlock(int fd) {
    //int oldflg= fcntl(fd,F_GETFL);
    int oldflg= fcntl(fd,F_GETFD);
    int newflg=oldflg|O_NONBLOCK;
    fcntl(fd,F_SETFL,newflg);
}

void HttpServer::HandleRead(HttpConn* client) {
    int m_errno=-1;
    ssize_t ret=client->ReadBuffer(&m_errno);
    if(ret<0 && m_errno!=(EAGAIN|EWOULDBLOCK))
    {
        //log
        CloseConn(client);
    }
    //客户端进行活动时应该更新定时器
    if(m_timeout_ms_>0)
        ExtentTime(client->get_fd());
    //模拟proactor模式，主线程读好的数据在client.m_read_buf,处理请求报文和准备响应数据在子线程完成
    thread_pool_->Append(std::bind(&HttpServer::Process,this,client));
}

void HttpServer::HandleWrite(HttpConn* client) {
    //现在读好的数据在iov和mmap中
    //模拟proactor模式，主线程处理，不需要再把写事件加入请求队列了，少了一次调用epoll_wait的开销
    int m_errno=-1;
    ssize_t ret=client->WriteBuffer(&m_errno);
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
    if(ret>0)
    {
        //写成功了
        //如果是长连接，应该看看缓冲区还有没有下一个请求，有继续响应，没有等待下一次发送重置EPOLLIN事件，即Process函数
        if(client->is_keep_alive())
        {
            Process(client);
        }
            //非keepalive，一次连接只处理一个请求，不管怎样都要关闭连接。
        else
        {
            CloseConn(client);
        }
    }
    else if(client->WriteableBytes()==0)
    {
        if(client->is_keep_alive())
        {
            Process(client);
        }
        else
        {
            CloseConn(client);
        }
    }
    else if(ret<0)
    {
        //写错误
        if(m_errno==(EAGAIN|EWOULDBLOCK))
        {
            //写不出去，重写
            epoller_->ModFd(client->get_fd(),connect_event_|EPOLLOUT);
        }
        else
        {
            return;
        }
    }
}

void HttpServer::Process(HttpConn* client) {
    //如果读完了，还有数据可处理，就手动重置EPOLLOUT事件
    if(client->HandleHttp())
    {
        epoller_->ModFd(client->get_fd(),connect_event_|EPOLLOUT);
    }
        //如果readbuffer没有可读数据，说明没读到，重置     由于EPOLLONESHOT需要重置EPOLLIN？
    else
    {
        epoller_->ModFd(client->get_fd(),connect_event_|EPOLLIN);
    }

}

void HttpServer::ExtentTime(int sockfd) {
    assert(sockfd);
    if(m_timeout_ms_>0)
    {
        timer_->Update(sockfd,m_timeout_ms_);
    }
}






