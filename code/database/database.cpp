#include "database.h"


/* bool MySQL::Connect(){
    MySQLptr conn=mysql_real_connect(conn_,HOST.c_str(),USER.c_str(),
        PWD.c_str(),DBNAME.c_str(),3306,nullptr,0);
    if(conn!=nullptr)
    {   //连接成功,set names gbk防止中文输入乱码
        mysql_query(conn_,"SET NAMES gbk");
    }
    else
    {
        //log
    }
    return conn;
} */

bool MySQL::Update(const std::string &sql)
{
    //局部对象，结束后连接池资源自动释放
    //conn_是传出参数
    SqlPoolRaii spr(conn_, SqlPool::GetInstance());
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        LOG_ERROR("更新失败");
        return false;
    }
    return true;
}

MYSQL_RES *MySQL::Query(const std::string &sql)
{
    SqlPoolRaii spr(conn_, SqlPool::GetInstance());
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        LOG_ERROR("查询失败");
        return nullptr;
    }
    result_ = mysql_use_result(conn_);
    return result_;
}

MYSQL_RES *MySQL::Query(const std::string &&sql)
{
    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        // log
        return nullptr;
    }
    result_ = mysql_use_result(conn_);
    return result_;
}

void MySQL::FreeResult()
{
    mysql_free_result(result_);
    result_ = nullptr;
}

MySQLptr MySQL::GetConnection() const
{
    return conn_;
}

MySQL::~MySQL() {
    FreeResult();
}
