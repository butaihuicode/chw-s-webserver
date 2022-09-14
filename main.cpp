#include<iostream>
#include"http_conn.h"
#include"locker.h"
#include"threadpool.h"
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<sys/socket.h>

using namespace std;

#define MAX_EVENT_NUMS 65535
#define MAX_USERS 1000

#define LISTENFD_LT
int http_conn::user_count=0;
int http_conn::m_epollfd=-1;



int main(int argc,char* argv[]){
    if (argc <= 1)
    {
        cout << "input error.Please input IP" << endl;
        exit(-1);
    }
    int port=atoi(argv[1]);


    http_conn* users=new http_conn[MAX_USERS];
    threadpool<http_conn>* pool=NULL;
    try{
        pool=new threadpool<http_conn>;
    }catch(...){
        //捕获threadpool里throw的可能出现的所有错误
        exit(-1);
    }
    int listenfd = socket(AF_INET,SOCK_STREAM,NULL);
    //设置端口复用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address;
    address.sin_port=port;
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=INADDR_ANY;

    bind(listenfd,(sockaddr*)&address,sizeof(address));
    listen(listenfd,5);

    int epollfd=epoll_create(114514);
    http_conn::m_epollfd=epollfd;
    epoll_event event;
    event.events=EPOLL_CTL_ADD;
    event.data.fd=listenfd;
    epoll_ctl(epollfd,EPOLLIN,listenfd,&event);
    epoll_event events[MAX_EVENT_NUMS];
    while(1){
        int num=epoll_wait(epollfd,events,MAX_EVENT_NUMS,-1);
        if(num<0&&errno!=EINTR){
            break;
        }
        for(int i=0;i<num;i++){
            int sockfd=events[i].data.fd;        
            if(sockfd==listenfd){
                sockaddr_in sock_addr;
                socklen_t len=sizeof(sock_addr);
#ifdef LISTENFD_LT                 
                int connfd=accept(listenfd,(sockaddr*)&sock_addr,&len);
                if(http_conn::user_count>MAX_USERS){
                    close(connfd);
                    continue;
                }
                users[sockfd].init(sockfd,sock_addr);             
            }
#endif
#ifdef LISTENFD_ET 
            while(1){
                int connfd=accept(listenfd,(sockaddr*)&sock_addr,&len);
                if(http_conn::user_count>MAX_USERS){
                    close(connfd);
                    continue;
                }
                users[sockfd].init(sockfd,sock_addr);             
            }
            }
#endif            
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                users[sockfd].close_conn();
                continue;
            }
            else if(events[i].events&EPOLLIN){
                //客户端有事件到达缓冲区，开始处理逻辑
                if(users[sockfd].read_once()){
                    pool->append(users+sockfd);}
                else{
                    users[sockfd].close_conn();
                }

            }
        }

    }
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;


    return 0;

}