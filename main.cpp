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
        size_t nMessage = 0;

        do
        {
            if (Wait (nMessage, nCounter, (size_t) 1) == false)
            {
                std::cout << "Executing " << GetName() << "::" << GetID () << ": " << "Timedout waiting for messages, All producers are busy.." << std::endl;
            }
            else
            {
                std::cout << "Executing " << GetName() << "::" << GetID () << ": " << "Counter: " << nMessage << ", Status:" << GetStatus() << ", SubStatus:" << GetSubStatus() << std::endl;
            }

            std::cout << std::flush;

        } while (Yield());
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
        nCounter = 10;
        size_t nNofieds = 0;

        do
        {
            if (LookForWaitings (nCounter, 1, 1000) == false)
            {
                std::cout << "Executing " << GetName() << "::" << GetID () << ": " << "All Consumers seams to be Busy, no Waiting threads at this moment" << std::endl;
            }
            else
            {
                if ((nNofieds = Notify (nCounter, nCounter, (uint32_t) 1, NotifyType::all)) == 0)
                {
                    std::cout << "Executing " << GetName() << "::" << GetID () << ": "<< "No thread has been notified." << std::endl;
                }
                else
                {
                    std::cout << "Executing " << GetName() << "::" << GetID () << ": "<< "Message has been successfully consumed by "  << nNofieds << "." << std::endl;
                }


                nCounter++;
            }
        }  while (Yield());

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
        std::cout << (atomicx::GetCurrent() == &th ? "*  " : "   ") << th.GetID() << "\t" << th.GetName() << "\t, Nc: " << th.GetNice() << "\t, Stk: " << (th.IsStackSelfManaged() ? 'A' : ' ') << th.GetStackSize() << "/i:" << th.GetStackIncreasePace() << ", UsedStk: " << th.GetUsedStackSize() << "\t, St: " << th.GetStatus() << "/" << th.GetSubStatus() << " TTime: " << th.GetTargetTime () << ", t:" << th.GetLastUserExecTime() << "ms" << std::endl;
    }

    std::cout << "-------------------------------------------------------" << std::endl;
}

Thread t1(500, "Producer 1");
ThreadConsummer e1(100, "Consumer 1");

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


    ThreadConsummer e2(500, "Consumer 2");
    ThreadConsummer e3(300, "Consumer 3");

    std::cout << "end context" << std::endl;

    std::cout << "LISTING....." << std::endl;

    ListAllThreads ();

    std::cout << "END LISTING....." << std::endl;

    atomicx::Start();

    std::cout << "[FULL LOCK]-------------------------" << std::endl;

    ListAllThreads();
}
