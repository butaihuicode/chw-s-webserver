//
// Created by challway on 2022/12/3.
//
#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE {
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/nsword"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".au", "audio/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css "},
        {".js", "text/javascript "},
        {"json",    "text/json"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS {
        {200, "OK"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_4XX_STATUS {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"},
};

HttpResponse::HttpResponse():m_code(-1),m_mmfile(nullptr),m_keep_alive(false),m_path(""),m_src_dir(""){
    memset(&m_stat,0,sizeof(m_stat));
}

HttpResponse::~HttpResponse()
{
    unmap();
}

void HttpResponse::_init(const std::string& path,bool is_alive,const std::string& src_dir,int code) {
    //munmap一定在m_stat重置之前,因为用到了st_size
    if(m_mmfile!=nullptr){
        unmap();
        m_mmfile=nullptr;
    }
    m_code=code;
    m_path=path;
    m_src_dir=src_dir;
    m_keep_alive=is_alive;
    memset(&m_stat,0,sizeof(m_stat));
    m_stat={0};
}
bool HttpResponse::make_response(buffer& buff) {
    std::string path=m_src_dir+m_path;
    int stat_res=stat(path.c_str(),&m_stat);
    assert(stat_res==0);
    //400badrequest在httprequest就知道了，其他的需要查找文件才知道
    if(stat_res<0||m_stat.st_mode & S_IFDIR)
    {
        m_code=404;
    }
    //其他用户不具备读权限
    else if(!(m_stat.st_mode&S_IROTH))
    {
        m_code=403;
    }
    //不能直接else，还有可能是400,BAD_REQUEST
    else if(m_code==-1||m_code==200)
    {
        m_code=200;
    }
    //如果是错误码，跳转到错误的HTML
    errorHTML();
    add_status_line(buff);
    add_header_line(buff);
    add_entity_body(buff);

    return m_code == 200;
}


void HttpResponse::add_status_line(buffer& buff) const{
    std::string status;
    if(CODE_STATUS.count(m_code))
    {
        //200,403,404
        status=CODE_STATUS.find(m_code)->second;
    }else
    {
        //400
        status=CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 "+ std::to_string(m_code)+" "+status+"\r\n");
}

void HttpResponse::add_header_line(buffer& buff)const {
    buff.Append("Connection: ");
    if(m_keep_alive){
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=60\r\n");
    }
    else{
        buff.Append("close\r\n");
    }
    //body文件的大小
    buff.Append("Content-Length: "+std::to_string(m_stat.st_size)+"\r\n");
    buff.Append("Content-Type: "+getFileType()+"\r\n\r\n");
}

void HttpResponse::add_entity_body(buffer& buff) {
    int srcFd = open((m_src_dir + m_path).data(), O_RDONLY);
    if (srcFd == -1)
    {
        error_content(buff, "File NotFound!\r\n");
        //LOG_ERROR("error:open file faild");
        return;
    }
    //内存映射，操作缓冲区相当于操作文件
    int *mmret = (int *)mmap(nullptr, m_stat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (mmret == MAP_FAILED)
    {
        error_content(buff, "File NotFound!\r\n");
        //LOG_ERROR("error:mmap failed");
        return;
    }
    m_mmfile= (char *)mmret;
    close(srcFd);
    // LOG_DEBUG("响应报文的BODY部分组装完毕");
}
//?
void HttpResponse::error_content(buffer &buff, const std::string& message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(m_code) == 1) {
        status = CODE_STATUS.find(m_code)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(m_code) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

char *HttpResponse::m_mmfilePtr(){
    return m_mmfile;
}

size_t HttpResponse::m_file_size()const{
    return m_stat.st_size;
}

void HttpResponse::unmap() {
    munmap(m_mmfile,m_stat.st_size);
}

void HttpResponse::errorHTML() {
    if(CODE_4XX_STATUS.count(m_code))
    {
        m_path=CODE_4XX_STATUS.at(m_code);
        stat((m_src_dir+m_path).data(),&m_stat);
    }
}

std::string HttpResponse::getFileType()const {
    std::string::size_type res=m_path.find_last_of('.');
    if(res==std::string::npos){
        return "text/plain";
    }
    else
    {
        //第二个参数是默认参数，子串长度直到npos
        std::string type=m_path.substr(res);
        if(SUFFIX_TYPE.count(type)>0){
            return SUFFIX_TYPE.at(type);
        }
    }
    return "text/plain";
}







