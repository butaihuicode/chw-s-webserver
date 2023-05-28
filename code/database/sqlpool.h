//
// Created by challway on 2022/12/18.
//

#ifndef HTTPSERVER_FINAL_SQLPOOL_H
#define HTTPSERVER_FINAL_SQLPOOL_H

#include "../locker.h"
#include <mysql/mysql.h>
#include <queue>
#include <string>
//#include "database.h"
class MYSQL;

// 数据库配置信息,静态全局变量，只能在本源文件使用
const static std::string HOST = "127.0.0.1";
const static std::string USER = "root";
const static std::string PWD = "991005";
const static std::string DBNAME = "webserver";

typedef MYSQL* MySQLptr;
using std::string;

class SqlPool
{
public:
    static SqlPool *GetInstance();
    MySQLptr GetConn();
    void FreeConn(MySQLptr conn);

private:
    // 构造函数是私有的，外界不能通过调用构造函数创建新对象
    SqlPool();
    ~SqlPool();

    size_t queue_size_;
    bool is_close_;
    static SqlPool *single_;
    std::queue<MySQLptr> m_sql_queue_;
    sem m_empty_, m_full_;
    locker m_mutex_;

    /* 一些数据库需要的参数
    string m_host_;
    string m_user_;
    string m_dbName_;
    unsigned int m_dbPort_;
    string m_password_; */
};

#endif // HTTPSERVER_FINAL_SQLPOOL_H
