#include "http_conn.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>

#define CONNFD_ET
// #define CONNFD_LT
#define LISTENFD_LT
//#define LISTENFD_ET

void setnonblocking(int fd)
{
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
}

void addfd(int cfd, int epollfd, bool oneshot)
{
    epoll_event event;
    event.data.fd = cfd;
#ifdef CONNFD_LT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif
#ifdef CONNFD_ET
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
#endif
#ifdef LISTENFD_LT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif
#ifdef LISTENFD_ET
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
#endif
    if (oneshot)
        event.events |= EPOLLONESHOT;
    else
        event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, cfd, &event);
    setnonblocking(cfd);
}

void removefd(int cfd, int epollfd)
{
    epoll_event event;
    event.data.fd = cfd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, cfd, NULL);
}

//重置ONESHOT
void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
#ifdef CONNFD_LT
    event.events = ;
#endif
#ifdef CONNFD_ET
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
#endif
}

void http_conn::init(int sockfd, sockaddr_in &address)
{
    m_sockfd = sockfd;
    m_address = address;
    addfd(m_sockfd, m_epollfd, true);
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ++user_count;
    init();
}

void http_conn::init()
{
    m_read_idx = 0;
    m_write_idx = 0;
    m_check_idx = 0;
    m_start_line=0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_method=GET;
    memset(m_read_buf, 0, READ_BUFFER_SIZE);
    memset(m_write_buf, 0, READ_BUFFER_SIZE);
}

void http_conn::close_conn()
{
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sockfd, NULL);
    close(m_sockfd);
}

bool http_conn::read_once()
{
    if (m_read_idx > READ_BUFFER_SIZE)
    {
        return false;
    }
    int already_read = 0;
#ifdef CONNFD_ET
    while (true)
    {
        already_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += already_read;
        if (already_read == 0)
        {
            // close_conn();
            return false;
        }
        else if (already_read == -1)
        {
            if (errno == EAGAIN | EWOULDBLOCK)
                ;
            break;
            return false;
        }
    }
    return true;
#endif

#ifdef CONNFD_LT
    already_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
    m_read_idx += already_read;
    if (already_read == -1)
    {
        return false;
    }
    /*else if(already_read==0){
        close_conn();
        return false;
    }
    内核2.6更新后不需要使用recv返回值判断对端端开，使用EPOLLRDHUP（读关闭）和EPOLLHUP（都关闭）事件
    */
    return true;

#endif
}

void http_conn::process()
{
}

http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_ret = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    // parse_line成功解析到一行就会返回结束，将末尾变成\0\0，由此可以分割
    char *text = 0;
    //第一个条件是请求行和请求头的判断，第二个条件是POST的请求体，因为请求体最后没有\r\n，因此parseline
    //不能分割，所以要借助状态判断，为防止解析完请求体继续循环，加入LINEOK的判断条件
    while (((line_ret = parse_line() )== LINE_OK)||line_ret==LINE_OK&&m_check_state==CHECK_STATE_CONTENT)
    {
        text = get_line();
        m_start_line = m_check_idx;
        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
            ret = parse_request_line(text);
            if (BAD_REQUEST == ret)
            {
                //语法错误直接结束
                return BAD_REQUEST;
            }
            break;
        case CHECK_STATE_HEADER:
            parse_request_head(text);
            if (BAD_REQUEST == ret)
            {
                //语法错误直接结束
                return BAD_REQUEST;
            }
            else if (GET_REQUEST == ret)
            {
                //如果是GET请求体为空白，这里就可以do_request
                return do_request();
            }
            break;
        case CHECK_STATE_CONTENT:
            ret = parse_request_content(text);
            if (GET_REQUEST == ret)
            {
                return do_request();
            }
            //防止再次进入循环
            line_ret = LINE_OPEN;
            break;
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

//从状态机，依靠\r\n解析行
http_conn::LINE_STATUS http_conn::parse_line()
{
    for (; m_check_idx < m_read_idx; ++m_check_idx)
    {
        if (m_read_buf[m_check_idx] == '\r')
        {
            if (m_read_buf[m_check_idx + 1] == '\n')
            {
                m_read_buf[m_check_idx++] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            else if (m_check_idx + 1 == m_read_idx)
            {
                return LINE_OPEN;
            }
            return LINE_BAD;
        }
        else if (m_read_buf[m_check_idx] == '\n')
        {
            if (m_check_idx > 1 && m_read_buf[m_check_idx - 1] == '\r')
            {
                m_read_buf[m_check_idx - 1] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            else
            {
                return LINE_BAD;
            }
        }
    }
    return LINE_OPEN;
}

http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{

    return BAD_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_request_head(char *text)
{
    return BAD_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_request_content(char *text)
{
}
