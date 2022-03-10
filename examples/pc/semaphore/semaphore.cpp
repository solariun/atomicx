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

#ifdef FAKE_TIMER
uint nCounter=0;
#endif

void ListAllThreads();

atomicx_time Atomicx_GetTick (void)
{
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (atomicx_time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

size_t nCounter = 0;

void Atomicx_SleepTick(atomicx_time nSleep)
{
#if 0
    atomicx_time nCurrent= Atomicx_GetTick ();
    std::cout << "Current: " << nCurrent << ", Sleep: " << nSleep << ", Thread time:" << atomicx::GetCurrent()->GetTargetTime () << ", Calculation:" << (atomicx::GetCurrent()->GetTargetTime () - nCurrent)<< std::endl << std::flush;
    ListAllThreads();
#endif

    usleep ((useconds_t)nSleep * 1000);
}

size_t nGlobalCount = 0;

atomicx::semaphore sem(2);

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
        while (Yield ())
        {
            if (sem.acquire (0))
            {
                std::cout << GetName () << ":" << (size_t) this << " ACQUIRED, Max: " << sem.GetMaxAcquired () << ", Acquired: " << sem.GetCount () << ", Waiting: " << sem.GetWaitCount () << std::endl << std::flush;
                std::cout << GetName () << ":" << (size_t) this << " GlobalCounter: " << nGlobalCount << std::endl << std::flush;
            }
            else
            {
                std::cout << GetName () << ":" << (size_t) this << " TIMED OUT, Max: " << sem.GetMaxAcquired () << ", Acquired: " << sem.GetCount () << ", Waiting: " << sem.GetWaitCount () << std::endl << std::flush;
            }

            Yield ();

            sem.release();
        }
    }

    void StackOverflowHandler (void) noexcept override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return m_pszName;
    }

private:
    size_t stack[128]={};
    const char* m_pszName;
};


class Thread : public atomicx
{
public:
    Thread(atomicx_time nNice, const char* pszName) : atomicx(128, 10), m_pszName(pszName)
    {
        SetNice(nNice);
    }

    ~Thread()
    {
        std::cout << "Deleting " << GetName() << ": " << (size_t) this << std::endl;
    }

    void run() noexcept override
    {
        while (Yield ())
        {
            nGlobalCount++;
        }

    }

    void StackOverflowHandler (void) noexcept override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return m_pszName;
    }
private:
    const char* m_pszName;
};



void ListAllThreads()
{
    std::cout << "[Thread]-----------------------------------------------" << std::endl;

    for (auto& th : *(atomicx::GetCurrent()))
    {
        std::cout << (atomicx::GetCurrent() == &th ? "*  " : "   ") << th.GetID() << "\t" << th.GetName() << "\t, Nc: " << th.GetNice() << "\t, Stk: " << (th.IsStackSelfManaged() ? 'A' : ' ') << th.GetStackSize() << "/i:" << th.GetStackIncreasePace() << "\t, UsedStk: " << th.GetUsedStackSize() << "\t, St: " << th.GetStatus() << "/" << th.GetSubStatus() << " TTime: " << th.GetTargetTime () << ", t:" << th.GetLastUserExecTime() << "ms" << std::endl;
    }

    std::cout << "-------------------------------------------------------" << std::endl;
}

Thread t1(500, "Producer 1");
ThreadConsummer e1(500, "Consumer 1");

int main()
{

    ThreadConsummer e2(500, "Consumer 2");
    ThreadConsummer e3(500, "Consumer 3");
    ThreadConsummer e4(500, "Consumer 4");
    ThreadConsummer e5(500, "Consumer 5");
    ThreadConsummer e6(500, "Consumer 6");

    std::cout << "end context" << std::endl;

    std::cout << "LISTING....." << std::endl;

    ListAllThreads ();

    std::cout << "END LISTING....." << std::endl;

    atomicx::Start();

    std::cout << "[FULL LOCK]-------------------------" << std::endl;

    ListAllThreads();
}
