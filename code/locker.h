//
// Created by challway on 2022/11/26.
//

#ifndef VSCODE_PROJECT_LOCKER_H
#define VSCODE_PROJECT_LOCKER_H

#ifndef LOCKER_H
#define LOCKER_H

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

#endif


#endif //VSCODE_PROJECT_LOCKER_H