//
// Created by challway on 2022/12/3.
//
#include "httprequest.h"

const char HttpRequest::CRLF[] = "\r\n";

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture",
    "/upload", "/download"};

HttpRequest::HttpRequest()
{
    _init();
}

void HttpRequest::_init()
{
    m_state = REQUEST_LINE;
    m_body = m_method = m_version = m_path = "";
    m_header.clear();
    is_keep_alive_ = false;
    content_len_ = 0;
    m_user_info_.clear();
}

HTTP_CODE HttpRequest::parse(buffer &buff)
{
    while (buff.readableBytes() && m_state != FINISH)
    {
        // 如果是lineOpen还需要继续找怎么办—这里readbuf已经是完整报文了，如果还是找不到\r\n说明报文有误
        // search找出first1-last1里first2-last2的位置，找不到返回last1
        const char *lineEnd;
        std::string line;
        // BODY没有CRLF
        if (m_state != BODY)
        {
            lineEnd = std::search(buff.curReadPtr(), buff.curWritePtr(), CRLF, CRLF + 2);
            // 说明没找到CRLF，请求有误
            if (lineEnd == buff.curWritePtr())
            {
                return NO_REQUEST;
            }
            // string(const char* first,const char* last)-截取生成string
            line = std::string(buff.curReadPtr(), lineEnd);
            // 更新读指针位置
            buff.updateReadPtr(static_cast<size_t>(lineEnd - buff.curReadPtr()) + 2);
        }
        else
        {
            line = std::string(buff.curReadPtr(), buff.curWritePtr());
            m_body += line;
            if (m_body.size() < content_len_)
            {
                // 请求不完整
                return NO_REQUEST;
            }
        }
        switch (m_state)
        {
        case REQUEST_LINE:
            if (BAD_REQUEST == parse_request_line(line))
            {
                // 如果解析的不是完整行，就false
                return BAD_REQUEST;
            }
            // 这里path解析出来了，但要补充完整要访问的文件信息
            parsePath();
            break;
        case HEADERS:
            if (GET_REQUEST == parse_headers(line))
            {
                return GET_REQUEST;
            }
            break;
        case BODY:
        {
            HTTP_CODE ret = parse_body(line);
            HandleUserInfo();
            if (ret == GET_REQUEST)
            {
                // 只有成功解析headers没有body和成功解析body两种可以返回GETREQUEST
                return GET_REQUEST;
            }
            break;
        }
        default:
            break;
        }
        if (lineEnd >= buff.curWritePtr())
        {
            break;
        }
    }
    return NO_REQUEST;
}

HTTP_CODE HttpRequest::parse_request_line(const std::string &line)
{
    /* std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        m_method = subMatch[1];
        m_path = subMatch[2];
        m_version= subMatch[3];
        m_state = HEADERS;
        //接下来解析header，还不是完整请求
        return NO_REQUEST;
    }
    //如果解析不出来，说明不是完整行
    return BAD_REQUEST; */
    const char *text = line.c_str();
    char *strTmp = new char[108];
    int textIndex = 0;
    int strTmpIndex = 0;
    int seq = 0;
    while (1)
    {
        if (textIndex >= strlen(text))
        {
            strTmp[strTmpIndex] = '\0';
            // HTTP/1.1
            m_version = std::string(strTmp);
            break;
        }
        else if (seq < 2 && (text[textIndex] == ' ' || text[textIndex] == '\t'))
        {
            strTmp[strTmpIndex] = '\0';
            strTmpIndex = 0;
            // GET or /
            seq == 0 ? m_method = std::string(strTmp) : m_path = std::string(strTmp);
            seq++;
            textIndex++;
        }
        strTmp[strTmpIndex++] = text[textIndex++];
    }
    delete[] strTmp;
    // 打印测试一下
    // LOG_DEBUG("m_method:%s, m_path:%s, m_version:%s", m_method.c_str(), m_path.c_str(), m_version.c_str());
    m_state = PARSE_STATE::HEADERS;
    return NO_REQUEST;
}

HTTP_CODE HttpRequest::parse_headers(const std::string &line)
{
    /* std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    //解析出来还是Hearder继续解析，没解析出来状态转为Body
    if(regex_match(line, subMatch, patten)) {
        m_header[subMatch[1]] = subMatch[2];
        if (subMatch[1]=="Connection")
            is_keep_alive_=(subMatch[2]=="keep-alive"||subMatch[2]=="Keep-Alive");
        if (subMatch[1]=="Content-Length")
        {
            content_len_=stoi(subMatch[2]);
        }
        return NO_REQUEST;
    }
    else if(content_len_)
    {
        //如果有body
        m_state=BODY;
        return NO_REQUEST;
    }
    else
    {
        m_state=FINISH;
        //没有body，成功请求
        return GET_REQUEST;
    } */
    const char *text = line.c_str();
    if (text[0] == '\0')
    {
        m_state = PARSE_STATE::BODY;
        return NO_REQUEST;
    }

    char *head = new char[32];
    int headIdx = 0;
    while (*text != ':')
    {
        head[headIdx++] = *text;
        text++;
    }
    text += 2; // :
    head[headIdx] = '\0';
    m_header[std::string(head)] = std::string(text);
    delete[] head;
    return GET_REQUEST;
}

void HttpRequest::parsePath()
{
    if (m_path == "/")
    {
        m_path = "index.html";
    }
    else if (DEFAULT_HTML.count(m_path) > 0)
    {
        m_path += ".html";
    }
    // 客户端要访问文件列表
    else if (m_path == "/list.json")
    {
        // 获取files目录里的文件列表
        auto files = GetFiles("./files");
        Json::Value root;
        Json::Value file;
        for (int i = 0; i < (int)files.size(); i++)
        {
            file["filename"] = files[i];
            root.append(file);
        }
        WriteJson("./resources/list.json", root);
    }
}

HTTP_CODE HttpRequest::parse_body(const std::string &body)
{
    m_body = body;
    HTTP_CODE ret = NO_REQUEST;
    if ("POST" == m_method)
    {
        ret = ParsePost();
    }
    m_state = FINISH;
    return ret;
}

HTTP_CODE HttpRequest::ParsePost()
{
    // post请求的数据一般都会被编码，浏览器默认的编码格式为：application/x-www-form-urlencoded
    if ("" == m_body || m_header.count("Content-Type") == 0)
    {
        // log
        return NO_REQUEST;
    }
    // 传送大型文件时效率低下，因为要用3个字符表示一个non-ASCII字符
    if (m_method == "POST" && m_header["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseUrlEncodeed();
    }
    // 向服务器发送二进制数据，效率高
    else if (m_method == "POST" && m_header["Content-Type"].find("multipart/form-data") != std::string::npos)
    {
        ParseFormData();
        // 添加到资源列表里
        std::ofstream ofs;
        ofs.open("./resources/response.txt", std::ios::ate);
        ofs << "./files/" << m_file_info_["filename"] << std::endl;
        ofs.close();
        m_path = "/reponse.txt"; //?
    }
    return GET_REQUEST;
}

std::string HttpRequest::path_() const
{
    return m_path;
}

bool HttpRequest::is_keep_alive() const
{
    if (m_header.count("Connection"))
    {
        if (is_keep_alive_ && m_version == "HTTP/1.1")
        {
            return true;
        }
    }
    return false;
}

void HttpRequest::ParseUrlEncodeed()
{
    // urlencode表单格式是由键值对组成。键和值之间用=。多个键值对之间用&。例如：name=ZhangSan&age=16
    std::string key, value;
    int num = 0;
    int n = m_body.size();
    // key=value&key=value
    //%:hex,+:blank
    // i:right,j:left
    int i = 0, j = 0;
    for (; i < n; i++)
    {
        char ch = m_body[i];
        switch (ch)
        {
        case '=':
            key = m_body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            m_body[i] = ' ';
            break;
        case '%':
            num = ConvertHex(m_body[i + 1] * 16 + ConvertHex(m_body[i + 2]));
            m_body[i + 2] = num % 10 + '0';
            m_body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = m_body.substr(j, i - j);
            j = i + 1;
            m_user_info_[key] = value;
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    // 没有&，最后一个
    if (m_user_info_.count(key) == 0 && j < i)
    {
        value = m_body.substr(j, i - j);
        m_user_info_[key] = value;
    }
}

void HttpRequest::ParseFormData()
{
    // Content-Disposition: form-data; name="user"; filename="hello.txt"
    if (m_body.size() == 0)
        return;
    size_t start = 0, end = 0;
    // boundary前有CRLF
    end = m_body.find(CRLF);
    // boundary是一串标识符
    string boundary = m_body.substr(start, end);
    // 现在start是文件名第一个字符
    start = m_body.find("filename=\"", end) + strlen("filename=\"");
    end = m_body.find("\"", start);
    m_file_info_["filename"] = m_body.substr(start, end - start);
    // 解析文件内容,以\r\n\r\n开始
    start = m_body.find("\r\n\r\n", end) + strlen("\r\n\r\n");
    // 文件末尾也有\r\n
    end = m_body.find(boundary, start) - strlen("\r\n");
    string file_content = m_body.substr(start, end - start);
    // 将文件信息写到服务器文件
    std::ofstream ofs;
    // ate模式：每次打开均定位到末尾
    ofs.open("./files/" + m_file_info_["filename"], std::ios::ate);
    // file_content写到ofs流（文件）中
    ofs << file_content;
    ofs.close();
}

std::vector<std::string> HttpRequest::GetFiles(const std::string &dir)
{
    std::vector<std::string> files;
    DIR *dir_ptr = nullptr;
    dir_ptr = opendir(dir.c_str());
    if (nullptr == dir_ptr)
    {
        // log
        perror("Opendir Wrong");
        return files;
    }
    struct dirent *file_ptr = nullptr;
    while ((file_ptr = readdir(dir_ptr)) != nullptr)
    {
        // 避开上一级目录那个..
        if (strcmp(".", file_ptr->d_name) == 0 || strcmp("..", file_ptr->d_name) == 0)
        {
            continue;
        }
        else
        {
            files.emplace_back(file_ptr->d_name);
        }
    }
    return files;
}

void HttpRequest::WriteJson(const string &file, Json::Value root)
{
    std::ostringstream os;
    Json::StreamWriterBuilder writerBuilder;
    std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    jsonWriter->write(root, &os);

    std::ofstream ofs;
    ofs.open(file);
    assert(ofs.is_open());
    ofs << os.str();
    ofs.close();
}

// 判断register和post谁先？浏览器怎么知道是post？--网址先输入了，get到网页，点提交时候才是post
void HttpRequest::HandleUserInfo()
{
    std::string name = m_user_info_["username"];
    std::string password = m_user_info_["password"];

    User user = usermodel_.Query(name);
    if ("/register" == m_path || "/register.html" == m_path)
    {
        // 注册
        user.SetName(name);
        user.SetPassword(password);
        usermodel_.Insert(user);
        // 怎么实现网页跳转？
        m_path = "/login.html";
    }
    else if ("/login" == m_path || "/login.html" == m_path)
    {
        // post的密码和查询到的一样
        if (user.GetPassword() == password)
        {
            m_path = "/welcome.html";
        }
        else
        {
            m_path = "/error.html";
        }
    }
    else
    {
        m_path = "/error.html";
    }
}

int HttpRequest::ConvertHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch;
}
