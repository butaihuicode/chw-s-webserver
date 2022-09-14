#include"threadpool.h"
#include"locker.h"
#include<sys/socket.h>
#include<arpa/inet.h>


class http_conn
{
private:
    int m_sockfd;
    sockaddr_in m_address;
    static int m_epollfd;
    static int user_count;

    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    int m_read_idx;
    int m_write_idx;
    int m_check_idx;
    int m_start_line;

    char m_read_buf[READ_BUFFER_SIZE];
    char m_write_buf[WRITE_BUFFER_SIZE];
    

    //报文的请求方法，本项目只用到GET和POST
    enum METHOD{GET=0,POST,HEAD,PUT,DELETE,TRACE,OPTIONS,CONNECT,PATH};
    //主状态机的状态
    enum CHECK_STATE{CHECK_STATE_REQUESTLINE=0,CHECK_STATE_HEADER,CHECK_STATE_CONTENT};
    //报文解析的结果
    enum HTTP_CODE{NO_REQUEST,GET_REQUEST,BAD_REQUEST,NO_RESOURCE,FORBIDDEN_REQUEST,FILE_REQUEST,INTERNAL_ERROR,CLOSED_CONNECTION};
    //从状态机的状态
    enum LINE_STATUS{LINE_OK=0,LINE_BAD,LINE_OPEN};
    
    CHECK_STATE m_check_state;
    METHOD m_method;


public:
    http_conn(){};
    ~http_conn(){};
    static int m_epollfd;
    static int user_count;
    void init(int sockfd,sockaddr_in& address);
    void init();

    bool read_once();
    void close_conn();

    void process();
    HTTP_CODE process_read();
    int process_write();

    HTTP_CODE do_request();
    LINE_STATUS parse_line();   //从状态机解析行
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_request_head(char* text);
    HTTP_CODE parse_request_content(char* text);

    inline char* get_line(){return m_read_buf+m_start_line;}




};


