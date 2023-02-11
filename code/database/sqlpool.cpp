#include "sqlpool.h"

SqlPool *SqlPool::single_ = new SqlPool;

// 静态成员函数只能访问静态变量
// 饿汉模式
SqlPool *SqlPool::GetInstance()
{
    return single_;
}

SqlPool::SqlPool() : queue_size_(8), m_empty_(), m_full_(8), is_close_(false)
{
    for (size_t i = 0; i < queue_size_; i++)
    {
        MySQLptr mp = mysql_init(nullptr);
        MySQLptr conn = mysql_real_connect(mp, HOST.c_str(), USER.c_str(),
                                           PWD.c_str(), DBNAME.c_str(), 3306, nullptr, 0);
        if (conn == NULL)
        {
            // log
            // 创建失败就不能再继续下去了
            is_close_ = true;
        }
        else
        {
            m_sql_queue_.push(conn);
        }
    }
}

MySQLptr SqlPool::GetConn()
{
    MySQLptr conn = nullptr;
    if (!is_close_)
    {
        m_full_.wait();
        m_mutex_.lock();
        conn = m_sql_queue_.front();
        m_sql_queue_.pop();
        m_mutex_.unlock();
        m_empty_.post();
    }
    return conn;
}

void SqlPool::FreeConn(MySQLptr conn)
{
    if (!is_close_)
    {
        m_empty_.wait();
        m_mutex_.lock();
        m_sql_queue_.push(conn);
        m_mutex_.unlock();
        m_empty_.post();
    }
}

SqlPool::~SqlPool()
{
    is_close_ = true;
    while (!m_sql_queue_.empty())
    {
        mysql_close(m_sql_queue_.front());
        m_sql_queue_.pop();
    }
}