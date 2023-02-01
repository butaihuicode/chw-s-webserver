//
// Created by challway on 2022/11/30.
//

#ifndef HTTPSERVER_FINAL_HTTPRESPONSE_H
#define HTTPSERVER_FINAL_HTTPRESPONSE_H

#include <sys/stat.h>
#include <unordered_map>
#include <string>
#include "../buffer/buffer.h"
#include <cassert>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

class HttpResponse{
public:
    HttpResponse();
    ~HttpResponse();

    void _init(const std::string& path,bool is_alive,const std::string& src_dir,int code);

    bool make_response(buffer& buff);
    //写入文件内容错误时的body内容
    void error_content(buffer& buff,const std::string& message);
    //处理错误网页
    void errorHTML();
    //返回请求的文件类型
    std::string getFileType()const;

    void unmap();

    char* m_mmfilePtr();
    size_t m_file_size()const;

private:
    struct stat m_stat;

    bool m_keep_alive;
    //状态码
    int m_code;
    //请求的资源路径来判断是否存在
    std::string m_path;
    //源目录
    std::string m_src_dir;

    //add到buffer里，httpconn把buffer写到iov，这里只需要用buffer就行
    void add_status_line(buffer& buff)const;
    void add_header_line(buffer& buff)const;
    //添加body信息，获取共享内存指针
    //获得共享内存，直接写到iov就行了，没必要先搞到缓冲区再写到iov，缓冲区和共享内存都在内存
    void add_entity_body(buffer& buff);


    //共享内存指针
    char* m_mmfile;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_4XX_STATUS;


};


#endif //HTTPSERVER_FINAL_HTTPRESPONSE_H
