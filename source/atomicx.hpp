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
#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Official version */
#define ATOMICX_VERSION "2.0.0"
#define ATOMIC_VERSION_LABEL "AtomicX v" ATOMICX_VERSION " built at " __TIMESTAMP__

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
    std::cout << "[" << #i << "] "                                                                           \
              << "(" << __FUNCTION__ << ", " << __FILE_NAME__ << ":" << __LINE__ << "):  " << x << std::endl \
              << std::flush
#else
#define TRACE(i, x) NOTRACE(i, x)
#endif

enum class DBGLevel
{
    KERNEL_CRITICAL,
    CRITICAL,
    KERNEL_ERROR,
    ERROR,
    WARNING,
    INFO,
    TRACE,
    DEBUG,
    LOCK,
    WAIT,
    KERNEL,
    SCHEDULER,
};

namespace atomicx
{
    // typedef uint32_t Tick;

    enum class Status
    {
        STARTING,
        RUNNING,
        PAUSED,
        TIMEOUT,
        NOW,
        WAIT,
        SYNC_WAIT
    };

    const char* statusName(Status st);

    enum class Notify
    {
        ONE,
        ALL
    };

    enum class Wait
    {
        MESSAGE,
        NOTIFY_ONLY
    };

    /**
     * Tick time object implementation
     * @note:    Tick_t will auto select if
     *           size_t >= 32bits = Tick_t 64bits
     *           size_t < 32bits  = Tick_t 32bits
     */
#if SIZE_MAX >= UINT32_MAX
    using Tick_t = int64_t;
    static constexpr size_t TICK_DEFAULT = static_cast<size_t>(INT64_MAX);
#else
    using Tick_t = int32_t;
    static constexpr size_t TICK_DEFAULT = INT32_MAX;
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
         * BOTH Tick::now and Tick::sleep must be defined EXTERNALLY
         */
        static Tick now(void);
        static void sleep(Tick nSleep);

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
        explicit Timeout(Tick nTimeoutValue);
        Timeout(Tick_t nTimeoutValue);

        /**
         * @brief greater operator
         *
         * @param tm        Timeout object to compare
         *
         * @return true     if greater otherwise false
         */
        bool operator>(Timeout& tm);

        /**
         * @brief lesser operator
         *
         * @param tm        Timeout object to compare
         *
         * @return true     if lesses otherwise false
         */
        bool operator<(Timeout& tm);

        /**
         * @brief different operator
         *
         * @param tm        Timeout object to compare
         *
         * @return true     if different otherwise false
         */
        bool operator!=(Timeout& tm);

        /**
         * @brief equal operator
         *
         * @param tm        Timeout object to compare
         *
         * @return true     if equal otherwise false
         */
        bool operator==(Timeout& tm);

        /**
         *
         * @brief Set a timeout from now
         *
         * @param nTimeoutValue timeout in Tick
         */
        void set(Tick nTimeoutValue);

        /**
         * @brief assignment operator
         *
         * @param tm        Timeout object to assign
         *
         * @return true     always returns true
         */

        bool operator=(Tick tm);

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
         *
         * @note: the ref Now point is taken from getTick
         */
        Tick until() const;

        /**
         * @brief Get the remaining time till timeout from a ref now point
         *
         * @param  the ref Now point
         *
         * @return Tick Remaining time till timeout, otherwise 0;
         */
        Tick until(Tick now) const;

        const Tick& value();

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
        struct Payload
        {
            Payload();
            Payload(size_t type, size_t message);
            Payload(size_t type, size_t message, size_t channel);
            size_t channel{0};
            size_t type;
            size_t message;
        };

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
            void* endpoint{nullptr};
            Payload payload{};
        };

        Thread* next();

        Iterator<Thread> begin();

        Iterator<Thread> end();

        static bool start();

        const Params& getParams() const;

    protected:
        virtual void run() = 0;
        virtual const char* getName() const = 0;
        template <size_t N>
        Thread(Tick nNice, volatile size_t (&stack)[N]) : mVirtualStack(stack[0])
        {
            mDt.stackSize = N * sizeof(size_t);
            mDt.nice = nNice;

            addThread(*this);
        }

        static inline void contextChange();
        static bool yield(Tick tm, Status st = Status::RUNNING);
        static bool yield();
        static bool yieldNow();

        size_t safeNotify(void* endpoint, Payload& payload, Status st, bool notifyOnly, bool notifyAll);

        template <typename T>
        inline size_t notify(T& endpoint, Payload& payload, Status st, bool notifyOnly, bool notifyAll)
        {
            size_t nCount = 0;

            TRACE(WAIT, "endp:" << &endpoint << ", payl:" << payload.type << "," << payload.message << "," << payload.channel);
            nCount = safeNotify(static_cast<void*>(&endpoint), payload, st, notifyOnly, notifyAll);

            yieldNow();

            return nCount;
        }

        template <typename T>
        inline bool wait(T& endpoint, Payload& payload, Timeout& tm, Status st)
        {
           TRACE(WAIT, "WAITING:" << &endpoint << ", t:" << payload.type << ", m:" << payload.message << ", ch:" << payload.channel << ", TM:" << tm.value());

            // Prepare payload
            mDt.endpoint = static_cast<void*>(&endpoint);
            mDt.payload = payload;
            // Start waiting
            yield(tm.until(), st);

            if (mDt.status == Status::TIMEOUT) return false;

            return true;
        }

        template <typename T>
        size_t notify(T& endpoint, Payload payload, Timeout tm, bool notifyAll = true)
        {
            size_t nCount{0};
            while (!tm && !(nCount = notify(endpoint, payload, Status::WAIT, false, notifyAll))) {
                wait(endpoint, payload, tm, Status::SYNC_WAIT);
            }

            TRACE(WAIT, "NOTIFY: endp:" << &endpoint << ", count:" << nCount << ", st:" << statusName(mDt.status));
            if (nCount == 0 && mDt.status == Status::TIMEOUT) return 0;

            return nCount;
        }

        template <typename T, typename FUNC>
        bool wait(T& endpoint, Payload& payload, Timeout tm, FUNC condition = []()->bool{return true;})
        {
            while (!tm && !condition()) {
                notify(endpoint, payload, Status::SYNC_WAIT, true, false);
                if (!wait(endpoint, payload, tm, Status::WAIT))
                    return false;
            }

            return true;
        }

        template <typename T>
        bool wait(T& endpoint, Payload& payload, Timeout tm)
        {
            notify(endpoint, payload, Status::SYNC_WAIT, true, false);
            if (!wait(endpoint, payload, tm, Status::WAIT))
                return false;

            return true;
        }

    private:
        static Thread* scheduler();

        static void addThread(Thread& thread);

        static Thread* mBegin;
        static Thread* mEnd;
        static Thread* mCurrent;
        static jmp_buf mJmpStart;
        static size_t mThreadCount;

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
