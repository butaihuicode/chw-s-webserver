//
// Created by challway on 2022/11/30.
//

#ifndef HTTPSERVER_FINAL_HTTPREQUEST_H
#define HTTPSERVER_FINAL_HTTPREQUEST_H

#include "../buffer/buffer.h"
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstring>
#include <algorithm>
#include "../database/sqlpoolraii.h"
#include "../database/usermodel.h"
#include "../database/user.h"
#include <fstream>
#include <sstream>
#include <ios>
#include <dirent.h>
//#include <json/json.h>

// 当前解析状态
enum PARSE_STATE
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH
};
// Http处理状态码
enum HTTP_CODE
{
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURSE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};

class HttpRequest
{
public:
    HttpRequest();
    ~HttpRequest() = default;

    void _init();

    HTTP_CODE parse(buffer &buff);

    // 是否长连接
    bool is_keep_alive() const;
    std::string path_() const;

private:
    // 解析请求行
    HTTP_CODE parse_request_line(const std::string &line);
    // 继续解析path
    void parsePath();
    // 解析请求头
    HTTP_CODE parse_headers(const std::string &headers);
    // 解析请求体（POST）
    HTTP_CODE parse_body(const std::string &body);
    // 解析POST请求
    HTTP_CODE ParsePost();
    // 解析UrlEncodeed格式
    void ParseUrlEncodeed();
    // 解析Form格式，文件等（二进制数据）
    void ParseFormData();
    // 处理kv数据
    void HandleUserInfo();
    // 获取文件列表
    std::vector<std::string> GetFiles(const std::string &dir);
    // 写入json文件
    //void WriteJson(const std::string &file, Json::Value root);

    PARSE_STATE m_state;
    std::string m_method, m_path, m_version, m_body;
    // 首部字段的哈希表
    std::unordered_map<std::string, std::string> m_header;
    bool is_keep_alive_;
    size_t content_len_;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    // 用户表
    std::unordered_map<std::string, std::string> m_user_info_;
    // 用户上传的文件信息
    std::unordered_map<std::string, std::string> m_file_info_;

    static const char CRLF[];
    static int ConvertHex(char ch);
    UserModel usermodel_;
};

#endif // HTTPSERVER_FINAL_HTTPREQUEST_H
