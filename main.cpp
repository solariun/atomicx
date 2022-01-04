//
//  main.cpp
//  atomic
//
//  Created by GUSTAVO CAMPOS on 28/08/2021.
//

#include <unistd.h>

#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <cstdint>
#include <iostream>
#include <setjmp.h>
#include <string>

#include "atomicx.hpp"

using namespace thread;

uint8_t nLockVar = 0;
const std::string strTopic = "message/topic";

atomicx::lock glock;

#ifdef FAKE_TIMER
uint nCounter=0;
#endif

void ListAllThreads();

atomicx_time Atomicx_GetTick (void)
{
#ifndef FAKE_TIMER
    usleep (20000);
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (atomicx_time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
#else
    nCounter++;

    return nCounter;
#endif
}

void Atomicx_SleepTick(atomicx_time nSleep)
{
//    printf ("Thread: %s, type: %u, Sleeping: %u Lock: %u/%u\n", atomic::GetCurrent()->GetName(), atomic::GetCurrent()->GetStatus(), nSleep, glock.IsLocked(), glock.IsShared());

    //ListAllThreads();
#ifndef FAKE_TIMER
    usleep ((useconds_t)nSleep * 1000);
#else
    while (nSleep); usleep(100);
#endif
}

atomicx::queue<size_t>q(5);
size_t nGlobalCount = 0;

class ThreadConsummer : public atomicx
{
public:
    ThreadConsummer() = delete;

    ThreadConsummer(atomicx_time nNice, const char* pszName) : atomicx (stack), m_pszName(pszName)
    {
        std::cout << "Creating Eventual: " << pszName << ", ID: " << (size_t) this << std::endl;

        SetNice(nNice);
    }

    ~ThreadConsummer()
    {
        std::cout << "Deleting Eventual: " << GetName() << ": " << (size_t) this << std::endl;
    }

    void run(void) noexcept override
    {
        size_t nCount=0;
        Message message={0,0};

        do
        {
            WaitBrokerMessage (strTopic.c_str(), strTopic.length(), message);
            nCount = message.message;

            std::cout << "Executing " << GetName() << ": " << (size_t) this << ", Counter: " << nCount << ", time: " << Atomicx_GetTick () << ", queue size: " << q.GetSize() << ", Lock: " << (int) glock.IsLocked() << "/" << (int) glock.IsShared() << "Message: tag: " << message.tag << ":" << message.message<< std::endl;

            std::cout << std::flush;

        } while (Yield());
    }

    void StackOverflowHandler (void) override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return m_pszName;
    }

private:
    uint8_t stack[1024]="";
    const char* m_pszName;
};

class ThreadEventual : public atomicx
{
public:
    ThreadEventual(atomicx_time nNice, const char* pszName) : atomicx (stack), m_pszName(pszName)
    {
        std::cout << "Creating Eventual: " << pszName << ", ID: " << (size_t) this << std::endl;

        SetNice(nNice);
    }

    ~ThreadEventual()
    {
        std::cout << "Deleting: " << GetName() << ": " << (size_t) this << std::endl;
    }

    void finish () noexcept override
    {
        delete this;
        std::cout << "DELETED THREAD EVENTUAL" << std::endl << std::endl << std::endl;
    }

    void run(void) noexcept override
    {
        size_t nCount=0;
        Message message={0,0};

        WaitBrokerMessage (strTopic.c_str(), strTopic.length(), message);
        nCount = message.message;

//            std::cout << "SharedLock..." << std::endl;
//            glock.SharedLock();
//
//            nCount = nGlobalCount;
//
//            Yield ();
//
//            glock.SharedUnlock();

//            std::cout << "SharedUnlock..." << std::endl;

        std::cout << "Executing " << GetName() << ": " << (size_t) this << ", Counter: " << nCount << ", time: " << Atomicx_GetTick () << ", queue size: " << q.GetSize() << ", Lock: " << (int) glock.IsLocked() << "/" << (int) glock.IsShared() << "Message: tag: " << message.tag << ":" << message.message<< std::endl;

        std::cout << std::flush;
    }

    void StackOverflowHandler (void) override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return m_pszName;
    }

private:
    uint8_t stack[1024]="";
    const char* m_pszName;
};

class Thread : public atomicx
{
public:
    Thread(atomicx_time nNice, const char* pszName) : atomicx (stack), m_pszName(pszName)
    {
        SetNice(nNice);
    }

    ~Thread()
    {
        std::cout << "Deleting " << GetName() << ": " << (size_t) this << std::endl;
    }

    void run() noexcept override
    {
        size_t nCount=0;

        do
        {
            std::cout << __FUNCTION__ << ", Executing " << GetName() << ": " << (size_t) this << ", Counter: " << nCount << ", Queue Size: " << q.GetSize() << ", Lock: " << (int) glock.IsLocked() << "/" << (int) glock.IsShared() << std::endl;

            std::cout << std::flush;

            nCount++;

            new ThreadEventual (100, "Eventual"); //Finish method from Eventual must delete the object

            //q.PushBack(nCount);

            SafePublish(strTopic.c_str(), strTopic.length(), {0,nCount});

//            std::cout << "Lock..." << std::endl;
//            glock.Lock();
//
//            nGlobalCount = nCount;
//
//            glock.Unlock();
//            std::cout << "Unlock..." << std::endl;

        }  while (Yield());

    }

    void StackOverflowHandler (void) override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return m_pszName;
    }
private:
    uint8_t stack[1024]="";
    const char* m_pszName;
};

void ListAllThreads()
{
    std::cout << "[Thread]-----------------------------------------------" << std::endl;
    std::cout << "Atomic ctx_size: " << sizeof (atomicx) << ", Lock state -> Locked: " << (bool) glock.IsLocked() << ", Shared: " << glock.IsShared() << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;

    for (auto& th : *(atomicx::GetCurrent()))
    {
        std::cout << (atomicx::GetCurrent() == &th ? "*  " : "   ") << th.GetID() << "\t" << th.GetName() << "\t, Nice: " << th.GetNice() << "\t, Stack: " << th.GetStackSize() << ", UsedStack: " << th.GetUsedStackSize() << "\t, Status: " << th.GetStatus() << ", Lock: " << (int) glock.IsLocked() << "/" << (int) glock.IsShared()  << "  LockReference: " << th.GetTagLock() << std::endl;
    }

    std::cout << "-------------------------------------------------------" << std::endl;
}

int main()
{
    q.PushBack(1);
    q.PushBack(2);
    q.PushBack(3);
    q.PushBack(4);
    q.PushBack(5);
    q.PushBack(6);
    q.PushBack(7);
    q.PushBack(8);

    q.PushFront(-1);
    q.PushFront(-2);
    q.PushFront(-3);
    q.PushFront(-5);

   // while (q.GetSize()) std::cout << "push: " << q.pop() << std::endl;

    Thread t1(500, "Producer 1");
    ThreadConsummer e1(100, "Consumer 1");
    ThreadConsummer e2(500, "Consumer 2");
    ThreadConsummer e3(300, "Consumer 3");


    std::cout << "context" << std::endl;
    {
        Thread t3_1(0, "Eventual 1");
        Thread t3_2(0, "Eventual 2");
        Thread t3_3(0, "Eventual 3");
    }
    std::cout << "end context" << std::endl;

    std::cout << "LISTING....." << std::endl;

    ListAllThreads ();

    std::cout << "END LISTING....." << std::endl;

    atomicx::Start();

    std::cout << "[FULL LOCK]-------------------------" << std::endl;

    ListAllThreads();
}
