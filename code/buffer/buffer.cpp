//
// Created by challway on 2022/11/27.
//
#include "buffer.h"

buffer::buffer(int buf_size):m_read_idx_(0),m_write_idx_(0){
    m_elements_=new char[buf_size];
    assert(m_elements_);
    m_end=m_elements_+buf_size;
}
buffer::~buffer() {
    m_write_idx_=0;
    m_read_idx_=0;
    delete []m_elements_;
}

void buffer::Append(const char *str,int len) {
    if(str==nullptr){
        //log
        return;
    }
    EnsureWriteable(len);
    for(int i=0;i<len;i++){
        m_elements_[m_write_idx_+i]=str[i];
    }
    m_write_idx_+=len;
}

void buffer::Append(const std::string &str) {
    Append(str.data(),str.size());  //.data()没/0,.c_str()有/0
}

void buffer::Append(const std::string &&str) {
    Append(str.data(),str.size());
}

//一次把数据读完,因为可能是EAGAIN，所以需要传出错误号
ssize_t buffer::recvfd(int sockfd,int* m_errno) {
    char extraBuf[65535];
    struct iovec iov[2];
    size_t writeable = writeableBytes();
    iov[0].iov_base=const_cast<char*>(curWritePtr());
    iov[0].iov_len=writeable;
    iov[1].iov_base=extraBuf;
    iov[1].iov_len=sizeof(extraBuf);
    const ssize_t len=readv(sockfd,iov,2);
    //readv按顺序写
    if(len<0){
        m_errno=&errno; // 错误号传出
    }else if(len<=writeable){
        //本缓冲区已经把所有数据读完
        m_write_idx_+=len;
    }else{
        //读到了额外缓冲区中
        m_write_idx_=m_size();
        Append(extraBuf,len-writeable);
    }
    return len;
}

ssize_t buffer::writefd(int sockfd,int* m_errno) {
    size_t remainRead=readableBytes();
    ssize_t len=send(sockfd,curReadPtr(),remainRead,0);
    if(len<0){
        //log
        m_errno=&errno;
        return len;
    }
    else{
        m_read_idx_+len;
    }
    return len;
}


void buffer::EnsureWriteable(size_t len) {
    size_t remainBytes=writeableBytes();
    if(len>remainBytes){
        AllocateSpace(len-remainBytes);
    }
    //检测扩容结果
    remainBytes=writeableBytes();
    if(len>remainBytes){
        //载入日志
    }
}
//数组扩容
void buffer::AllocateSpace(size_t len) {
    //2倍扩容
    size_t newSize=(len+m_write_idx_)*2;
    char* new_buf=new char[newSize];
    for(int i=0;i<m_write_idx_;i++){
        new_buf[i]=m_elements_[i];
    }
    delete[] m_elements_;
    m_elements_=new_buf;
    m_end=m_elements_+newSize;
}

void buffer::updateReadPtr(size_t len) {
    assert(len<=readableBytes());
    m_read_idx_+=len;
}

std::string buffer::_toString() {
    m_str=std::string(m_elements_,m_write_idx_);
    _init();
    return m_str;
}

void buffer::_init() {
    bzero(m_elements_, m_size());
    m_read_idx_ = 0;
    m_write_idx_ = 0;
}

void buffer::updateWritePtr(size_t len) {
    m_write_idx_+=len;
}


