//
// Created by challway on 2022/11/30.
//
#include "httpconn.h"

bool HttpConn::isET = true;
std::atomic<int> HttpConn::userCount(0);
const char *HttpConn::m_url;

HttpConn::HttpConn() : m_sockfd(-1), is_close_(false) {}

// RAII
HttpConn::~HttpConn() { CloseConn(); }

void HttpConn::InitConn(int sockfd, struct sockaddr_in &sockaddrIn)
{
    m_sockfd = sockfd;
    m_addr_ = sockaddrIn;
    m_read_buf_._init();
    m_write_buf_._init();
    userCount++;
    is_close_ = false;
}

bool HttpConn::CloseConn()
{
    m_read_buf_._init();
    m_write_buf_._init();
    m_http_response_.unmap();
    if (!is_close_)
    {
        is_close_ = true;
        close(m_sockfd);
        m_sockfd = -1;
        --userCount;
        // log
    }
    return false;
}

ssize_t HttpConn::ReadBuffer(int *m_errno)
{
    ssize_t len = 0;
    ssize_t sum = 0;
    // 如果是ET，一次全部读完，LT只读一次，所以用do-while
    do
    {
        len = m_read_buf_.recvfd(m_sockfd, m_errno);
        // 关闭连接或者出现错误，错误号已传出
        if (len <= 0)
        {
            break;
        }
        sum += len;
    } while (isET);
    return sum;
}

bool HttpConn::HandleHttp()
{
    m_http_request_._init();
    if (m_read_buf_.readableBytes() <= 0)
    {
        return false;
    }
    HTTP_CODE ret = m_http_request_.parse(m_read_buf_);
    if (ret == GET_REQUEST)
    {
        m_http_response_._init(m_http_request_.path_(), m_http_request_.is_keep_alive(), m_url, 200);
        // 如果是长连接，初始化一下
        m_http_request_._init();
    }
    else if (ret == BAD_REQUEST)
    // 请求有误
    {
        m_http_response_._init(m_http_request_.path_(), false, m_url, 400);
    }
    else if (ret == NO_REQUEST)
    // 请求不完整，返回false，重新请求
    {
        return false;
    }
    m_http_response_.make_response(m_write_buf_);
    // void*无法接受底层const指针，用const_cast去除const属性
    // 不要因为这里是写操作就误以为填充写指针位置，写指针是向m_write_buf_写数据时增长的，这里应该是往出读
    m_iov[0].iov_base = const_cast<char *>(m_write_buf_.curReadPtr());
    m_iov[0].iov_len = m_write_buf_.readableBytes();
    // iov1是文件内容
    if (m_http_response_.m_file_size() != 0 && m_http_response_.m_mmfilePtr() != nullptr)
    {
        m_iov[1].iov_base = m_http_response_.m_mmfilePtr();
        m_iov[1].iov_len = m_http_response_.m_file_size();
    }
    return true;
}

ssize_t HttpConn::WriteBuffer(int *m_errno)
{
    ssize_t len = 0;
    ssize_t have_write = 0;
    // writev分散写m_writebuf即响应报文（iov0）和文件内容（iov1）到sockfd
    do
    {
        len = writev(m_sockfd, m_iov, 2);
        if (len < 0)
        {
            m_errno = &errno;
            return -1;
        }
        if (m_iov[0].iov_len + m_iov[1].iov_len == 0)
        {
            break;
        }
        else if (static_cast<size_t>(len) > m_iov[0].iov_len)
        {
            m_iov[1].iov_base = (char *)m_iov[1].iov_base + (len - m_iov[0].iov_len);
            m_iov[1].iov_len = m_iov[1].iov_len - len + m_iov[0].iov_len;
            // iov0填充m_write_buf_写完了，需要把m_write_buf_重置
            if (m_iov[0].iov_len)
            {
                m_write_buf_._init();
                m_iov[0].iov_len = 0;
            }
        }
        else
        {
            // 一次还没写到iov1，只写iov0还没写完，更新m_write_buf_ReadPtr指针（不是writePtr，因为wptr是写进去时候的指针，此时已在末尾）
            m_iov[0].iov_len -= len;
            m_iov[0].iov_base = (char *)m_iov[0].iov_base + len;
            m_write_buf_.updateReadPtr(len);
        }
        have_write += len;
    } while (isET || have_write > 10240);
    //?为什么是10240
    return have_write;
}
