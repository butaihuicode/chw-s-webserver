#项目描述#
在linux环境下使用C++11开发的轻量级Web服务器，实现对用户访问的并发处理。实现客户端访问服务器的图
片视频等资源，并提供上传⽂件功能。支持用户登录与注册，将用户信息存⼊数据库以对用户登录进⾏校验。经压测QPS
可达1W+
#主要工作#
1. 使用C++11实现了⾼并发HTTP服务器
2. 利用I/O多路复用技术的Epoll与线程池实现多线程的⾼并发服务器模型
3. 使用继承机制实现了Reactor模式和模拟Proactor模式
4. 用vector容器封装char，实现了⼀个可自动扩容的缓冲区
5. 利用有限状态机实现HTTP请求报⽂解析，可处理GET与POST请求
6. 基于小根堆结构实现的定时器，关闭超时的非活跃⽹络连接
7. 利用单例模式实现连接MySQL的数据库连接池，减少数据库连接建立与关闭的开销，实现了用户注册登录功能
8. 利用单例模式与阻塞队列实现异步日志系统，记录服务器运⾏状态
9. 能够处理前端发送的multi/form-data类型的post请求，实现了⽂件上传功能，并通过jsoncpp⽣成json数据，向前端发送
⽂件列表，实现⽂件展示与下载


#压力测试#
