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

#include "atomic.hpp"

using namespace thread;

#ifdef FAKE_TIMER
uint nCounter=0;
#endif

void ListAllThreads();

atomic_time Atomic_GetTick (void)
{
#ifndef FAKE_TIMER
    usleep (20000);
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (atomic_time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
#else
    nCounter++;

    return nCounter;
#endif
}

void Atomic_SleepTick(atomic_time nSleep)
{
#ifndef FAKE_TIMER
    usleep ((useconds_t)nSleep * 1000);
#else
    while (nSleep); usleep(100);
#endif
}


class Thread : public atomic
{
public:
    Thread(atomic_time nNice, const char* pszName) : stack{}, atomic (stack), m_pszName(pszName)
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
            std::cout << __FUNCTION__ << ", Executing " << GetName() << ": " << (size_t) this << ", Counter: " << nCount << std::endl << std::flush;

            nCount++;

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
    uint8_t stack[1024];
    const char* m_pszName;
};


int main()
{
    Thread t1(200, "Producer 1");
    Thread t2(500, "Producer 2");


    std::cout << "Start contexts for thread 3" << std::endl;

    {
        Thread t3_1(0, "Eventual 3.1");
        Thread t3_2(0, "Eventual 3.2");
        Thread t3_3(0, "Eventual 3.3");
    }


    std::cout << "end context" << std::endl;

    Thread t4(1000, "Producer 4");

    atomic::Start();

}

