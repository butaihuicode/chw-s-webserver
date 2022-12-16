//
// Created by challway on 2022/12/15.
//
#ifndef HTTPSERVER_FINAL_TIMER_H
#define HTTPSERVER_FINAL_TIMER_H

#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cassert>

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds ms;
typedef Clock::time_point TimeStamp;
typedef std::function<void()> TimerCallback;

class TimeNode{
public:
    //用sockfd标记定时器
    int id;
    //超时时间
    TimeStamp expiredTime;
    //设置回调函数，在超时时间到了删除定时器时用以关闭连接
    //我现在设计这么个回调函数，我并不关心他是什么功能，我只知道他是一个void（）的可调用对象，到时候实现的具体实现用bind绑定
    TimerCallback cb;

    //重载<运算符
    bool operator<(TimeNode& tn) const{
        return expiredTime<tn.expiredTime;
    }
    bool operator<=(TimeNode& tn) const{
        return expiredTime<=tn.expiredTime;
    }
    bool operator>(TimeNode& tn) const{
        return expiredTime>tn.expiredTime;
    }
    bool operator>=(TimeNode& tn) const{
        return expiredTime>=tn.expiredTime;
    }

};




class TimeManager{
public:
    TimeManager();
    ~TimeManager(){Clear();}
    //添加一个定时器
    void AddTimer(int id,int timeout,const TimerCallback& cb);
    //删除一个定时器
    void DeleteTimer(int id);
    //对过时定时器进行操作
    void HandleExpiredTimer();
    //下一次来查看定时的时间--如果新链接进来的是否一定小于下一次定时的时间？
    int GetNextHandle();
    //调整过期时间
    void Update(int id,int timeout);
    // 封装删除节点和触发回调函数（结束连接等）
    void WorkId(int id);
    void WorkIndex(size_t index);

    //弹出堆顶元素
    void Pop();

    inline void Clear();


private:
    //用堆来存放定时器，用vector模拟堆
    std::vector<TimeNode> heap_;
    //往上交换
    void SwapUp(size_t i);
    //往下交换
    bool SwapDown(size_t index,size_t size);
    //交换两个节点的位置
    void SwapNode(size_t i,size_t j);
    //删除指定位置的节点
    void DelNode(size_t i);
    //映射id和下标i的关系
    std::unordered_map<int,size_t> m_pos_;



};











#endif //HTTPSERVER_FINAL_TIMER_H
