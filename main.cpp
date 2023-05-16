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

class T1 : public atomicx
{
public:
    T1(atomicx_time nNice, const char* pszName) : atomicx(1024, 250), m_pszName(pszName)
    {
        SetNice(nNice);
    }

    ~T1()
    {
        std::cout << "Deleting " << GetName() << ": " << (size_t) this << std::endl;
    }

    void run() noexcept override
    {
        size_t value = 0;

        while (Yield())
        {
	    value++;
	    std::cout << "value:" << value << ", Stack:" << GetUsedStackSize() << std::endl;
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

class T2 : public atomicx
{
public:
    T2(atomicx_time nNice, const char* pszName) : atomicx(1024, 250), m_pszName(pszName)
    {
        SetNice(nNice);
    }

    T2()
    {
        std::cout << "Deleting " << GetName() << ": " << (size_t) this << std::endl;
    }

    void run() noexcept override
    {
        size_t value = 0;
        char data[] = "DATA TO BE DISPLAYED";

        while (Yield())
        {
	    value++;
	    std::cout << "value:" << value << ", Stack:" << GetUsedStackSize() << ", DT:" << data << std::endl;
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

    T1 t1(1000, "Producer 1");
    T2 t2(1000, "Producer 1");
    //Thread t2(1000, "Producer 2");
    //Thread t3(1000, "Producer 3");
    //Thread t4(1000, "Producer 4");
    
    std::cout << "LISTING....." << std::endl;

    ListAllThreads ();    

    std::cout << "END LISTING....." << std::endl;

    atomicx::Start();

    std::cout << "[FULL LOCK]-------------------------" << std::endl;

    ListAllThreads();
}
