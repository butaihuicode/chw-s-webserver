# 项目描述
在linux环境下使用C++11开发的轻量级Web服务器，实现对用户访问的并发处理。实现客户端访问服务器的图
片视频等资源，并提供上传⽂件功能。支持用户登录与注册，将用户信息存⼊数据库以对用户登录进⾏校验。经压测QPS
可达1W+

# 主要工作
1. 使用C++11实现了⾼并发HTTP服务器
2. 利用I/O多路复用技术的Epoll与线程池实现多线程的⾼并发服务器模型
3. 使用继承机制实现了Reactor模式和模拟Proactor模式
4. 用vector容器封装char，实现了⼀个可自动扩容的缓冲区
5. 利用有限状态机实现HTTP请求报⽂解析，可处理GET与POST请求
6. 基于小根堆结构实现的定时器，关闭超时的非活跃⽹络连接
7. 利用单例模式实现连接MySQL的数据库连接池，减少数据库连接建立与关闭的开销，实现了用户注册登录功能
8. 利用单例模式与阻塞队列实现异步日志系统，记录服务器运⾏状态
9. 能够处理前端发送的multi/form-data类型的post请求，实现了⽂件上传功能

# 环境要求
+ C++14
+ linux
+ MySQL

# 项目启动
```cpp
// 编译程序
./autobuild.sh
/* ./可执行文件名 端口号 服务器模式 定时时间  线程数量 触发模式 日志等级 */
./myserver 9999 1 60 8 1 3 
```

# 压力测试
### 在日志与定时器功能关闭状态下、LT+ET进行测试
|模式|10|100|1000|10000|
|-----|:-:|:-:|:-:|:-:|
|Reactor 模式|10128|11987|12175|12163|
|Proactor 模式|10232|11548|10786|9786|
#### LT+ET与ET+ET模式QPS几乎没有区别

Reactor模式
![0B2B6BEE-FD9C-4CFE-B119-A4E20B95AC8B](https://user-images.githubusercontent.com/91518739/220807554-5280f1e1-74b8-4aa6-83b2-de00aa6b0169.png)

模拟Proactor模式
![0B2B6BEE-FD9C-4CFE-B119-A4E20B95AC8B](https://user-images.githubusercontent.com/91518739/220807655-f0ed5eda-f24c-46de-b05a-caad7ec4b281.png)

目前最好结果:
![F8C5B97D-6F76-489A-AE4B-6293C890BF93](https://user-images.githubusercontent.com/91518739/220807727-f4c0fe26-b4b5-4bdb-b758-c64d5e9740c9.png)

