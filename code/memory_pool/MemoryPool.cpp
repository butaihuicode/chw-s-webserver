//
// Created by challway on 2023/5/31.
//

#include "MemoryPool.h"

namespace MemoryPool {

    //把block的内存释放了
    MemoryPool::~MemoryPool() {
        Slot* cur = currentBlock_;
        while(cur) {
            Slot* next = cur->next;
            // 全程没有调用构造函数只分配了空间
            // 转化为 void 指针，是因为 void 类型不需要调用析构函数,只释放空间
            operator delete(reinterpret_cast<void*>(cur));
            cur = next;
        }
    }

    void MemoryPool::init(int size) {
        slot_size_ = size;
        currentSlot_ = currentBlock_ = lastSlot_ = freeSlot_ = nullptr;
    }


    //计算补齐需要的空间
    size_t MemoryPool::padPointer(char* p, size_t align){
        //把p转成size_t，是指针大小还是空间大小
        size_t result = reinterpret_cast<size_t>(p);
        return ((align - result) % align);
    }


//申请一个新块给内存池
    Slot* MemoryPool::allocateBlock() {
        //只有reinterpret_cast能转型指针
        //申请了空间，其实就是许多Slot按序排开，因为Slot只有一个成员next，也就是加一个指针的长度就到了next的位置，再加一个指针就到了next的next的位置
        char *newBlock = reinterpret_cast<char *> (operator new(BlockSize));
        char *body = newBlock + sizeof(Slot *);
        //计算需要对其需要填补的字节数
        size_t bodyPadding = padPointer(body, slot_size_);

        Slot *useSlot;
        mutex_other_.lock();
        //新block，接到pool的头部
        reinterpret_cast<Slot *> (newBlock)->next = currentBlock_;
        currentBlock_ = reinterpret_cast<Slot *> (newBlock);
        // 为该Block开始的地方加上bodyPadding个char* 空间,改变当前slot
        currentSlot_ = reinterpret_cast<Slot *> (body + bodyPadding);
        //newBlock新地址+整个BLock的大小-Slot的大小 = 最后一个Slot的地址
        lastSlot_ = reinterpret_cast<Slot *> (newBlock + BlockSize - slot_size_ + 1);
        useSlot = currentSlot_;
        //为什么这里要移动？--因为这个是allocate调用的，算是这个Slot已经分配出去了
        //指针+1移动的是指向区域的大小，这里本意是移动slotsize大小
        //也就是指示下一个Slot的位置，前一个Slot的地址就是前面的内存，这一个SLot的地址从这里开始
        currentSlot_ += (slot_size_ >> 3);

        mutex_other_.unlock();
        return useSlot;
    }
    //
    Slot *MemoryPool::no_free_solve() {
        //如果当前Block所有Slot都被用了
        if(currentSlot_ >= lastSlot_){
            return allocateBlock();
        }
        Slot* useSlot;
        //currentslot就一定可用？-是的，current之前可能有用过的没用过的，current之后就都是没用过的
        //只要不用了就会方法哦free里，free没有才会推进currentslot
        mutex_other_.lock();
        useSlot = currentSlot_;
        currentSlot_ += (slot_size_ >> 3);
        mutex_other_.unlock();
        return useSlot;
    }
    //分配一个Slot
    Slot *MemoryPool::allocate() {
        if(freeSlot_){
            mutex_freeSlot_.lock();
            if(freeSlot_){
                Slot* result = freeSlot_;
                freeSlot_ = freeSlot_->next;
                mutex_freeSlot_.unlock();
                return result;
            }
            mutex_freeSlot_.unlock();
        }
        //没有free可用了
        return no_free_solve();
    }
    //放一个Slot回收到freeslot中
    void MemoryPool::deallocate(Slot *p) {
        if(p){
            mutex_freeSlot_.lock();
            p->next = freeSlot_;
            freeSlot_ = p;
            mutex_freeSlot_.unlock();
        }
    }
    //memoryPool数组设置为静态，这样不使用就不会创建浪费空间
    MemoryPool &get_MemoryPool(int id) {
        static MemoryPool memoryPool[64];
        assert(id < 64);
        return memoryPool[id];
    }
    //初始化pool，64个pool的slotsize分别为8，16，24，32...512
    void init_MemoryPool() {
        for(int i = 0; i < 64; ++i){
            get_MemoryPool(i).init((i + 1) << 3);
        }
    }
    //返回值是void* 是为了兼容operator new
    void *use_Memory(size_t size) {
        if(!size)
            return nullptr;
        //超过512字节了，直接使用new
        if(size > 512)
            return operator new(size);
        //不够512字节，取内存池里的一个Slot，取的下标是+7然后除以8: 1-0,9-1,17-3...
        return reinterpret_cast<void *>(get_MemoryPool(((size + 7) >> 3) - 1).allocate());
    }
    //把use了的释放掉
    void free_Memory(size_t size, void *p) {
        if(!p) return;
        //大于512,说明是new出来的
        if(size > 512){
            operator delete(p);
            return;
        }
        //分配的slot释放掉
        (get_MemoryPool(((size + 7) >> 3) - 1)).deallocate(reinterpret_cast<Slot *>(p));
    }


} // MemoryPool