//
// Created by challway on 2023/5/31.
//

#ifndef CLIONPROJECT_MEMORYPOOL_H
#define CLIONPROJECT_MEMORYPOOL_H

#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include <stdarg.h>
#include <utility>
#include <functional>
#define BlockSize 4096

namespace MemoryPool {

    struct Slot{
        Slot* next;
    };

    class MemoryPool{
    public:
        MemoryPool();
        ~MemoryPool();

        void init(int size);

        //分配或者收回一个元素的内存空间
        Slot* allocate();
        void deallocate(Slot* p);

    private:
        //每个槽所占的字节数
        int slot_size_;

        Slot* currentBlock_;    // 内存块链表的头指针
        Slot* currentSlot_;     // 元素链表的头指针
        Slot* lastSlot_;        // 可存放元素的最后指针
        Slot* freeSlot_;        // 元素构造后释放掉的内存链表头指针

        locker mutex_freeSlot_;
        locker mutex_other_;

        size_t padPointer(char* p, size_t align);   // 计算对齐所需空间
        Slot* allocateBlock();      // 申请内存块放进内存池（申请Block）
        Slot* no_free_solve();
    };

    MemoryPool& get_MemoryPool(int id);     //获得第id个pool

    void init_MemoryPool();
    void* use_Memory(size_t size);
    void free_Memory(size_t size, void* p);

    //实际构建对象：T是类型名，Args是构造函数参数
    template<typename T, typename ...Args>
    T* newElement(Args&&... args){
        T* p;
        if(p = reinterpret_cast<T*>(use_Memory(sizeof(T))) != nullptr){
            //调用placement new调用构造函数,地址是p,也就申请好的内存
            new (p)T(std::forward<Args>(args)...);      //完美转发
        }
        return p;
    }

    template<typename T>
    void deleteElement(T* p){
        if(p){
            p->~T();
        }
        //一定是sizeof(T)，不能是T*
        free_Memory(sizeof(T),reinterpret_cast<void *>(p));
    }



} // MemoryPool

#endif //CLIONPROJECT_MEMORYPOOL_H
