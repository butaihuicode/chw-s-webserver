//
// Created by challway on 2022/12/18.
//

#ifndef HTTPSERVER_FINAL_DATABASE_H
#define HTTPSERVER_FINAL_DATABASE_H

#include <mysql/mysql.h>
#include <string>

/* const static std::string HOST = "127.0.0.1";
const static std::string USER = "root";
const static std::string PWD = "991005";
const static std::string DBNAME = "webserver"; */
//本类目的是封装数据库对象和增删减改操作

typedef MYSQL* MySQLptr;
class MySQL{
public:
    MySQL();
    ~MySQL();

    //连接
    //bool Connect();
    //执行SQL语句，增删减改
    bool Update(const std::string& sql);
    //执行查询操作，返回结果集
    MYSQL_RES* Query(const std::string& sql);
    MYSQL_RES* Query(const std::string&& sql);
    void FreeResult();
    MySQLptr GetConnection()const;
    


private:
    MySQLptr conn_;
    MYSQL_RES* result_;
};





#endif //HTTPSERVER_FINAL_DATABASE_H
