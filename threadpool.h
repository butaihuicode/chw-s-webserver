#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <vector>
#include <list>
#include "locker.h"
#include "http_conn.h"

#define THREAD_NUM 8

template <typename T>
class threadpool
{
public:
    threadpool(int max_request){};
    ~threadpool();

    bool append(T *request);

    static void *worker(void *arg);
    void run();

private:
    pthread_t *m_threadpool;        //线程池数组
    std::list<T *> m_request_queue; //请求队列
    int m_max_request;
    bool stop;                      //线程停止取任务标志位

    locker m_lock;
    sem m_sem;
};

template <typename T>
threadpool<T>::threadpool(int max_request = 10000) : m_max_request(max_request)
{
    if (max_request <= 0)
    {
        throw std::exception();
    }
    m_threadpool = new pthread_t[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i)
    {
        if (pthread_create(m_threadpool + i, NULL, worker, this) != 0)
        {
            delete[] m_threadpool;
            throw std::exception();
        }
        if (pthread_detach(m_threadpool[i]) != 0)
        {
            delete[] m_threadpool;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threadpool;
    stop=true;
}

template <typename T>
bool threadpool<T>::append(T *request)
{
    //取大小已经相当于访问，加锁要在size之前
    m_lock.lock();
    if (m_max_request <= m_request_queue.size())
    {
        m_lock.unlock();
        return false;
    }
    m_request_queue.push_back(task);
    m_lock.unlock();
    m_sem.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *m_arg = (threadpool *)arg;
    m_arg->run();
    return NULL;
}

template <typename T>
void threadpool<T>::run()
{
    m_sem.wait();
    m_lock.lock();
    //循环取任务，如果没有stop会死循环
    while (!stop)
    {
        if (m_request_queue.empty())
        {
            m_lock.unlock();
            continue;
        }
        T *request = m_request_queue.front();
        m_request_queue.pop_front();
        m_lock.unlock();
        if(!request){
            continue;
        }
        request->process();
    }
}

#endif