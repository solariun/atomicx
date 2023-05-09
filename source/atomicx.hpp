//
//  atomic.hpp
//  atomic
//
//  Created by GUSTAVO CAMPOS on 31/01/2023.

#pragma once

#ifndef ATOMICX_H_
#define ATOMICX_H_

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Official version */
#define ATOMICX_VERSION "2.0.0"
#define ATOMIC_VERSION_LABEL "AtomicX v" ATOMICX_VERSION " built at " __TIMESTAMP__

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

// ------------------------------------------------------
// LOG FACILITIES
//
// TO USE, define -D_DEBUG=<LEVEL> where level is any of
// . those listed in DBGLevel, ex
// .    -D_DEBUG=INFO
// .     * on the example, from DEBUG to INFO will be
// .       displayed
// ------------------------------------------------------

#define STRINGIFYY(a) #a
#define STRINGIFY(a) STRINGIFYY(a)

#define NOTRACE(i, x) (void)#x

#ifdef _DEBUG
#include <iostream>
#define TRACE(i, x)                                                                                          \
    if (DBGLevel::i <= DBGLevel::_DEBUG)                                                                     \
    std::cout << Thread::GetCurrent() << "(" << Thread::GetCurrent()->GetName() << ")[" << #i << "] "        \
              << "(" << __FUNCTION__ << ", " << __FILE_NAME__ << ":" << __LINE__ << "):  " << x << std::endl \
              << std::flush
#else
#define TRACE(i, x) NOTRACE(i, x)
#endif

enum class DBGLevel
{
    CRITICAL,
    ERROR,
    WARNING,
    KERNEL,
    WAIT,
    LOCK,
    INFO,
    TRACE,
    DEBUG
};

namespace atomicx
{
    typedef uint32_t Tick;

    enum class Status : uint8_t
    {
        STARTING,
        RUNNING,
        PAUSED
    };

    /*
     * ITERATOR IMPLEMENTATION
    */
    template <class T>
    class Iterator
    {
    public:
        Iterator() = delete;
        Iterator(T* obj) : mObj(obj)
        {}

        T& operator*() const
        {
            return *mObj;
        }
        T* operator->()
        {
            return mObj;
        }

        // Prefix increment
        Iterator& operator++()
        {
            mObj = mObj->next();
            return *this;
        }

        friend bool operator==(const Iterator& a, const Iterator& b)
        {
            return a.mObj == b.mObj;
        };
        friend bool operator!=(const Iterator& a, const Iterator& b)
        {
            return a.mObj != b.mObj;
        };

    private:
        T* mObj;
    };

    /*
     * THREAD IMPLEMENTATION
    */
    class Thread
    {
    public:
        Thread* next();

        Iterator<Thread> begin();

        Iterator<Thread> end();

        static bool start();

    protected:
        virtual void run() = 0;

        Tick getTick (void);
        void sleepTick(Tick nSleep);

        template <size_t N>
        Thread(Tick nNice, volatile size_t (&stack)[N]) : mStackSize{N}, mNice(nNice), mVirtualStack(stack[0])
        {
            addThread(*this);
        }

        static bool yield();

    private:
        void addThread(Thread& thread);

        static Thread* mBegin;
        static Thread* mEnd;
        static Thread* mCurrent;
        static jmp_buf mJmpStart;

        static volatile uint8_t* mStackBegin;

        // Initialized by constructor
        size_t mStackSize;
        Tick mNice;
        volatile size_t& mVirtualStack;

        // Self-Initialized
        Status mStatus{Status::STARTING};
        Thread* mNext{nullptr};
        jmp_buf mJmpThread{};

        volatile uint8_t* mStackEnd{nullptr};
        size_t mUsedStackSize{0};
    };

}  // namespace atomicx

#endif
