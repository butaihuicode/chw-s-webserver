//
// Created by challway on 2022/12/20.
//

#ifndef USERMODEL_H
#define USERMODEL_H

#include"database.h"
#include"user.h"
#include<cstdlib>
#include<cstring>


//封装整个流程的查找和添加
class UserModel{
public:
    bool Insert(User& user);
    User Query(const std::string& name);

};

#endif USERMODEL_H
