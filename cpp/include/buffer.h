//
// Created by tang on 2024/6/13.
//

#ifndef ANDROIDPLAYER_BUFFER_H
#define ANDROIDPLAYER_BUFFER_H
//
// Created by tang on 2024/6/13.
//
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include <iostream>
#include <memory>
#include <android/log.h>

template <typename T> //泛型
struct NODE
{
    NODE *pNext;                                                                      //指针域
    T data;                                                                           //数据域
    NODE(T data = {}, NODE *pNext = nullptr) : data(std::move(data)), pNext(pNext) {} //配合allcator_traits的construct使用的构造函数
};                                                                                    //节点数据类型
template <typename T, typename Alloc = std::allocator<NODE<T>>>
class QUEUE : private Alloc
{
private:
    NODE<T> dummy{};         //头节点
    NODE<T> *pRear = &dummy; //尾节点
    int length;              //长度
    Alloc &getalloc()
    {
        return static_cast<Alloc &>(*this);
    }

public:
    QUEUE()
    {
        pRear->pNext = nullptr;
        length = 0;
    }
    QUEUE(T val)
    {
        NODE<T> *pNew = getalloc().allocate(1);                                    //新节点分配空间
        std::allocator_traits<Alloc>::construct(getalloc(), pNew, std::move(val)); //参数一为分配器，参数二为被构造对象，第三个是值
        pRear = pRear->pNext = pNew;
        length = 1;
    }
    QUEUE(const int size, T val)
    {
        for (unsigned int i = 0; i < size; i++)
        {
            NODE<T> *pNew = getalloc().allocate(1);
            std::allocator_traits<Alloc>::construct(getalloc(), pNew, std::move(val));
            pRear = pRear->pNext = pNew;
        }
        length = size;
    }
    QUEUE(std::initializer_list<T> Args)
    {
        for (auto &&p : Args)
        {
            NODE<T> *pNew = getalloc().allocate(1);
            std::allocator_traits<Alloc>::construct(getalloc(), pNew, std::move(p));
            pRear = pRear->pNext = pNew;
        }
        length = Args.size();
    }
    QUEUE(QUEUE &rhs)
    {
        dummy = rhs.dummy;
        pRear = rhs.pRear;
        length = rhs.length;
    }
    QUEUE(QUEUE &&rhs)
    {
        dummy = std::move(rhs.dummy);
        pRear = std::exchange(rhs.pRear, nullptr);
        length = std::exchange(rhs.length, 0);
    }
    bool is_empty()
    {
        return !dummy.pNext; //如果dummy.pNext==nullptr返回true,反之返回false(!dummy.pNext==dummy.pNext==nullptr)
    }
    void show()
    {
        if (is_empty())
        {
            __android_log_print(ANDROID_LOG_ERROR, "native-log", "The Queue is empty");
            return;
        }
        else
        {
            NODE<T> *pTemp = dummy.pNext;
            while (pTemp)
            {
                std::cout << pTemp->data;
                pTemp = pTemp->pNext;
            }
            printf("\n");
        }
    }
    int size()
    {
        return length; //返回长度
    }
    T back()
    {
        return pRear->data; //返回尾节点元素
    }
    T front()
    {
        return dummy.pNext->data; //返回第一个元素
    }
    void push(T val) //在尾部添加元素
    {
        NODE<T> *pNew = getalloc().allocate(1); //分配新节点
        std::allocator_traits<Alloc>::construct(getalloc(), pNew, std::move(val));
        pRear = pRear->pNext = pNew; //将新节点与原尾节点产生逻辑关系，同时让尾指针移动到新节点处
        length++;
    }
    template <typename... Args>
    void emplace(Args &&...args)
    {
        NODE<T> *pNew = getalloc().allocate(1);
        std::allocator_traits<Alloc>::construct(getalloc(), pNew, std::forward<Args>(args)...);
        pRear = pRear->pNext = pNew;
        length++;
    }
    void pop()
    {
        if (is_empty())
        {
            return; //为空直接返回
        }
        else
        {
            if (dummy.pNext == pRear) //第一个元素节点就是尾节点时
            {
                getalloc().destroy(pRear);
                pRear = &dummy;        //将尾指针重新指向头节点
                dummy.pNext = nullptr; //头节点下一个元素置空
                length--;              //长度-1
                return;
            }
            auto del = dummy.pNext;           //留存第一个元素的地址
            dummy.pNext = dummy.pNext->pNext; //让第二个元素节点移到第一个的位置
            getalloc().destroy(del);          //删除原第一个元素节点
            length--;                         //长度--;
            return;
        }
    }
    void swap(QUEUE &rhs)
    {
        std::swap(dummy, rhs.dummy);
        std::swap(this->pRear, rhs.pRear);
        std::swap(this->length, rhs.length);
    }
    QUEUE &operator=(QUEUE rhs)
    {
        swap(rhs); //交换复制技术
        return *this;
    }
    void clear()
    {
        while (!is_empty()) //如果不为空，一直进行头部删除
        {
            pop();
        }
    }
    ~QUEUE()
    {
        while (!is_empty()) //如果不为空，一直进行头部删除
        {
            pop();
        }
    };
};

template<typename T>
class CircleBuffer
{
public:
    //构造函数
    CircleBuffer(size_t size)
    {
        m_nBufSize = size;
        m_nReadPos = 0;
        m_nWritePos = 0;
        m_pBuf = new T[m_nBufSize];
        m_bEmpty = true;
        m_bFull = false;
    }

    //析构函数
    ~CircleBuffer()
    {
        if (m_pBuf)
        {
            delete[] m_pBuf;
            m_pBuf = nullptr;
        }
    }
    //缓存区是否满
    bool isFull()
    {
        return m_bFull;
    }
    //判空
    bool isEmpty()
    {
        return m_bEmpty;
    }
    //清空缓存区
    void Clear()
    {
        m_nReadPos = 0;
        m_nWritePos = 0;
        m_bEmpty = true;
        m_bFull = false;
    }
    //获取写入内存的大小
    int GetLength()
    {
        if (m_bEmpty)
        {
            return 0;
        }
        else if (m_bFull)
        {
            return m_nBufSize;
        }
        else if (m_nReadPos < m_nWritePos)
        {
            return m_nWritePos - m_nReadPos;
        }
        else
        {
            return m_nBufSize - m_nReadPos + m_nWritePos;
        }
    }
    //向缓存区中写数据，返回实际写入的字节数
    int Write(T* buf, int count)
    {
        if (count <= 0)
            return 0;
        m_bEmpty = false;
        // 缓冲区已满，不能继续写入
        if (m_bFull)
        {
            return 0;
        }
            // 缓冲区为空时
        else if (m_nReadPos == m_nWritePos)
        {
            /*                          == 内存模型 ==
                    (empty)                    m_nReadPos                (empty)
             |----------------------------------|-----------------------------------------|
                                            m_nWritePos                             m_nBufSize
             */
            //计算剩余可写入的空间
            int leftcount = m_nBufSize - m_nWritePos;
            if (leftcount > count)
            {
                memcpy(m_pBuf + m_nWritePos, buf, count);
                //尾部位置偏移
                m_nWritePos += count;
                m_bFull = (m_nWritePos == m_nReadPos);
                return count;
            }
            else
            {
                //先把能放下的数据放进缓存区去
                memcpy(m_pBuf + m_nWritePos, buf, leftcount);
                //写指针位置偏移，如果写指针右边的区域能放下剩余数据，就偏移到cont-leftcount位置，
                //否则就偏移到读指针位置，表示缓存区满了，丢弃多余数据
                m_nWritePos = (m_nReadPos > count - leftcount) ? count - leftcount : m_nWritePos;
                //沿着结尾的位置开辟新内存写入剩余的数据
                memcpy(m_pBuf, buf + leftcount, m_nWritePos);
                m_bFull = (m_nWritePos == m_nReadPos);
                return leftcount + m_nWritePos;
            }
        }
            // 有剩余空间，写指针在读指针前面
        else if (m_nReadPos < m_nWritePos)
        {
            /*                           == 内存模型 ==
             (empty)                        (data)                            (empty)
             |-------------------|----------------------------|---------------------------|
                            m_nReadPos                m_nWritePos       (leftcount)
             */
            // 计算剩余缓冲区大小(从写入位置到缓冲区尾)
            int leftcount = m_nBufSize - m_nWritePos;
            if (leftcount > count)   // 有足够的剩余空间存放
            {
                //写入缓存区
                memcpy(m_pBuf + m_nWritePos, buf, count);
                //尾部位置偏移
                m_nWritePos += count;
                m_bFull = (m_nReadPos == m_nWritePos);
                assert(m_nReadPos <= m_nBufSize);
                assert(m_nWritePos <= m_nBufSize);
                return count;
            }
                // 写指针右边剩余空间不足以放下数据
            else
            {
                // 先填充满写指针右边的剩余空间，再看读指针左边能否放下剩余数据
                memcpy(m_pBuf + m_nWritePos, buf, leftcount);
                //写指针位置偏移，如果读指针左边的区域能放下剩余数据，就偏移到cont-leftcount位置，
                //否则就偏移到读指针位置，表示缓存区满了，丢弃多余数据
                m_nWritePos = (m_nReadPos >= count - leftcount) ? count - leftcount : m_nReadPos;
                //沿着结尾位置开辟新内存写入剩余数据
                memcpy(m_pBuf, buf + leftcount, m_nWritePos);
                m_bFull = (m_nReadPos == m_nWritePos);
                assert(m_nReadPos <= m_nBufSize);
                assert(m_nWritePos <= m_nBufSize);
                return leftcount + m_nWritePos;
            }
        }
            //读指针在写指针前面
        else
        {
            /*                          == 内存模型 ==
             (unread)                 (read)                     (unread)
             |-------------------|----------------------------|---------------------------|
                            m_nWritePos        (leftcount)         m_nReadPos
             */
            int leftcount = m_nReadPos - m_nWritePos;
            if (leftcount > count)
            {
                // 有足够的剩余空间存放
                memcpy(m_pBuf + m_nWritePos, buf, count);
                m_nWritePos += count;
                m_bFull = (m_nReadPos == m_nWritePos);
                assert(m_nReadPos <= m_nBufSize);
                assert(m_nWritePos <= m_nBufSize);
                return count;
            }
            else
            {
                // 剩余空间不足时要丢弃后面的数据
                memcpy(m_pBuf + m_nWritePos, buf, leftcount);
                m_nWritePos += leftcount;
                m_bFull = (m_nReadPos == m_nWritePos);
                assert(m_bFull);
                assert(m_nReadPos <= m_nBufSize);
                assert(m_nWritePos <= m_nBufSize);
                return leftcount;
            }
        }
    }

    //从缓冲区读数据，返回实际读取的字节数
    int Read(T* buf, int count)
    {
        if (count <= 0)
            return 0;
        m_bFull = false;
        // 缓冲区空，不能继续读取数据
        if (m_bEmpty)
        {
            return 0;
        }
            // 缓冲区满时
        else if (m_nReadPos == m_nWritePos)
        {
            /*                          == 内存模型 ==
                    (data)                    m_nReadPos                (data)
             |--------------------------------|--------------------------------------------|
                                            m_nWritePos         m_nBufSize
             */
            int leftcount = m_nBufSize - m_nReadPos;
            if (leftcount > count)
            {
                memcpy(buf, m_pBuf + m_nReadPos, count);
                m_nReadPos += count;
                m_bEmpty = (m_nReadPos == m_nWritePos);
                return count;
            }
            else
            {
                memcpy(buf, m_pBuf + m_nReadPos, leftcount);
                //如果写指针左边的区域能读出剩余数据，就偏移到count-leftcount位置，否则就偏移到
                //写指针位置，表示缓存区空了
                m_nReadPos = (m_nWritePos > count - leftcount) ? count - leftcount : m_nWritePos;
                memcpy(buf + leftcount, m_pBuf, m_nReadPos);
                m_bEmpty = (m_nReadPos == m_nWritePos);
                return leftcount + m_nReadPos;
            }
        }
            // 写指针在前(未读数据是连续的)
        else if (m_nReadPos < m_nWritePos)
        {
            /*                          == 内存模型 ==
             (read)                 (unread)                      (read)
             |-------------------|----------------------------|---------------------------|
                            m_nReadPos                m_nWritePos                     m_nBufSize
             */
            int leftcount = m_nWritePos - m_nReadPos;
            int c = (leftcount > count) ? count : leftcount;
            memcpy(buf, m_pBuf + m_nReadPos, c);
            m_nReadPos += c;
            m_bEmpty = (m_nReadPos == m_nWritePos);
            assert(m_nReadPos <= m_nBufSize);
            assert(m_nWritePos <= m_nBufSize);
            return c;
        }
            // 读指针在前
        else
        {
            /*                          == 内存模型 ==
             (unread)                (read)                      (unread)
             |-------------------|----------------------------|---------------------------|
                            m_nWritePos                  m_nReadPos                  m_nBufSize

             */
            int leftcount = m_nBufSize - m_nReadPos;
            // 需要读出的数据是连续的，在读指针右边  m_nReadPos<=count<=m_nBufSize
            if (leftcount > count)
            {
                memcpy(buf, m_pBuf + m_nReadPos, count);
                m_nReadPos += count;
                m_bEmpty = (m_nReadPos == m_nWritePos);
                assert(m_nReadPos <= m_nBufSize);
                assert(m_nWritePos <= m_nBufSize);
                return count;
            }
                // 需要读出的数据是不连续的，分别在读指针右边和写指针左边
            else
            {
                //先读出读指针右边的数据
                memcpy(buf, m_pBuf + m_nReadPos, leftcount);
                //位置偏移
                //读指针位置偏移，如果写指针左边的区域能读出剩余数据，就偏移到cont-leftcount位置，
                //否则就偏移到写指针位置，表示缓存区空了，丢弃多余数据
                m_nReadPos = (m_nWritePos >= count - leftcount) ? count - leftcount : m_nWritePos;
                //在读出写指针左边的数据
                memcpy(buf, m_pBuf, m_nReadPos);
                m_bEmpty = (m_nReadPos == m_nWritePos);
                assert(m_nReadPos <= m_nBufSize);
                assert(m_nWritePos <= m_nBufSize);
                return leftcount + m_nReadPos;
            }
        }
    }
    int GetReadPos()
    {
        return m_nReadPos;
    }
    int GetWritePos()
    {
        return m_nWritePos;
    }
private:
    bool m_bEmpty, m_bFull;
    //环形缓存区头指针
    T * m_pBuf=nullptr;
    //环形缓存区大小
    size_t m_nBufSize;
    //可读指针位置（头）
    int m_nReadPos;
    //可写指针位置（尾）
    int m_nWritePos;
};

#endif //ANDROIDPLAYER_BUFFER_H
