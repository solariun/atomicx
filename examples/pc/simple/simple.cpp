//
//  main.cpp
//  atomicx
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

/*
 * Define the default ticket granularity
 * to milliseconds or round tick if -DFAKE_TICKER
 * is provided on compilation
 */
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

/*
 * Sleep for few Ticks, since the default ticket granularity
 * is set to Milliseconds (if -DFAKE_TICKET provide will it will
 * be context switch countings), the thread will sleep for
 * the amount of time needed till next thread start.
 */
void Atomicx_SleepTick(atomicx_time nSleep)
{
#ifndef FAKE_TIMER
    usleep ((useconds_t)nSleep * 1000);
#else
    while (nSleep); usleep(100);
#endif
}

/*
 * Object that implements thread
 */
class SelfManagedThread : public atomicx
{
public:
    SelfManagedThread(atomicx_time nNice) : atomicx()
    {
        SetNice(nNice);
    }

    ~SelfManagedThread()
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

    void StackOverflowHandler (void) noexcept override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return "Self-Managed Thread";
    }
};

/*
 * Object that implements thread
 */
class Thread : public atomicx
{
public:
    Thread(atomicx_time nNice) : atomicx(stack)
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

    void StackOverflowHandler (void) noexcept override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return "Thread";
    }

private:
    uint8_t stack[1024]=""; //Static initialization to avoid initialization order problem
};


int main()
{
    Thread t1(200);
    Thread t2(500);

    SelfManagedThread st1(200);

    // This must creates threads and destroy on leaving {} context
    {
        Thread t3_1(0);
        Thread t3_2(0);
        Thread t3_3(0);

        // since those objects will be destroied here
        // they should never start and AtomicX should
        // transparently clean it from the execution list
    }

    Thread t4(1000);

    atomicx::Start();
}
