/**
 * @file atomicx.hpp
 * @author Gustavo Campos
 * @brief  Atomicx header
 * @version 2.0
 * @date 2023-05-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#ifndef ATOMICX_H_
#define ATOMICX_H_

/**
 * @note: standard <c.....> that replaces original C header are
 *        not used here for general compatibility with constraint
 *        and old microprocessors. ex: avr c++11 compatible.
*/
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* Official version */
#define ATOMICX_VERSION "2.0.0"
#define ATOMIC_VERSION_LABEL "AtomicX v" ATOMICX_VERSION " built at " __TIMESTAMP__

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    // typedef uint32_t Tick;

    enum class Status : uint8_t
    {
        STARTING,
        RUNNING,
        PAUSED,
        NOW,
        WAIT
    };

    /**
    * Tick time object implementation
    * @note:    Tick_t will auto select if 
    *           size_t >= 32bits = Tick_t 64bits
    *           size_t < 32bits  = Tick_t 32bits
    */
#if SIZE_MAX >= UINT32_MAX 
    using Tick_t = int64_t;
#else
     using Tick_t = int32_t;
#endif

    class Tick
    {
    public:
        Tick();
        Tick(Tick_t val);

        Tick& operator=(Tick_t& val);

        operator const Tick_t&() const;

        Tick_t value() const;

        Tick& update();
        /*
         * BOTH getTick and sleepTick must be defined EXTERNALLY
         */
        static Tick getTick(void);
        static void sleepTick(Tick nSleep);

        Tick_t diffT(Tick tick) const;
        Tick_t diffT() const;

        Tick diff(Tick tick) const;
        Tick diff() const;

    private:
        Tick_t mTick;
    };

	/**
     * @brief General purpose timeout  facility
     */

	class Timeout
	{
	public:
		/**
         * @brief Default construct a new Timeout object
         *
         * @note    To decrease the amount of memory, Timeout does not save
         *          the start time.
         *          Special use case: if nTimeoutValue == 0, IsTimedout is always false.
         */
		Timeout();

		/**
         * @brief Construct a new Timeout object
         *
         * @param nTimeoutValue  Timeout value to be calculated
         *
         * @note    To decrease the amount of memory, Timeout does not save
         *          the start time.
         *          Special use case: if nTimeoutValue == 0, IsTimedout is always false.
         */
		Timeout(Tick nTimeoutValue);

        bool operator > (Timeout& tm);
        bool operator < (Timeout& tm);
        bool operator != (Timeout& tm);
        bool operator == (Timeout& tm);

		/**
         * 
         * @brief Set a timeout from now
         *
         * @param nTimeoutValue timeout in Tick
         */
		void set(Tick nTimeoutValue);

        bool operator = (Tick tm);

        void update();

		/**
         * @brief Check wether it has timeout
         *
         * @return true if it timeout otherwise 0
         */
		operator bool() const;

        /**
         * @brief Check wether it Can timeout
         *
         * @return true if tiemout value > 0 otherwise false
         */
        bool hasTimeout() const;
        
		/**
         * @brief Get the remaining time till timeout
         *
         * @return Tick Remaining time till timeout, otherwise 0;
         */
		Tick until() const;

		/**
         * @brief Get the Time Since the specific point in time
         *
         * @param startTime     The specific point in time
         *
         * @return Tick How long since the point in time
         *
         * @note    To decrease the amount of memory, Timeout does not save
         *          the start time.
         */
		Tick since(Tick startTime);

        Tick since();

	private:
		Tick m_timeoutValue = 0;
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
        /**
         * @brief User controllable Params
         */
        struct Params
        {
            Status status{Status::STARTING};
            Tick nice{0};
            size_t stackSize{0};
            size_t usedStackSize{0};
            Timeout timeout{0};
        };

        Thread* next();

        Iterator<Thread> begin();

        Iterator<Thread> end();

        static bool start();

        const Params& getParams() const;

    protected:
        virtual void run() = 0;

        template <size_t N>
        Thread(Tick nNice, volatile size_t (&stack)[N]) : mVirtualStack(stack[0])
        {
            mDt.stackSize = N * sizeof(size_t);
            mDt.nice = nNice;

            addThread(*this);
        }

        static bool yield();

    private:
        static void scheduler();

        static void addThread(Thread& thread);

        static Thread* mBegin;
        static Thread* mEnd;
        static Thread* mCurrent;
        static jmp_buf mJmpStart;

        // The single point from Start
        static volatile uint8_t* mStackBegin;

        // Initialized by the constructor
        volatile size_t& mVirtualStack;

        // Thread Data
        Params mDt{};

        // Self-Initialized
        Thread* mNext{nullptr};
        jmp_buf mJmpThread{};

        volatile uint8_t* mStackEnd{nullptr};
    };

}  // namespace atomicx

#endif
