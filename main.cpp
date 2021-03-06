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

struct transf
{
    size_t Counter;
    char   pszData[80];
};

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
        size_t nReceived;

        for (;;)
        {
            transf tr = {.Counter=19, .pszData=""};
            
            if ((nReceived = Receive(nGlobalCount, (uint8_t*) &tr, (uint16_t) sizeof (tr), 1000)))
            {
                printf ("Receiver: nReceived: %zu, Counter: %zu (%s)\n", nReceived, tr.Counter, tr.pszData);
            }
            else
            {
                printf ("Receiver: failed receive...\n");
                ListAllThreads();
            }
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
    uint8_t stack[1024]="";
    const char* m_pszName;
};


class Thread : public atomicx
{
public:
    Thread(atomicx_time nNice, const char* pszName) : atomicx(1024, 250), m_pszName(pszName)
    {
        SetNice(nNice);
    }

    ~Thread()
    {
        std::cout << "Deleting " << GetName() << ": " << (size_t) this << std::endl;
    }

    void run() noexcept override
    {
        transf tr { .Counter=0, .pszData=""};
        
        for (;;)
        {
            tr.Counter ++;
            snprintf(tr.pszData, sizeof tr.pszData, "Counter: %zu", tr.Counter);
            
            if (Send (nGlobalCount, (uint8_t*) &tr, (uint16_t) sizeof (tr), 1000) == false)
            {
                printf ("Publish %zu: (%s) Failed to send data.\n", GetID(), GetName ());
            }
            else
            {
                printf ("%zu: (%s) Data Sent..\n", GetID(), GetName ());
            }
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


int main()
{

    Thread t1(1000, "Producer 1");
    //Thread t2(1000, "Producer 2");
    //Thread t3(1000, "Producer 3");
    //Thread t4(1000, "Producer 4");
    
    ThreadConsummer e1(500, "Consumer 1");
    
    std::cout << "LISTING....." << std::endl;

    ListAllThreads ();    

    std::cout << "END LISTING....." << std::endl;

    atomicx::Start();

    std::cout << "[FULL LOCK]-------------------------" << std::endl;

    ListAllThreads();
}
