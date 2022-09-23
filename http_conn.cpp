#include "http_conn.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include<sys/stat.h>
#include<unistd.h>
#include<sys/mman.h>
#include<cstdarg>
#include<sys/uio.h>

#define CONNFD_ET
// #define CONNFD_LT
#define LISTENFD_LT
//#define LISTENFD_ET

using namespace std;

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

//当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
const char *doc_root = "/home/xdchww/Desktop/Cppproject/chwebserver/root";

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
    event.events = ev| EPOLLONESHOT | EPOLLET|EPOLLRDHUP;
#endif
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
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
    bytes_have_send=0;
    bytes_to_send=0;
    m_url=0;
    m_version=0;
    m_host=0;
    m_content_length=0;
    m_linger=0;
    m_file_address=0;
    cgi=0;        //是否启用的POST
    m_string=0; //存储请求头数据
    memset(m_read_buf,'\0', READ_BUFFER_SIZE);
    memset(m_write_buf,'\0', READ_BUFFER_SIZE);
    memset(m_real_file,'\0',FILENAME_MAX);
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
        if (already_read == 0)
        {
            // close_conn();
            return false;
        }
        else if (already_read == -1)
        {
            if (errno == EAGAIN | EWOULDBLOCK)
            break;
            else
            return false;
        }
        m_read_idx+=already_read;
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
        cout<<"get 1 http line:"<<text<<endl;
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
            ret=parse_request_head(text);
            if (BAD_REQUEST == ret)
            {
                //语法错误直接结束
                return BAD_REQUEST;
            }
            else if (GET_REQUEST == ret)
            {
                //如果是GET请求体为空白，这里就可以do_request
                cout<<"Get req"<<endl;
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

//解析http请求行，获得请求方法，目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    //在HTTP报文中，请求行用来说明请求类型,要访问的资源以及所使用的HTTP版本，其中各个部分之间通过\t或空格分隔。
    //请求行中最先含有空格和\t任一字符的位置并返回
    m_url=strpbrk(text," \t");

    //如果没有空格或\t，则报文格式有误
    if(!m_url)
    {
        return BAD_REQUEST;
    }

    //将该位置改为\0，用于将前面数据取出
    *m_url++='\0';

    //取出数据，并通过与GET和POST比较，以确定请求方式
    char *method=text;
    if(strcasecmp(method,"GET")==0)
        m_method=GET;
    else if(strcasecmp(method,"POST")==0)
    {
        m_method=POST;
        cgi=1;
    }
    else
        return BAD_REQUEST;

    //m_url此时跳过了第一个空格或\t字符，但不知道之后是否还有
    //将m_url向后偏移，通过查找，继续跳过空格和\t字符，指向请求资源的第一个字符
    m_url+=strspn(m_url," \t");

    //使用与判断请求方式的相同逻辑，判断HTTP版本号
    m_version=strpbrk(m_url," \t");
    if(!m_version)
        return BAD_REQUEST;
    *m_version++='\0';
    m_version+=strspn(m_version," \t");

    //仅支持HTTP/1.1,strcasecmp比较时会忽略大小写
    if(strcasecmp(m_version,"HTTP/1.1")!=0)
        return BAD_REQUEST;

    //对请求资源前7个字符进行判断
    //这里主要是有些报文的请求资源中会带有http://，这里需要对这种情况进行单独处理
    if(strncasecmp(m_url,"http://",7)==0)
    {
        m_url+=7;
        m_url=strchr(m_url,'/');
    }

    //同样增加https情况
    if(strncasecmp(m_url,"https://",8)==0)
    {
        m_url+=8;
        m_url=strchr(m_url,'/');
    }

    //一般的不会带有上述两种符号，直接是单独的/或/后面带访问资源
    if(!m_url||m_url[0]!='/')
        return BAD_REQUEST;

    //当url为/时，显示欢迎界面
    if(strlen(m_url)==1)
        strcat(m_url,"judge.html");

    //请求行处理完毕，将主状态机转移处理请求头
    m_check_state=CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//0xfffff73ee1d8 "User-Agent: Mozilla/5.0 (X11; Linux aarch64; rv:104.0) Gecko/20100101 Firefox/104.0"
http_conn::HTTP_CODE http_conn::parse_request_head(char *text)
{
    //判断是空行还是请求头
    if(text[0]=='\0')
    {
        //判断是GET还是POST请求
        if(m_content_length!=0)
        {
            //POST需要跳转到消息体处理状态
            m_check_state=CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    //解析请求头部连接字段
    else if(strncasecmp(text,"Connection:",11)==0)
    {
        text+=11;

        //跳过空格和\t字符
        text+=strspn(text," \t");
        if(strcasecmp(text,"keep-alive")==0)
        {
            //如果是长连接，则将linger标志设置为true
            m_linger=true;
        }
    }
    //解析请求头部内容长度字段
    else if(strncasecmp(text,"Content-length:",15)==0)
    {
        text+=15;
        text+=strspn(text," \t");
        m_content_length=atol(text);
    }
    //解析请求头部HOST字段
    else if(strncasecmp(text,"Host:",5)==0)
    {
        text+=5;
        text+=strspn(text," \t");
        m_host=text;
    }
    else{
        printf("oop!unknow header: %s\n",text);
    }
    return NO_REQUEST;
}

//判断http请求是否被完整读入
http_conn::HTTP_CODE http_conn::parse_request_content(char *text)
{
    //判断buffer中是否读取了消息体
    if(m_read_idx>=(m_content_length+m_check_idx)){

        text[m_content_length]='\0';

        //POST请求中最后为输入的用户名和密码
        m_string = text;

        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_request(){
    strcpy(m_real_file,doc_root);
    int len=strlen(m_real_file);
    // /后面是想访问的文件，处理时前面要加上m_real_file
    const char* p= strrchr(m_url,'/');
    if(cgi==1 && (*(p+1) == '9' || *(p+1) == '8'))
    {
        //根据标志判断是登录检测还是注册检测

        //同步线程登录校验

        //CGI多进程登录校验
    }
    //0,1是注册登录界面
    if (*(p + 1) == '0')
    {
        char *m_url_real = new char[200];
        strcpy(m_url_real, "/register.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        delete[] m_url_real;
    }
    else if(*(p+1)=='1'){
        char *m_url_real = new char[200];
        strcpy(m_url_real, "/log.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        delete[] m_url_real;
    }
     else if(*(p+1)=='2'){
        char *m_url_real = new char[200];
        strcpy(m_url_real, "/video.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        delete[] m_url_real;
    } else if(*(p+1)=='3'){
        char *m_url_real = new char[200];
        strcpy(m_url_real, "/picture.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        delete[] m_url_real;
    } else if(*(p+1)=='4'){
        char *m_url_real = new char[200];
        strcpy(m_url_real, "/welcome.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        delete[] m_url_real;
    } else if(*(p+1)=='5'){
        char *m_url_real = new char[200];
        strcpy(m_url_real, "/judge.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));
        delete[] m_url_real;
    }
    else
    //正常欢迎界面
    {
        strncpy(m_real_file+len,m_url,FILENAME_LEN-len-1);
    }
    if(stat(m_real_file,&m_file_stat)<0){
        return NO_RESOURCE;
    }

    //如果是不可读文件
    if(!(m_file_stat.st_mode&S_IROTH)){
        return FORBIDDEN_REQUEST;
    }
    //如果是目录
    if(m_file_stat.st_mode&S_IFDIR){
        return BAD_REQUEST;
    }
    int fd=open(m_real_file,O_RDONLY);
    m_file_address=(char*)mmap(NULL,m_file_stat.st_size,PROT_READ,MAP_SHARED,fd,0);
    close(fd);
    return FILE_REQUEST;
}

//利用可变参数列表实现状态行向写缓冲区中的更新,format是输出格式
bool http_conn::add_response(const char* format,...){
    if(m_write_idx>WRITE_BUFFER_SIZE){
        return false;
    }
    va_list arg_list;
    va_start(arg_list,format);
    //将可变参数格式化输出（按照format的格式输出）到写缓冲区里,返回生成字符串的长度
    int len=vsnprintf(m_write_buf+m_write_idx,WRITE_BUFFER_SIZE-m_write_idx-1,format,arg_list);
    if(len>=(WRITE_BUFFER_SIZE-1-m_write_idx)){
        va_end(arg_list);
        return false;
    }
    m_write_idx+=len;
    va_end(arg_list);
    return true;
}

//写回状态行
bool http_conn::add_status_line(int status,const char* titles){
    return add_response("%s %d %s\r\n","HTTP/1.1",status,titles);
}
//添加Content-Length，表示响应报文的长度
bool http_conn::add_headers(int content_length){
    add_content_length(content_length);
    add_linger();
    add_blank_line();
    return true;
}
bool http_conn::add_content_length(int content_len){
    return add_response("Content-Length:%d\r\n",content_len);
}

//添加文本类型，这里是html
bool http_conn::add_content_type()
{
    return add_response("Content-Type:%s\r\n","text/html");
}

//添加连接状态，通知浏览器端是保持连接还是关闭
bool http_conn::add_linger()
{
    return add_response("Connection:%s\r\n",(m_linger==true)?"keep-alive":"close");
}
//添加空行
bool http_conn::add_blank_line()
{
    return add_response("%s","\r\n");
}

//添加文本content
bool http_conn::add_content(const char* content)
{
    return add_response("%s",content);
}

bool http_conn::process_write(http_conn::HTTP_CODE ret){
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500,error_500_title);
        add_headers(strlen(error_500_form));
        if(add_content(error_500_form)!=1){
            return false;
        }
        break;
    }
    case BAD_REQUEST:
    {
        add_status_line(404,error_404_title);
        add_headers(strlen(error_404_form));
        if(add_content(error_404_form)!=1){
            return false;
        }
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_status_line(403,error_403_title);
        add_headers(strlen(error_403_form));
        if(!add_content(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        add_status_line(200,ok_200_title);
        //如果请求的资源存在
        if(m_file_stat.st_size!=0){
            //把文件内容当作content发送
            add_headers(m_file_stat.st_size);
            //第一个iovec指针指向m_write_buf，长度指向m_write_idx
            m_iv[0].iov_base=m_write_buf;
            m_iv[0].iov_len=m_write_idx;
            //第二个iovec指针指向mmap返回的文件指针，长度指向文件大小
            m_iv[1].iov_base=m_file_address;
            m_iv[1].iov_len=m_file_stat.st_size;
            m_iv_count=2;
            //写缓冲里是报文的信息，要发送的内容是m_file
            bytes_to_send=m_write_idx+m_file_stat.st_size;
            return true;
        }else{
            const char* ok_string="<html><body></body></html>";
            add_headers(strlen(ok_string));
            if(!add_content(ok_string))
                return false; 
        } 
    }
    default:
        return false;
    }
    //剩余未处理报文
    m_iv[0].iov_base=m_write_buf;
    m_iv[0].iov_len=m_write_idx;
    m_iv_count=1;
    bytes_to_send = m_write_idx;
    return true;
}

bool http_conn::write(){
    int temp=0;
    int newadd=0;
    while(1){
        temp=writev(m_sockfd,m_iv,m_iv_count);
        if(temp){
            bytes_have_send+=temp;
            //写入数据的“结束点”可能位于一个iovec的中间某个位置，因此需要调整临界iovec的io_base和io_len
            newadd=bytes_have_send-m_write_idx;
        }
        else if(temp<=-1){
            //缓冲区满了会返回EAGIN
            if(errno==EAGAIN|EWOULDBLOCK){
                //如果是报文写满了
                if(bytes_have_send>=m_iv[0].iov_len){
                    m_iv->iov_len=0;        //不再发送iv1
                    m_iv[1].iov_base=m_file_address+newadd;
                    m_iv[1].iov_len=bytes_to_send;
                }
            }
            else
                //iv是按顺序写的，0写完之前不会写1
                {
                    m_iv[0].iov_base = m_write_buf + bytes_to_send;
                    m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
                }
                //没写完，重新注册写事件
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
        }
        //如果发送失败，但不是缓冲区问题，取消映射
        unmap();
        return false;
    }
    bytes_to_send -= temp;
    if (bytes_to_send <= 0)
        {
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN);

            if (m_linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
}
}

void http_conn::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    std::cout<<"解析请求，生成相应"<<std::endl;
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
    }
    std::cout<<"写入响应"<<std::endl;
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}
