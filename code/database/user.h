//
// Created by challway on 2022/12/20.
//

#ifndef HTTPSERVER_FINAL_USER_H
#define HTTPSERVER_FINAL_USER_H
#include <string>

// ORM类，把数据库user表的信息映射到类中
class User
{
public:
    User(const std::string &name = "", const std::string &password = "") : name_(name), password_(password){};
    void SetName(const std::string &name) { name_ = name; }
    void SetPassword(const std::string &pwd) { password_ = pwd; }
    const std::string &GetName() const { return name_; }
    const std::string &GetPassword() const { return password_; }

private:
    // 表要存储的信息
    std::string name_;
    std::string password_;
};

#endif
