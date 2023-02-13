//
// Created by challway on 2022/12/20.
//
#include "usermodel.h"
#include <cstdio>

bool UserModel::Insert(User &user)
{
    char sql[1024] = {0};
    memset(sql, 0, 1024);
    // 填充SQL语句
    sprintf(sql, "Insert Into User(username,password) values('%s','%s')",
            user.GetName().c_str(), user.GetPassword().c_str());
    MySQL mysql;
    // 不用连接，执行Update会自动从连接池取出连接，直接用就好
    bool ret = mysql.Update(std::string(sql));
    return ret;
}

User UserModel::Query(const std::string &name)
{
    char sql[1024] = {0};
    memset(sql, 0, 1024);
    sprintf(sql, "Select * from user where username='%s", name.c_str());
    MySQL mysql;
    User user;
    MYSQL_RES *res = mysql.Query(sql);
    if (res)
    {
        // mysql_row is char**
        MYSQL_ROW row = mysql_fetch_row(res);
        if(row != nullptr){
            user.SetName(row[0]);
            user.SetPassword(row[1]);
        }
    }
    mysql.FreeResult();
    return user;
}
