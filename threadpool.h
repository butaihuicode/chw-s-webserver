#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<thread>
#include<stdlib.h>
#include "locker.h"
#include <functional>
#include <queue>

//设计时候不要想别的类，只想这个类接受什么东西，怎么做
class threadpool
{
public:
    threadpool(int thread_num,int task_num=10):m_threadNum(thread_num),max_tasks(task_num),m_stop(false)
    {
        //thread传入的是void()型的可调用对象，lambda和函数都可以
        for(int i=0;i<m_threadNum;i++){
            std::thread([this]()
            {
                while(!m_stop){
                    m_sem.wait();
                    m_lock.lock();
                    auto cur_task=m_taskQueue.front();  //因为是生产者-消费者模型，所以不用进行非空判断
                    m_taskQueue.pop();
                    m_lock.unlock();
                    m_sem.post();
                    cur_task();
                }
            }).detach();
        }
    };
    ~threadpool(){
        m_stop=true;
    };
    //添加任务，传进来的是可执行对象，与bind配合使用，所以是右值引用。函数模版，调用时会自动推导
    template<typename T>
    void append(T&& task){
        m_sem.wait();
        m_lock.lock();
        m_taskQueue.emplace(std::forward<T>(task));      //完美转交
        m_lock.unlock();
        m_sem.post();
    };
    //删除不需要的函数
    threadpool(const threadpool&)=delete;
    threadpool& operator= (const threadpool&)=delete;

private:
    locker m_lock;
    sem m_sem;
    int max_tasks;
    int m_threadNum;
    bool m_stop;
    std::queue<std::function<void()>> m_taskQueue;
    std::vector<std::thread> m_pool;
};

#endif


