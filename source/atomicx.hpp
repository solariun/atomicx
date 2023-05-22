/**
 * @file atx.hpp
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
#define TRACE(i, x)                                                                                              \
    { if (DBGLevel::i <= DBGLevel::_DEBUG || DBGLevel::i == DBGLevel::YYTRACE)                                                                         \
    std::cout << atx::Thread::getCurrent().getName() << "." << &(atx::Thread::getCurrent()) << "[" << #i << "] " \
              << "(" << __FUNCTION__ << ", " << __FILE_NAME__ << ":" << __LINE__ << "):  " << x << std::endl     \
              << std::flush; }
#else
#define TRACE(i, x) NOTRACE(i, x)
#endif

enum class DBGLevel
{
    YYTRACE,
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

namespace atx
{
/**
 * @brief Simple flattened wait (notification only)
 *
 * @param ENDP  Endpoint
 * @param PL    Payload
 * @param TM    Timeout
 * @param ST    Status
 *
 * @note To be used by the system implementation only
 */
#define SWAIT_(ENDP, PL, TM, ST)            \
    {                                       \
        getCurrent().setWait(ENDP, PL); \
        getCurrent().yield(TM.until(), ST); \
    }

/**
 * @brief Simple flattened notify (notification only)
 *
 * @param ENDP  Endpoint
 * @param PL    Payload
 * @param ST    Status
 * @param ALL   notify all
 *
 * @note To be used by the system implementation only
 */
#define SNOTIFY_(ENDP, PL, ST, ALL)                       \
    {                                                     \
        getCurrent().safeNotify(ENDP, PL, ST, true, ALL); \
        getCurrent().yield(0, Status::NOW);               \
    }

    static constexpr size_t SYSTEM_NOTIFY_CHANNEL = SIZE_MAX;

    static constexpr size_t SLOCK_UNIQUE_NTYPE = 1;
    static constexpr size_t SLOCK_SHARED_NTYPE = 2;

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
    static constexpr Tick_t TICK_MAX = static_cast<size_t>(INT64_MAX);
#else
    using Tick_t = int32_t;
    static constexpr Tick_t TICK_MAX = INT32_MAX;
#endif

    /*
     * BOTH now and sleep must be defined EXTERNALLY
     */
    class Tick;

    Tick now(void);
    void sleep(Tick nSleep);

    class Tick
    {
    public:
        Tick();
        Tick(Tick_t val);

        Tick& operator=(Tick_t& val);

        operator const Tick_t&() const;

        Tick_t value() const;

        Tick& update();
        Tick& update(Tick_t add);

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
        bool operator>(Timeout& tm) const;

        /**
         * @brief lesser operator
         *
         * @param tm        Timeout object to compare
         *
         * @return true     if lesses otherwise false
         */
        bool operator<(Timeout& tm) const;

        /**
         * @brief different operator
         *
         * @param tm        Timeout object to compare
         *
         * @return true     if different otherwise false
         */
        bool operator!=(Timeout& tm) const;

        /**
         * @brief equal operator
         *
         * @param tm        Timeout object to compare
         *
         * @return true     if equal otherwise false
         */
        bool operator==(Timeout& tm) const;

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

        const Tick& value() const;

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

    /**
     * @brief message payload from wait/notify
     */
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
        const void* endpoint{nullptr};
        Payload payload{};
    };

    /*
     * THREAD IMPLEMENTATION
     */
    class Thread
    {
    public:
        class Mutex;

        Thread* next();

        Iterator<Thread> begin();

        Iterator<Thread> end();

        static bool start();

        const Params& getParams() const;

        static Thread& getCurrent();

        virtual const char* getName() const = 0;

    protected:
        virtual void run() = 0;
        template <size_t N>
        Thread(Tick nNice, volatile size_t (&stack)[N]) : mVirtualStack(stack[0])
        {
            mDt.stackSize = N * sizeof(size_t);
            mDt.nice = nNice;

            addThread(*this);
        }

        bool yield(Tick tm = TICK_MAX, Status st = Status::RUNNING);

        template <typename T>
        static size_t hasWaitings(const T& endpoint, const Payload& payload, Status st)
        {
            bool ret = false;

            for (auto& th : getCurrent()) {
                if (st == th.mDt.status && &endpoint == th.mDt.endpoint && th.mDt.payload.channel == payload.channel
                    && th.mDt.payload.type == payload.type) {
                    ret = true;
                }
            }

            TRACE(WAIT,
                  "HASWAIT: (" << (ret ? "TRUE" : "FALSE") << ") Expecting: endp:" << (void*)&endpoint << ", st:" << statusName(st)
                               << ", t:" << payload.type << ", c:" << payload.channel);

            return ret;
        }

        template <typename T>
        size_t notify(T& endpoint, Payload payload, Timeout tm, bool notifyAll = true)
        {
            if (payload.channel == SYSTEM_NOTIFY_CHANNEL)
                return 0;

            size_t nCount{0};
            if (!hasWaitings(endpoint, payload, Status::WAIT)) {
                setWait(endpoint, payload);
                yield(tm.until(), Status::SYNC_WAIT);
            }

            nCount = safeNotify(endpoint, payload, Status::WAIT, false, notifyAll);
            yield(0, Status::NOW);

            return nCount;
        }

        static bool defaultFunc()
        {
            return true;
        }
        template <typename T, typename FUNC>
        bool wait(T& endpoint, Payload& payload, Timeout tm, FUNC condition = defaultFunc)
        {
            if (payload.channel == SYSTEM_NOTIFY_CHANNEL)
                return false;

            while (!condition() && mDt.status != Status::TIMEOUT) {
                if (hasWaitings(endpoint, payload, Status::SYNC_WAIT))
                    safeNotify(endpoint, payload, Status::SYNC_WAIT, true, false);

                setWait(endpoint, payload);
                yield(tm.until(), Status::WAIT);
            }

            if (!tm && mDt.status != Status::TIMEOUT) {
                payload = mDt.payload;
                mDt.payload = {};
                return true;
            }

            return true;
        }

        template <typename T>
        bool wait(T& endpoint, Payload& payload, Timeout tm)
        {
            if (payload.channel == SYSTEM_NOTIFY_CHANNEL)
                return false;

            if (hasWaitings(endpoint, payload, Status::SYNC_WAIT))
                safeNotify(endpoint, payload, Status::SYNC_WAIT, true, false);

            setWait(endpoint, payload);
            yield(tm.until(), Status::WAIT);

            if (mDt.status != Status::TIMEOUT) {
                payload = mDt.payload;
                mDt.payload = {};
                return true;
            }
            return false;
        }

    private:
        template <typename T>
        static size_t safeNotify(const T& endpoint, const Payload& payload, Status st, bool notifyOnly, bool notifyAll)
        {
            size_t nCount{0};

            TRACE(WAIT,
                  "NOTIFYING:" << &endpoint << ", st:" << statusName(st) << ", t:" << payload.type << ", m:" << payload.message
                               << ", ch:" << payload.channel << ", NotOnly:" << notifyOnly << ", NotifyAll:" << notifyAll);

            for (auto& th : getCurrent()) {
                if (st == th.mDt.status)
                    if (&endpoint == th.mDt.endpoint && th.mDt.payload.channel == payload.channel && th.mDt.payload.type == payload.type) {
                        if (!notifyOnly)
                            th.mDt.payload.message = payload.message;
                        th.mDt.status = Status::NOW;
                        th.mDt.timeout.set(0);
                        nCount++;
                        TRACE(WAIT,
                              "FOUND:" << nCount << "," << &th << "." << th.getName() << ", endp:" << th.mDt.endpoint
                                       << ", st:" << statusName(th.mDt.status) << ", t:" << th.mDt.payload.type << ", m:" << th.mDt.payload.message
                                       << ", ch:" << th.mDt.payload.channel);
                        if (notifyAll)
                            break;
                    }
            }

            return nCount;
        }

        template <typename T>
        inline void setWait(const T& endpoint, Payload& payload)
        {
            TRACE(WAIT,
                  "WAITING:" << &endpoint << ", t:" << payload.type << ", m:" << payload.message
                             << ", ch:" << payload.channel);
                             
            // Prepare payload
            mDt.endpoint = static_cast<const void*>(&endpoint);
            mDt.payload = payload;
        }

        static Thread* scheduler();

        static void addThread(Thread& thread);

        static Thread* mBegin;
        static Thread* mEnd;
        static Thread* mCurrent;
        static jmp_buf mJmpStart;
        static size_t mThreadCount;

        // The single point from Start
        static volatile uint8_t* mStackBegin;
        volatile uint8_t* mStackEnd{nullptr};

        // Initialized by the constructor
        volatile size_t& mVirtualStack;

        // Thread Data
        Params mDt{};

        // Self-Initialized
        Thread* mNext{nullptr};
        jmp_buf mJmpThread{};
    };

    /**
     * @brief Mutex implementation
     */
    class Thread::Mutex
    {
    public:
        Mutex() = default;

        bool isSharedLocked() const;
        size_t sharedLockCount() const;
        bool isUniqueLocked() const;

        bool uniqueLock(const Timeout tm = 0) noexcept;
        bool tryUniqueLock();
        bool sharedLock(const Timeout tm = 0) noexcept;
        bool trySharedLock();

        bool uniqueUnlock() noexcept;
        bool sharedUnlock() noexcept;

    public:
        bool mUniqueLock{false};
        size_t mSharedLock{0};
    };

}  // namespace atx

#endif
