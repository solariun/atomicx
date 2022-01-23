//
//  atomic.cpp
//  atomic
//
//  Created by GUSTAVO CAMPOS on 29/08/2021.
//

#include "atomicx.hpp"

#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#include <stdlib.h>

extern "C"
{
    #pragma weak yield
    void yield(void)
    {}
}

namespace thread
{
    // Static initializations
    static atomicx* ms_paFirst=nullptr;
    static atomicx* ms_paLast=nullptr;
    static jmp_buf ms_joinContext{};
    static atomicx* ms_pCurrent=nullptr;

    atomicx::aiterator::aiterator(atomicx* ptr) : m_ptr(ptr)
    {}

    atomicx& atomicx::aiterator::operator*() const
    {
        return *m_ptr;
    }

    atomicx* atomicx::aiterator::operator->()
    {
        return m_ptr;
    }

    atomicx::aiterator& atomicx::aiterator::operator++()
    {
        if (m_ptr != nullptr) m_ptr = m_ptr->m_paNext;
        return *this;
    }

    atomicx::aiterator atomicx::begin() { return aiterator(ms_paFirst); }
    atomicx::aiterator atomicx::end()   { return aiterator(nullptr); }

    atomicx* atomicx::GetCurrent()
    {
        return ms_pCurrent;
    }

    void atomicx::AddThisThread()
    {
        if (ms_paFirst == nullptr)
        {
            ms_paFirst = this;
            ms_paLast = ms_paFirst;
        }
        else
        {
            this->m_paPrev = ms_paLast;
            ms_paLast->m_paNext = this;
            ms_paLast = this;
        }
    }

    bool atomicx::SelectNextThread()
    {
        atomicx* pItem = ms_paFirst;

        do
        {
            if (ms_pCurrent->m_nTargetTime == 0 && pItem->m_nTargetTime > 0)
            {
                ms_pCurrent = pItem;
            }

            if (pItem->m_aStatus == aTypes::start || pItem->m_aStatus == aTypes::now)
            {
                ms_pCurrent = pItem;
                break;
            }
            else if ((pItem->m_aStatus == aTypes::sleep || (pItem->m_aStatus == aTypes::wait && pItem->m_nTargetTime > 0)) && ms_pCurrent->m_nTargetTime >= pItem->m_nTargetTime)
            {
                ms_pCurrent = pItem;
            }

        } while ((pItem = pItem->m_paNext));

        if (ms_pCurrent == nullptr)
        {
            return false;
        }
        else
        {
            switch (ms_pCurrent->m_aStatus)
            {
                case aTypes::wait:
                    if (ms_pCurrent->m_nTargetTime > 0)
                    {
                        ms_pCurrent->m_aSubStatus = aSubTypes::timeout;
                    }
                    else
                    {
                        // A blocked wait should never come here if the system was not in deadlock.
                        return false;
                    }

                case aTypes::sleep:
                {
                    atomicx_time nCurrent = Atomicx_GetTick();

                    (void) Atomicx_SleepTick(nCurrent < ms_pCurrent->m_nTargetTime ? ms_pCurrent->m_nTargetTime - nCurrent : 0);

                    break;
                }

                case aTypes::running:
                case aTypes::start:
                case aTypes::now:
                    break;

                default:
                    return false;
            }
        }

        return true;
    }

    bool atomicx::Start(void)
    {
        if (ms_paFirst != nullptr)
        {
            static bool nRunning = true;

            ms_pCurrent = ms_paFirst;

            while (nRunning && SelectNextThread ())
            {
                if (setjmp(ms_joinContext) == 0)
                {
                    if (ms_pCurrent->m_aStatus == aTypes::start)
                    {
                        volatile uint8_t nStackStart=0;

                        ms_pCurrent->m_aStatus = aTypes::running;

                        ms_pCurrent->m_pStaskStart = &nStackStart;

                        ms_pCurrent->run();

                        ms_pCurrent->m_aStatus = aTypes::start;

                        ms_pCurrent->finish ();
                    }
                    else
                    {
                        longjmp(ms_pCurrent->m_context, 1);
                    }
                }
            }
        }

        return false;
    }

    bool atomicx::Yield(atomicx_time nSleep)
    {
        if (m_aStatus == aTypes::running)
        {
            m_aStatus = aTypes::sleep;
            m_aSubStatus = aSubTypes::ok;
            m_nTargetTime=Atomicx_GetTick() + (nSleep == 0 ? m_nice : nSleep);
        }
        else if (m_aStatus == aTypes::wait)
        {
            m_nTargetTime=nSleep > 0 ? nSleep + Atomicx_GetTick() : 0;
        }
        else
        {
            m_nTargetTime = (atomicx_time)~0;
        }

        volatile uint8_t nStackEnd=0;
        m_pStaskEnd = &nStackEnd;
        m_stacUsedkSize = (size_t) (m_pStaskStart - m_pStaskEnd);


        if (m_stacUsedkSize > m_stackSize)
        {
            /*
            * Controll the auto-stack memory
            * Note: Due to some small microcontroller
            *   does not have try/catch/throw by default
            *   I decicde to use malloc/free instead
            *   to control errors
            */
            if (m_flags.autoStack == true)
            {
                if (m_stack != nullptr)
                {
                    free ((void*) m_stack);
                }

                if ((m_stack = (uint8_t*) malloc (m_stacUsedkSize)) != nullptr)
                {
                    m_stackSize = m_stacUsedkSize;
                }
                else
                {
                    m_aStatus = aTypes::stackOverflow;
                }
            }
            else
            {
                m_aStatus = aTypes::stackOverflow;
            }

            if (m_aStatus == aTypes::stackOverflow)
            {
                (void) StackOverflowHandler();
                abort();
            }
        }

        if (m_aStatus != aTypes::stackOverflow && memcpy((void*)m_stack, (const void*) m_pStaskEnd, m_stacUsedkSize) != (void*) m_stack)
        {
            return false;
        }

        if (setjmp(m_context) == 0)
        {
            longjmp(ms_joinContext, 1);
        }
        else
        {
            ms_pCurrent->m_stacUsedkSize = (size_t) (ms_pCurrent->m_pStaskStart - &nStackEnd);
            if (memcpy((void*) ms_pCurrent->m_pStaskEnd, (const void*) ms_pCurrent->m_stack, ms_pCurrent->m_stacUsedkSize) != (void*) ms_pCurrent->m_pStaskEnd)
            {
                return false;
            }
        }

        ms_pCurrent->m_aStatus = aTypes::running;

        return true;
    }

    atomicx_time atomicx::GetNice(void)
    {
        return m_nice;
    }

    size_t atomicx::GetStackSize(void)
    {
        return m_stackSize;
    }

    size_t atomicx::GetUsedStackSize(void)
    {
        return m_stacUsedkSize;
    }

    void atomicx::RemoveThisThread()
    {
        if (m_paNext == nullptr && m_paPrev == nullptr)
        {
            ms_paFirst = nullptr;
            m_paPrev = nullptr;
            ms_pCurrent = nullptr;
        }
        else if (m_paPrev == nullptr)
        {
            m_paNext->m_paPrev = nullptr;
            ms_paFirst = m_paNext;
            ms_pCurrent = ms_paFirst;
        }
        else if (m_paNext == nullptr)
        {
            m_paPrev->m_paNext = nullptr;
            ms_paLast = m_paPrev;
            ms_pCurrent = ms_paFirst;
        }
        else
        {
            m_paPrev->m_paNext = m_paNext;
            m_paNext->m_paPrev = m_paPrev;
            ms_pCurrent = m_paNext->m_paPrev;
        }
    }

    atomicx::atomicx() : m_context{}, m_stack(nullptr)
    {
        m_flags.autoStack = true;
        m_stackSize = 0;
        AddThisThread();
    }


    atomicx::~atomicx()
    {
        RemoveThisThread();

        if (m_flags.autoStack == true && m_stack != nullptr)
        {
            free((void*)m_stack);
        }
    }

    const char* atomicx::GetName(void)
    {
        return "thread";
    }

    void atomicx::SetNice (atomicx_time nice)
    {
        m_nice = nice;
    }

    size_t atomicx::GetID(void)
    {
        return (size_t) this;
    }

    atomicx_time atomicx::GetTargetTime(void)
    {
        return m_nTargetTime;
    }

    int atomicx::GetStatus(void)
    {
        return static_cast<int>(m_aStatus);
    }

    int atomicx::GetSubStatus(void)
    {
        return static_cast<int>(m_aSubStatus);
    }

    size_t atomicx::GetReferenceLock(void)
    {
        return (size_t) m_pLockId;
    }

    size_t atomicx::GetTagLock(void)
    {
        return (size_t) m_lockMessage.tag;
    }

    void atomicx::lock::Lock()
    {
        auto pAtomic = atomicx::GetCurrent();

        if(pAtomic == nullptr) return;

        // Get exclusive lock
        while (bExclusiveLock && pAtomic->Wait(bExclusiveLock, 1));

        bExclusiveLock = true;

        // Wait all shared locks to be done
        while (nSharedLockCount && pAtomic->Wait(nSharedLockCount,2));
    }

    void atomicx::lock::Unlock()
    {
        auto pAtomic = atomicx::GetCurrent();

        if(pAtomic == nullptr) return;

        if (bExclusiveLock == true)
        {
            bExclusiveLock = false;

            // Notify Other locks procedures
            pAtomic->Notify(nSharedLockCount, 2, NotifyType::all);
            pAtomic->Notify(bExclusiveLock, 1, NotifyType::one);
        }
    }

    void atomicx::lock::SharedLock()
    {
        auto pAtomic = atomicx::GetCurrent();

        if(pAtomic == nullptr) return;

        // Wait for exclusive lock
        while (bExclusiveLock > 0 && pAtomic->Wait(bExclusiveLock, 1));

        nSharedLockCount++;

        // Notify Other locks procedures
        pAtomic->Notify (nSharedLockCount, 2, NotifyType::one);
    }

    void atomicx::lock::SharedUnlock()
    {
        auto pAtomic = atomicx::GetCurrent();

        if(pAtomic == nullptr) return;

        if (nSharedLockCount)
        {
            nSharedLockCount--;

            pAtomic->Notify(nSharedLockCount, 2, NotifyType::one);
        }
    }

    size_t atomicx::lock::IsShared()
    {
        return nSharedLockCount;
    }

    bool atomicx::lock::IsLocked()
    {
        return bExclusiveLock;
    }

    uint16_t atomicx::crc16(const uint8_t* pData, size_t nSize, uint16_t nCRC)
    {
        #define POLY 0x8408
        unsigned char nCount;
        unsigned int data;

        nCRC = ~nCRC;

        if (nSize == 0) return (~nCRC);

        do
        {
            for (nCount = 0, data = (unsigned int)0xff & *pData++; nCount < 8; nCount++, data >>= 1)
            {
                if ((nCRC & 0x0001) ^ (data & 0x0001))
                    nCRC = (nCRC >> 1) ^ POLY;
                else
                    nCRC >>= 1;
            }
        } while (--nSize);

        nCRC = ~nCRC;
        data = nCRC;
        nCRC = (nCRC << 8) | (data >> 8 & 0xff);

        return (nCRC);
    }

    uint32_t atomicx::GetTopicID (const char* pszTopic, size_t nKeyLenght)
    {
        return ((uint32_t) ((crc16 ((const uint8_t*)pszTopic, nKeyLenght, 0) << 15) | crc16 ((const uint8_t*)pszTopic, nKeyLenght, 0x8408)));
    }

    bool atomicx::HasSubscriptions (const char* pszKey, size_t nKeyLenght)
    {
        if (pszKey != nullptr && nKeyLenght > 0)
        {
            uint32_t nKeyID = GetTopicID(pszKey, nKeyLenght);

            for (auto& thr : *this)
            {
                if (nKeyID == thr.m_TopicId || IsSubscribed (pszKey, nKeyLenght))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool atomicx::HasSubscriptions (uint32_t nKeyID)
    {
        for (auto& thr : *this)
        {
            if (nKeyID == thr.m_TopicId)
            {
                return true;
            }
        }

        return false;
    }

    bool atomicx::WaitBrokerMessage (const char* pszKey, size_t nKeyLenght, Message& message)
    {
        if (pszKey != nullptr && nKeyLenght > 0)
        {
            m_aStatus = aTypes::subscription;

            m_TopicId = GetTopicID(pszKey, nKeyLenght);
            m_nTargetTime = Atomicx_GetTick();

            m_lockMessage = {0,0};

            Yield();

            message.message = m_lockMessage.message;
            message.tag = m_lockMessage.tag;

            return true;
        }

        return false;
    }

    bool atomicx::WaitBrokerMessage (const char* pszKey, size_t nKeyLenght)
    {
        if (pszKey != nullptr && nKeyLenght > 0)
        {
            m_aStatus = aTypes::subscription;

            m_TopicId = GetTopicID(pszKey, nKeyLenght);
            m_nTargetTime = Atomicx_GetTick();

            Yield();

            m_lockMessage = {0,0};

            return true;
        }

        return false;
    }

    bool atomicx::SafePublish (const char* pszKey, size_t nKeyLenght, const Message message)
    {
        size_t nCounter=0;

        if (pszKey != nullptr && nKeyLenght > 0)
        {
            uint32_t nTagId = GetTopicID(pszKey, nKeyLenght);

            for (auto& thr : *this)
            {
                if (nTagId == thr.m_TopicId)
                {
                    nCounter++;

                    thr.m_aStatus = aTypes::now;
                    thr.m_TopicId = 0;
                    thr.m_nTargetTime = Atomicx_GetTick();
                    thr.m_lockMessage.message = message.message;
                    thr.m_lockMessage.tag = message.tag;
                }

                if (thr.IsSubscribed(pszKey, nKeyLenght))
                {
                    nCounter++;

                    thr.BrokerHandler(pszKey, nKeyLenght, thr.m_lockMessage);
                }
            }
        }

        return nCounter ? true : false;
    }

    bool atomicx::Publish (const char* pszKey, size_t nKeyLenght, const Message message)
    {
        bool nReturn = SafePublish(pszKey, nKeyLenght, message);

        if (nReturn) Yield();

        return nReturn;
    }

    bool atomicx::SafePublish (const char* pszKey, size_t nKeyLenght)
    {
        size_t nCounter=0;

        if (pszKey != nullptr && nKeyLenght > 0)
        {
            uint32_t nTagId = GetTopicID(pszKey, nKeyLenght);

            for (auto& thr : *this)
            {
                if (nTagId == thr.m_TopicId)
                {
                    nCounter++;

                    thr.m_aStatus = aTypes::now;
                    thr.m_TopicId = 0;
                    thr.m_nTargetTime = Atomicx_GetTick();
                    thr.m_lockMessage = {0,0};
                }

                if (thr.IsSubscribed(pszKey, nKeyLenght))
                {
                    nCounter++;

                    thr.m_lockMessage = {0,0};
                    thr.BrokerHandler(pszKey, nKeyLenght, thr.m_lockMessage);
                }

            }

            if (nCounter) Yield();
        }

        return nCounter ? true : false;
    }

    bool atomicx::Publish (const char* pszKey, size_t nKeyLenght)
    {
        bool nReturn = SafePublish(pszKey, nKeyLenght);

        if (nReturn) Yield();

        return nReturn;
    }

    atomicx_time atomicx::GetCurrentTick(void)
    {
        return Atomicx_GetTick ();
    }

    bool atomicx::IsStackSelfManaged(void)
    {
        return m_flags.autoStack;
    }
}
