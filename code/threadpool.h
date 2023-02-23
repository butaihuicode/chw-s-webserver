#ifndef ThreadPool_H
#define ThreadPool_H


#include <queue>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore.h>
#include "locker.h"

class ThreadPool
{
public:
    // 构造函数
    explicit ThreadPool(int t_num, int j_num=20)
    :m_threadNum(t_num), m_queMaxSize(j_num),
    m_resource(0, 0), m_resEmpty(0, m_queMaxSize), m_mtx(0,1), m_stop(false)
    {
        for(int i = 0; i < m_threadNum; ++i)
        {
            std::thread([i,this]()
            {
                while(!m_stop)
                {
                    //std::cout<<"nmsl"<<std::endl;
                    //std::cout<<"thread"<<i<<"created"<<std::endl;
                    m_resource.wait();
                    m_mtx.wait();
                    auto todoTask = m_jobQueue.front();
                    m_jobQueue.pop();

                    m_resEmpty.post();
                    m_mtx.post();

                    todoTask();
                }
            }).detach();
        }
    }
    // 析构函数
    ~ThreadPool()
    {
        m_stop = true;
    }
    // 对外接口：向线程池中添加任务
    template <class F>
    void Append(F &&task)
    {
        m_resEmpty.wait();
        m_mtx.wait();

        m_jobQueue.emplace(std::forward<F>(task));

        m_resource.post();
        m_mtx.post();
    }
    // 删除我们不需要的拷贝函数
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
private:
    int m_threadNum;
    int m_queMaxSize;
    Locker m_resource;
    Locker m_resEmpty;
    Locker m_mtx;
    bool m_stop;
    std::queue<std::function<void()>> m_jobQueue;
    std::vector<std::thread> m_workThreads;
};
#endif

/*
 //设计时候不要想别的类，只想这个类接受什么东西，怎么做
class ThreadPool
{
public:
    ThreadPool(int thread_num,int task_num=10):m_threadNum(thread_num),max_tasks(task_num),m_stop(false),m_empty_(0),m_full_(thread_num)
    {
        //thread传入的是void()型的可调用对象，lambda和函数都可以
        for(int i=0;i<m_threadNum;i++){
            std::cout<<"thread"<<i<<"created"<<std::endl;
            std::thread([this,i]()
            {
                while(!m_stop){
                    //内核线程阻塞不会导致其他线程阻塞，for继续执行
                    m_full_.wait();
                    m_lock.lock();
                    auto cur_task=m_taskQueue.front();  //因为是生产者-消费者模型，所以不用进行非空判断
                    m_taskQueue.pop();
                    m_lock.unlock();
                    m_empty_.post();
                    cur_task();
                }
            }).detach();
        }
    };
    ~ThreadPool(){
        m_stop=true;
    };
    //添加任务，传进来的是可执行对象，与bind配合使用，所以是右值引用。函数模版，调用时会自动推导
    template<typename T>
    void Append(T&& task){
        m_empty_.wait();
        m_lock.lock();
        m_taskQueue.emplace(std::forward<T>(task));      //完美转交
        m_lock.unlock();
        m_full_.post();
    };
    //删除不需要的函数
    ThreadPool(const ThreadPool&)=delete;
    ThreadPool& operator= (const ThreadPool&)=delete;

private:
    locker m_lock;
    sem m_empty_,m_full_;
    int max_tasks;
    int m_threadNum;
    bool m_stop;
    std::queue<std::function<void()> > m_taskQueue;
    std::vector<std::thread> m_pool;
};

#endif */


