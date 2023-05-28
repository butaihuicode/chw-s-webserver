//
// Created by challway on 2022/11/26.
//

#ifndef VSCODE_PROJECT_LOCKER_H
#define VSCODE_PROJECT_LOCKER_H

#include <semaphore.h>
#include <exception>
#include <stdexcept>
#include <iostream>

class Locker
{
public:
    explicit Locker(int _pshared, unsigned int value)
    {
        _init(_pshared, value);
    }
    ~Locker()
    {
        _destroy();
    }
    Locker(const Locker &) = delete;
    Locker(const Locker &&) = delete;
    Locker &operator=(const Locker &) = delete;

    bool wait()
    {
        int ret = 0;
        try
        {
            int ret = sem_wait(&m_sem);
            if (ret != 0)
            {
                throw std::runtime_error("call sem_wait() faild");
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << '\n';
        }

        return ret == 0;
    }
    bool post()
    {
        int ret = 0;
        try
        {
            int ret = sem_post(&m_sem);
            if (ret != 0)
            {
                throw std::runtime_error("call sem_post() faild");
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << '\n';
        }

        return ret == 0;
    }

private:
    sem_t m_sem;
    void _init(int _pshared, unsigned int value)
    {
        try
        {
            if (value < 0)
            {
                throw std::invalid_argument("The value cannot be less than 0");
            }
            int ret = sem_init(&m_sem, _pshared, value);
            if (ret != 0)
            {
                throw std::runtime_error("call sem_init() faild");
            }
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    void _destroy()
    {
        try
        {
            int ret = sem_destroy(&m_sem);
            if (ret != 0)
            {
                throw std::runtime_error("call sem_destroy() faild");
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
};



#include <unistd.h>
#include <stdlib.h>
#include <mutex>
#include <semaphore.h>
#include <exception>

// class mutex
class locker
{
private:
    pthread_mutex_t m_mutex;

public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
};

// class cond
class cond
{
private:
    pthread_cond_t m_cond;

public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
            throw std::exception();
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t &mutex)
    {
        return pthread_cond_wait(&m_cond, &mutex) == 0;
    }
    bool timedwait(pthread_mutex_t &mutex, timespec &t)
    {
        return pthread_cond_timedwait(&m_cond, &mutex, &t) == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond);
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond);
    }
};

// class sem
class sem
{
private:
    sem_t m_sem;

public:
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(const int &num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
    bool trywait()
    {
        return sem_trywait(&m_sem) == 0;
    }
}; 


#endif // VSCODE_PROJECT_LOCKER_H
