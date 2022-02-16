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

    atomicx::semaphore::semaphore(size_t nMaxShared) : m_maxShared(nMaxShared)
    {
    }

    size_t atomicx::semaphore::GetMax ()
    {
        return (size_t) ~0;
    }

    bool atomicx::semaphore::acquire(atomicx_time nTimeout)
    {
        Timeout timeout(nTimeout);

        while (m_counter >= m_maxShared)
        {
            if (timeout.IsTimedout () || GetCurrent() == nullptr || GetCurrent()->Wait (*this, 1, timeout.GetRemaining()) == false)
            {
                return false;
            }
        }

        m_counter++;

        return true;
    }

    void atomicx::semaphore::release()
    {
        if (m_counter && GetCurrent() != nullptr)
        {
            m_counter --;

            GetCurrent()->Notify (*this, 1, NotifyType::one);
        }
    }

    size_t atomicx::semaphore::GetCount ()
    {
        return m_counter;
    }

    size_t atomicx::semaphore::GetMaxAcquired ()
    {
        return m_maxShared;
    }

    size_t atomicx::semaphore::GetWaitCount ()
    {
        return GetCurrent() != nullptr ? GetCurrent()->HasWaitings (*this, 1) : 0;
    }

    // Smart Semaphore, manages the semaphore com comply with RII
    atomicx::smartSemaphore::smartSemaphore(semaphore& sem) : m_sem(sem)
    {}

    atomicx::smartSemaphore::~smartSemaphore()
    {
        release ();
    }

    bool atomicx::smartSemaphore::acquire(atomicx_time nTimeout)
    {
        if (bAcquired == false && m_sem.acquire (nTimeout))
        {
            bAcquired = true;
        }
        else
        {
            return false;
        }

        return true;
    }

    void atomicx::smartSemaphore::release()
    {
        if (bAcquired)
        {
            m_sem.release ();
        }
    }

    size_t atomicx::smartSemaphore::GetCount ()
    {
        return m_sem.GetCount ();
    }

    size_t atomicx::smartSemaphore::GetMaxAcquired ()
    {
        return m_sem.GetMaxAcquired();
    }

    size_t atomicx::smartSemaphore::GetWaitCount ()
    {
        return m_sem.GetWaitCount ();
    }

    bool atomicx::smartSemaphore::IsAcquired ()
    {
        return bAcquired;
    }

    atomicx::Timeout::Timeout () : m_timeoutValue (0)
    {
        Set (0);
    }

    atomicx::Timeout::Timeout (atomicx_time nTimeoutValue) : m_timeoutValue (0)
    {
        Set (nTimeoutValue);
    }

    void atomicx::Timeout::Set(atomicx_time nTimeoutValue)
    {
        m_timeoutValue = nTimeoutValue ? nTimeoutValue + Atomicx_GetTick () : 0;
    }

    bool atomicx::Timeout::IsTimedout()
    {
        return (m_timeoutValue == 0 || Atomicx_GetTick () < m_timeoutValue) ? false : true;
    }

    atomicx_time atomicx::Timeout::GetRemaining()
    {
        auto nNow = Atomicx_GetTick ();

        return (nNow < m_timeoutValue) ? m_timeoutValue - nNow : 0;
    }

    atomicx_time atomicx::Timeout::GetDurationSince(atomicx_time startTime)
    {
        return startTime - GetRemaining ();
    }

    // atomicx::aiterator::aiterator(atomicx* ptr) : m_ptr(ptr)
    // {}

    // atomicx& atomicx::aiterator::operator*() const
    // {
    //     return *m_ptr;
    // }

    // atomicx* atomicx::aiterator::operator->()
    // {
    //     return m_ptr;
    // }

    // atomicx::aiterator& atomicx::aiterator::operator++()
    // {
    //     if (m_ptr != nullptr) m_ptr = m_ptr->m_paNext;
    //     return *this;
    // }

    iterator<atomicx> atomicx::begin() { return iterator<atomicx>(ms_paFirst); }
    iterator<atomicx> atomicx::end()   { return iterator<atomicx>(nullptr); }

    atomicx* atomicx::operator++ (void)
    {
        return m_paNext;
    }

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

        printf ("ADDING: %zX: prev: %zX, Next: %zX\n\n", (size_t) this, (size_t) m_paPrev, (size_t) m_paNext);
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

        printf ("REMOVING: %zX: prev: %zX, Next: %zX\n\n", (size_t) this, (size_t) m_paPrev, (size_t) m_paNext);
    }

    bool atomicx::SelectNextThread()
    {
        atomicx* pItem = ms_paFirst;

        if (pItem != nullptr) do
        {
            if (pItem->m_aStatus == aTypes::stop)
            {
                continue;
            }

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

        if (ms_pCurrent->m_flags.dynamicNice == true)
        {
            ms_pCurrent->m_nice = ((ms_pCurrent->m_LastUserExecTime) + ms_pCurrent->m_nice) / 2;
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

                        ms_pCurrent->m_lastResumeUserTime = Atomicx_GetTick ();

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
        ms_pCurrent->m_LastUserExecTime = GetCurrentTick () - ms_pCurrent->m_lastResumeUserTime;

        if (ms_pCurrent->m_aStatus == aTypes::running)
        {
            ms_pCurrent->m_aStatus = aTypes::sleep;
            ms_pCurrent->m_aSubStatus = aSubTypes::ok;
            ms_pCurrent->m_nTargetTime=Atomicx_GetTick() + (nSleep == ATOMICX_TIME_MAX ? ms_pCurrent->m_nice : nSleep);
        }
        else if (ms_pCurrent->m_aStatus == aTypes::wait)
        {
            ms_pCurrent->m_nTargetTime=nSleep > 0 ? nSleep + Atomicx_GetTick() : 0;
        }
        else
        {
            ms_pCurrent->m_nTargetTime = (atomicx_time)~0;
        }

        volatile uint8_t nStackEnd=0;
        ms_pCurrent->m_pStaskEnd = &nStackEnd;
        ms_pCurrent->m_stacUsedkSize = static_cast<size_t>(ms_pCurrent->m_pStaskStart - ms_pCurrent->m_pStaskEnd + 1);

        if (ms_pCurrent->m_stacUsedkSize > ms_pCurrent->m_stackSize || ms_pCurrent->m_stack == nullptr)
        {
            /*
            * Controll the auto-stack memory
            * Note: Due to some small microcontroller
            *   does not have try/catch/throw by default
            *   I decicde to use malloc/free instead
            *   to control errors
            */

            if (ms_pCurrent->m_flags.autoStack == true)
            {
                if (ms_pCurrent->m_stack != nullptr)
                {
                    free ((void*) ms_pCurrent->m_stack);
                }

                if (ms_pCurrent->m_stacUsedkSize > ms_pCurrent->m_stackSize)
                {
                    ms_pCurrent->m_stackSize = ms_pCurrent->m_stacUsedkSize + ms_pCurrent->m_stackIncreasePace;
                }

                if ((ms_pCurrent->m_stack = (volatile uint8_t*) malloc (ms_pCurrent->m_stackSize)) == nullptr)
                {
                    ms_pCurrent->m_aStatus = aTypes::stackOverflow;
                }
            }
            else
            {
                ms_pCurrent->m_aStatus = aTypes::stackOverflow;
            }

            if (ms_pCurrent->m_aStatus == aTypes::stackOverflow)
            {
                (void) ms_pCurrent->StackOverflowHandler();
                abort();
            }
        }

        if (ms_pCurrent->m_aStatus != aTypes::stackOverflow && memcpy((void*)ms_pCurrent->m_stack, (const void*) ms_pCurrent->m_pStaskEnd, ms_pCurrent->m_stacUsedkSize) != (void*) ms_pCurrent->m_stack)
        {
            return false;
        }

        if (setjmp(ms_pCurrent->m_context) == 0)
        {
            longjmp(ms_joinContext, 1);
        }
        else
        {
            if (memcpy((void*) ms_pCurrent->m_pStaskEnd, (const void*) ms_pCurrent->m_stack, ms_pCurrent->m_stacUsedkSize) != (void*) ms_pCurrent->m_pStaskEnd)
            {
                return false;
            }

            ms_pCurrent->m_aStatus = aTypes::running;

            ms_pCurrent->m_lastResumeUserTime = Atomicx_GetTick ();
        }

        return true;
    }

    void atomicx::YieldNow ()
    {
        m_aStatus = aTypes::now;
        m_aSubStatus = aSubTypes::ok;
        Yield ();
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

    void atomicx::SetDefaultInitializations ()
    {
        m_flags.autoStack = false;
        m_flags.dynamicNice = false;
        m_flags.broadcast = false;
        m_flags.attached = true;
        
        AddThisThread();
    }

    atomicx::atomicx(size_t nStackSize, int nStackIncreasePace) : m_context{}, m_stackSize(nStackSize), m_stackIncreasePace(nStackIncreasePace), m_stack(nullptr)
    {
        SetDefaultInitializations ();

        m_flags.autoStack = true;

    }

    void atomicx::DestroyThread()
    {
        if (m_flags.attached)
        {
            RemoveThisThread();

            if (m_flags.autoStack == true && m_stack != nullptr)
            {
                free((void*)m_stack);
            }

            m_flags.attached = false;
        }
    }

    void atomicx::finish() noexcept
    {
        return;
    }

    void atomicx::Detach()
    {
        this->finish ();
        DestroyThread ();

        Yield ();
    }

    void atomicx::Restart()
    {
       this->finish ();
        m_aStatus = aTypes::start;

        Yield ();
    }

    atomicx::~atomicx()
    {
        DestroyThread ();
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

    bool atomicx::mutex::Lock(atomicx_time ttimeout)
    {
        Timeout timeout(ttimeout);

        auto pAtomic = atomicx::GetCurrent();

        if(pAtomic == nullptr) return false;

        // Get exclusive mutex
        while (bExclusiveLock) if  (! pAtomic->Wait(bExclusiveLock, 1, timeout.GetRemaining())) return false;

        bExclusiveLock = true;

        // Wait all shared locks to be done
        while (nSharedLockCount) if (! pAtomic->Wait(nSharedLockCount,2, timeout.GetRemaining())) return false;

        return true;
    }

    void atomicx::mutex::Unlock()
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

    bool atomicx::mutex::SharedLock(atomicx_time ttimeout)
    {
        Timeout timeout(ttimeout);
        auto pAtomic = atomicx::GetCurrent();

        if(pAtomic == nullptr) return false;

        // Wait for exclusive mutex
        while (bExclusiveLock > 0) if (! pAtomic->Wait(bExclusiveLock, 1, timeout.GetRemaining())) return false;

        nSharedLockCount++;

        // Notify Other locks procedures
        pAtomic->Notify (nSharedLockCount, 2, NotifyType::one);

        return true;
    }

    void atomicx::mutex::SharedUnlock()
    {
        auto pAtomic = atomicx::GetCurrent();

        if(pAtomic == nullptr) return;

        if (nSharedLockCount)
        {
            nSharedLockCount--;

            pAtomic->Notify(nSharedLockCount, 2, NotifyType::one);
        }
    }

    size_t atomicx::mutex::IsShared()
    {
        return nSharedLockCount;
    }

    bool atomicx::mutex::IsLocked()
    {
        return bExclusiveLock;
    }

    atomicx::smartMutex::smartMutex (mutex& lockObj) : m_lock(lockObj)
    {}

    atomicx::smartMutex::~smartMutex()
    {
        switch (m_lockType)
        {
            case 'L':
                m_lock.Unlock();
                break;
            case 'S':
                m_lock.SharedUnlock();
                break;
        }
    }

    bool atomicx::smartMutex::SharedLock(atomicx_time ttimeout)
    {
        bool bRet = false;

        if (m_lockType == '\0')
        {
            if (m_lock.SharedLock(ttimeout))
            {
                m_lockType = 'S';
                bRet = true;
            }
        }

        return bRet;
    }

    bool atomicx::smartMutex::Lock(atomicx_time ttimeout)
    {
        bool bRet = false;

        if (m_lockType == '\0')
        {
            if (m_lock.Lock(ttimeout))
            {
                m_lockType = 'L';
                bRet = true;
            }
        }

        return bRet;
    }

    size_t atomicx::smartMutex::IsShared()
    {
        return m_lock.IsShared();
    }

    bool atomicx::smartMutex::IsLocked()
    {
        return m_lock.IsLocked();
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

    atomicx_time atomicx::GetCurrentTick(void)
    {
        return Atomicx_GetTick ();
    }

    bool atomicx::IsStackSelfManaged(void)
    {
        return m_flags.autoStack;
    }

    atomicx_time atomicx::GetLastUserExecTime()
    {
        return m_LastUserExecTime;
    }

    void atomicx::SetStackIncreasePace(size_t nIncreasePace)
    {
        m_stackIncreasePace = nIncreasePace;
    }

    size_t atomicx::GetStackIncreasePace(void)
    {
        return m_stackIncreasePace;
    }

    void atomicx::SetDynamicNice(bool status)
    {
        m_flags.dynamicNice = status;
    }

    bool atomicx::IsDynamicNiceOn()
    {
        return m_flags.dynamicNice;
    }

    atomicx* atomicx::GetThread(size_t threadId)
    {
        for (auto& th : *(thread::atomicx::GetCurrent()))
        {
            if (reinterpret_cast<size_t>(&th) == threadId)
            {
                return &th;
            }
        }

        return nullptr;
    }

    atomicx& atomicx::GetThread (void)
    {
        return *this;
    }

    size_t  atomicx::GetThreadCount()
    {
        size_t nCounter=0;

        for (auto& th : *(thread::atomicx::GetCurrent()))
        {
            (void) th;
            nCounter++;
        }

        return nCounter;
    }

    void atomicx::SetReceiveBroadcast (bool bBroadcastStatus)
    {
        m_flags.broadcast = bBroadcastStatus;
    }

    size_t atomicx::BroadcastMessage (const size_t messageReference, const Message message)
    {
        size_t nReceived = 0;

        for (auto& thr : *this)
        {
            if (thr.m_flags.broadcast == true)
            {
                BroadcastHandler (messageReference, message);
                nReceived++;
            }
        }

        return nReceived;
    }

    void atomicx::BroadcastHandler (const size_t& messageReference, const Message& message)
    {
        (void) messageReference; // to avoid unused variable
        (void) message; // to avoid unused variable
    }

    void atomicx::Stop ()
    {
        m_aStatus = aTypes::stop;
        m_aSubStatus = aSubTypes::none;
        m_nTargetTime = 0;

        Yield();
    }

    void atomicx::Resume ()
    {
        m_aStatus = aTypes::now;
        m_aSubStatus = aSubTypes::ok;
        m_nTargetTime = 0;

        Yield();        
    }

    bool atomicx::IsStopped ()
    {
        return  m_aStatus == aTypes::stop;
    }

}
