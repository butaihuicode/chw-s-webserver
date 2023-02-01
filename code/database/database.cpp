#include "database.h"

MySQL::MySQL(){
    conn_=mysql_init(NULL);
}


MySQL::~MySQL(){
    if(conn_!=nullptr){
        mysql_close(conn_);
    }
}


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


bool MySQL::Update(const std::string& sql){
    if(mysql_query(conn_,sql.c_str())!=0){
        //log
        return false;
    }
    return true;
}

MYSQL_RES* MySQL::Query(const std::string& sql){
    if(mysql_query(conn_,sql.c_str())!=0){
        //log
        return nullptr;
    }
    result_=mysql_use_result(conn_);
    return result_;
}

MYSQL_RES* MySQL::Query(const std::string&& sql){
    if(mysql_query(conn_,sql.c_str())!=0){
        //log
        return nullptr;
    }
    result_=mysql_use_result(conn_);
    return result_;
}
 
void MySQL::FreeResult(){
    mysql_free_result(result_);
    result_=nullptr;
}

MySQLptr MySQL::GetConnection()const{
    return conn_;
}
