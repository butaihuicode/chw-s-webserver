#include "../../include/log/log.h"

Log::Log()
{
    struct timeval pt;
    gettimeofday(&pt, NULL);
    struct tm *lt = gmtime(&pt.tv_sec);
    char filePath[64] = {0};
    snprintf(filePath, sizeof(filePath) - 1, "./.log_file/%d_%02d_%02d_%02d:%02d:%02d.log",
             lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, (lt->tm_hour + 8) % 24, lt->tm_min, lt->tm_sec);
    _openFile = fopen(filePath, "w");
    if (_openFile == nullptr)
    {
        mkdir("./.log_file/", 0776);
        _openFile = fopen(filePath, "w");
    }
    // 启动日志线程
    log_thread1_ = std::thread(&Log::logThreadFunc1, this, 1);
    log_thread2_ = std::thread(&Log::logThreadFunc2, this, 1);
    // 设置线程的等级
    m_level = 1; // 默认为1：BASE等级
}
Log::~Log()
{
    fclose(_openFile);
    /*
    日志线程，只是调用这个实例的程序的一个子线程，如果调用者要结束进程了
    需要等待这个日志线程结束后再退出
    */
    if (log_thread1_.joinable()) { log_thread1_.join(); }
    if (log_thread2_.joinable()) { log_thread2_.join(); }
}
// 单例模式
Log *Log::getInstance()
{
    static Log _log;
    return &_log;
}
// 对外接口: 多个生产者
void Log::WriteMsg(int level, const char *filename, const char *func, int line,
                   const char *format, ...)
{
    if(level >= m_level){
        std::string msg;
        char date[30] = {0};
        getDate(date);
        msg.append(date);
        msg.append(" " + std::to_string(_gettid()) + " ");
        switch (level)
        {
        case 1:
        {
            msg.append("TRACE ");
            break;
        }
        case 2:
        {
            msg.append("DEBUG ");
            break;
        }
        case 3:
        {
            msg.append("INFO ");
            break;
        }
        case 4:
        {
            msg.append("WARN ");
            break;
        }
        case 5:
        {
            msg.append("ERROR ");
            break;
        }
        default:
        {
            msg.append("INFO ");
            break;
        }
        }
        va_list vaList;
        va_start(vaList, format);
        char str[256] = {0};
        vsnprintf(str, sizeof(str) - 1, format, vaList);
        va_end(vaList);
        msg.append(str);
        msg.append(" ");
        msg.append(filename);
        msg.append(":" + std::to_string(line));
        msg.append(":");
        msg.append(func);
        // 并发访问，上锁
        {
            std::unique_lock<std::mutex> lck(queue_mtx);
            while (buffer_.size() >= MAX_QUEUE_SIZE)
            {
                write_cond_.notify_one();
                queue_cond_.wait(lck);
            }
            buffer_.push(msg);
        }
    }
}

void Log::getDate(char *date)
{
    struct timeval pt;
    gettimeofday(&pt, NULL);
    struct tm *lt = gmtime(&pt.tv_sec);
    sprintf(date, "%d%02d%02d %02d:%02d:%02d.%-6ldZ",
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, (lt->tm_hour + 8) % 24, lt->tm_min, lt->tm_sec, pt.tv_usec);
}

// 将内存中的log消息写入到磁盘文件中去：单个消费者
void Log::WriteToFile()
{
    // 上锁
    std::unique_lock<std::mutex> lck(queue_mtx);
    if (buffer_.empty())
    {
        return;
    }
    while (!buffer_.empty())
    {
        std::string msg(buffer_.front());
        buffer_.pop();
        fprintf(_openFile, "%s\r\n", msg.c_str());
        fflush(_openFile);
    }
    queue_cond_.notify_all();
}

// 日志线程的工作函数
void Log::logThreadFunc1(int)
{
    while (true)
    {
        sleep(3);
        WriteToFile();
    }
}
void Log::logThreadFunc2(int)
{
    // 缓冲区满了后，就需要将日志写入到文件中
    while (true)
    {
        {
            std::unique_lock<std::mutex> lck(queue_mtx);
            if (buffer_.size() < MAX_QUEUE_SIZE)
            {
                write_cond_.wait(lck);
            }
        }
        WriteToFile();
    }
}
