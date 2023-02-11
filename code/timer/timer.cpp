//
// Created by challway on 2022/12/15.
//

#include "timer.h"

TimeManager::TimeManager(int timer_nums)
{
    heap_.reserve(timer_nums);
}

void TimeManager::HandleExpiredTimer()
{
    if (heap_.empty())
    {
        return;
    }
    while (!heap_.empty())
    {
        TimeNode head_node = heap_.front();
        // expiretime=timeout加当时的绝对时间  如果expire<now就超时
        if (std::chrono::duration_cast<ms>(head_node.expiredTime - Clock::now()).count() > 0)
        {
            break;
        }
        WorkIndex(0);
    }
}

int TimeManager::GetNextHandle()
{
    HandleExpiredTimer();
    if (heap_.empty())
    {
        return 0;
    }
    int res = -1;
    res = std::chrono::duration_cast<ms>(heap_.front().expiredTime - Clock::now()).count();
    if (res > 0)
    {
        return res;
    }
    else
    {
        return 0;
    }
}

void TimeManager::AddTimer(int id, int timeout, const TimerCallback &CB)
{
    assert(id >= 0);
    size_t index = -1;
    // 如果是新节点
    if (m_pos_.count(id) == 0)
    {
        index = heap_.size();
        m_pos_[id] = index;
        // 默认构造函数可以列表初始化,但要注意顺序与声明顺序一样
        heap_.push_back({id, Clock::now() + ms(timeout), CB});
        SwapUp(index);
    }
    else
    // 旧id，这个id要刷新定时
    {
        index = m_pos_[id];
        heap_[index].expiredTime = Clock::now() + ms(timeout);
        heap_[index].cb = CB;
        if (!SwapDown(index, heap_.size()))
        {
            SwapUp(index);
        }
    }
}

void TimeManager::WorkId(int id)
{
    if (m_pos_.count(id) == 0 || heap_.empty())
    {
        // log
        return;
    }
    size_t pos = m_pos_[id];
    heap_[pos].cb;
    DelTimer(id);
}

void TimeManager::WorkIndex(size_t index)
{
    if (index < 0 || index >= heap_.size() || heap_.empty())
    {
        // log
        return;
    }
    heap_[index].cb;
    DelNode(index);
}

void TimeManager::DelTimer(int id)
{
    if (m_pos_.count(id) > 0)
    {
        DelNode(m_pos_[id]);
    }
    else
    {
        // log
    }
}

void TimeManager::SwapUp(size_t i)
{
    assert(i >= 0 && i < heap_.size());
    if (i == 0)
    {
        return;
    }
    size_t j = (i - 1) / 2;
    while (j >= 0)
    {
        if (heap_[j].expiredTime > heap_[i].expiredTime)
        {
            SwapNode(i, j);
        }
        else
        {
            break;
        }
        i = j;
        j = (i - 1) / 2;
    }
}

bool TimeManager::SwapDown(size_t index, size_t size)
{
    assert(index >= 0 && index < heap_.size());
    assert(size >= 0 && size < heap_.size());
    size_t i = index;
    // j是左子树，但要保证小于n说明才存在这个元素（n是size）
    size_t j = (index * 2) + 1;
    while (j < size)
    {
        // 选出左右子树中expire最小的,因为父节点要小于两个儿子的最小值，j+1<size是为了确保右子树存在
        if (j < size - 1 && heap_[j] > heap_[j + 1])
            j++;
        // 满足条件跳出循环
        if (heap_[i] <= heap_[j])
            break;
        SwapNode(i, j);
        i = j;
        j = (i * 2) + 1;
    }
    // 返回值：如果下降过了就是true，没下降过说明不满足下降条件
    return i > index;
}

void TimeManager::DelNode(size_t index)
{
    // 将位置i的节点换到队尾，然后调整堆
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    size_t i = index, j = heap_.size() - 1;
    if (i < j)
    {
        SwapNode(i, j);
        if (!SwapDown(i, j))
        {
            SwapUp(i);
        }
    }
    // unordered_map erase方法参数可以是key
    m_pos_.erase(heap_.back().id);
    heap_.pop_back();
}

void TimeManager::SwapNode(size_t i, size_t j)
{
    std::swap(heap_[i], heap_[j]);
    // 暴力理解-把交换的直接全部换过去 即m_pos_[head_[j].id]=i;
    m_pos_[heap_[i].id] = i;
    m_pos_[heap_[j].id] = j;
}

void TimeManager::Update(int id, int timeout)
{
    assert(id > 2 && timeout >= 0);
    assert(!heap_.empty() && m_pos_.count(id) > 0);
    // now是个类静态方法（所有钟都是一份）
    heap_[m_pos_[id]].expiredTime = Clock::now() + ms(timeout);
    SwapDown(m_pos_[id], heap_.size());
}

void TimeManager::Pop()
{
    assert(!heap_.empty());
    DelNode(0);
}

void TimeManager::Clear()
{
    heap_.clear();
    m_pos_.clear();
}
