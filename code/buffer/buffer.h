//
// Created by challway on 2022/11/27.
//
#ifndef VSCODE_PROJECT_BUFFER_H
#define VSCODE_PROJECT_BUFFER_H

#include <cstring>
#include <string>
#include <cassert>
#include <sys/uio.h>
#include <sys/socket.h>
#include <cerrno>

const int BUF_SIZE_INIT = 128;

//本类负责缓冲区与socketfd的交互，收发
class buffer{
public:
    buffer(int buf_size=BUF_SIZE_INIT);
    ~buffer();
    //从文件读数据
    ssize_t recvfd(int sockfd,int* m_errno);
    //写数据到文件
    ssize_t writefd(int sockfd,int* m_errno);
    //缓冲区大小
    inline size_t m_size()const{return m_end-m_elements_;}
    //缓存区中可以写入的字节数
    inline size_t writeableBytes() const{return m_size()-m_write_idx_;}
    //缓存区中可以读取的字节数
    inline size_t readableBytes() const{return m_write_idx_-m_read_idx_;}
    //缓存区中已经读取的字节数
    inline size_t haveReadBytes() const{return m_read_idx_;}
    //缓冲区中已经写入的字节数
    inline size_t haveWriteBytes() const{return m_write_idx_;}
    // 当前的读取指针的位置
    const char *curReadPtr() const {return m_elements_ + m_read_idx_;}
    // 当前的写入指针的位置
    const char *curWritePtr() const {return m_elements_ + m_write_idx_;}

    void updateReadPtr(size_t len);
    void updateWritePtr(size_t len);

    void Append(const char* str,int len);
    void Append(const std::string& str);
    void Append(const std::string&& str);

    std::string _toString();
    void _init();


private:
    //不同线程分配独立缓冲区，所以不需要原子变量
    char* m_elements_;
    char* m_end;

    //读写缓冲区是同一个，因此需要指示读写指针位置,writeidx是往缓冲区写就要增长，
    //不管是从文件读进来的，还是自己写进去要发送给文件的，Append会使得m_write_idx_增加
    //readidx就是读出去的，所以要读的就是写完的writeidx-readidx
    size_t m_read_idx_;
    size_t m_write_idx_;

    //缓冲区是可能需要扩容的，方便存储容量大的数据
    // 检查空间是否足够
    void EnsureWriteable(size_t len);
    // 获得更多内存并拷贝已有元素
    void AllocateSpace(size_t len);
    std::string m_str;
};










#endif //VSCODE_PROJECT_BUFFER_H
