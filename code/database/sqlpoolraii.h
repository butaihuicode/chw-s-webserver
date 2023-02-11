//
// Created by challway on 2022/12/18.
//

#ifndef HTTPSERVER_FINAL_SQLPOOLRAII_H
#define HTTPSERVER_FINAL_SQLPOOLRAII_H

#include "sqlpool.h"
#include <cassert>

typedef MYSQL *MySQLptr;
// 动机是要重复的进行get，free的操作
class SqlPoolRaii
{
public:
    SqlPoolRaii(SqlPool *pool) : pool_(pool)
    {
        assert(pool_ != nullptr);
        conn_ = pool_->GetConn();
    };
    ~SqlPoolRaii()
    {
        if (conn_)
        {
            pool_->FreeConn(conn_);
        }
    };

private:
    // 这里并没有调用构造函数，只是声明，所以可以
    SqlPool *pool_;
    MySQLptr conn_;
};

#endif // HTTPSERVER_FINAL_SQLPOOLRAII_H
