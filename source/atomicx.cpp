
#include "atomicx.hpp"

namespace atx
{
    /* STATIC INITIALIZATIONS */
    Thread* Thread::mBegin{nullptr};
    Thread* Thread::mEnd{nullptr};
    Thread* Thread::mCurrent{nullptr};
    size_t Thread::mThreadCount{0};
    jmp_buf Thread::mJmpStart{};
    volatile uint8_t* Thread::mStackBegin{nullptr};

    const char* statusName(Status st)
    {
#define CASE_STATUS(x) \
    case Status::x:    \
        return #x
        switch (st) {
            CASE_STATUS(STARTING);
            CASE_STATUS(RUNNING);
            CASE_STATUS(PAUSED);
            CASE_STATUS(TIMEOUT);
            CASE_STATUS(NOW);
            CASE_STATUS(WAIT);
            CASE_STATUS(SYNC_WAIT);
            default:
                return "UNKNOW";
        }
    }

    /* PAYLOAD INITIALIZATION */
    Payload::Payload() : type(0), message(0)
    {
        channel = 0;
    }
    Payload::Payload(size_t type, size_t message) : type(type), message(message)
    {}
    Payload::Payload(size_t channel, size_t type, size_t message) : type(type), message(message)
    {
        this->channel = channel;
    }

    /* Tick object and integer emulator */

    Tick::Tick() : mTick(now()){};
    Tick::Tick(Tick_t val) : mTick(val){};

    Tick& Tick::operator=(Tick_t& val)
    {
        mTick = val;
        return *this;
    }

    Tick::operator const Tick_t&() const
    {
        return mTick;
    };

    Tick_t Tick::value() const
    {
        return mTick;
    }

    Tick& Tick::update()
    {
        mTick = now();
        return *this;
    }

    Tick& Tick::update(Tick_t add)
    {
        mTick = now() + add;
        return *this;
    }

    Tick_t Tick::diffT(Tick tick) const
    {
        return (tick - mTick);
    }

    Tick_t Tick::diffT() const
    {
        return (now() - mTick);
    }

    Tick Tick::diff(Tick tick) const
    {
        return Tick(tick - mTick);
    }

    Tick Tick::diff() const
    {
        return Tick(now() - mTick);
    }

    /* TIMEOUT */

    Timeout::Timeout() : m_timeoutValue(0)
    {
        set(0);
    }

    Timeout::Timeout(Tick nTimeoutValue) : m_timeoutValue(nTimeoutValue ? nTimeoutValue + now() : 0)
    {}

    Timeout::Timeout(Tick_t nTimeoutValue) : m_timeoutValue(nTimeoutValue ? nTimeoutValue + now() : 0)
    {}

    bool Timeout::operator>(Timeout& tm) const
    {
        return m_timeoutValue != 0 ? m_timeoutValue > tm.m_timeoutValue : true;
    }

    bool Timeout::operator<(Timeout& tm) const
    {
        return m_timeoutValue != 0 ? m_timeoutValue < tm.m_timeoutValue : false;
    }

    bool Timeout::operator!=(Timeout& tm) const
    {
        return m_timeoutValue != tm.m_timeoutValue;
    }

    bool Timeout::operator==(Timeout& tm) const
    {
        return m_timeoutValue == tm.m_timeoutValue;
    }

    void Timeout::set(Tick nTimeoutValue)
    {
        m_timeoutValue = nTimeoutValue != 0 ? nTimeoutValue + now() : 0;
    }

    bool Timeout::operator=(Tick tm)
    {
        return (m_timeoutValue = tm);
    }

    void Timeout::update()
    {
        m_timeoutValue = now();
    }

    bool Timeout::hasTimeout() const
    {
        return (m_timeoutValue == 0 ? false : true);
    }

    Timeout::operator bool() const
    {
        return (m_timeoutValue == 0 || m_timeoutValue > now()) ? false : true;
    }

    Tick Timeout::until() const
    {
        return (m_timeoutValue ? m_timeoutValue - now() : TICK_MAX);
    }

    Tick Timeout::until(Tick now) const
    {
        return (m_timeoutValue ? m_timeoutValue - now : TICK_MAX);
    }

    const Tick& Timeout::value() const
    {
        return m_timeoutValue;
    }

    /* SHARED LOCK */
    bool Thread::Mutex::isSharedLocked() const
    {
        return mSharedLock > 0;
    }

    size_t Thread::Mutex::sharedLockCount() const
    {
        return mSharedLock;
    }

    bool Thread::Mutex::isUniqueLocked() const
    {
        return mUniqueLock;
    }

    bool Thread::Mutex::uniqueLock(const Timeout tm) noexcept
    {
        // Wait the unique lock to be released
        while (mUniqueLock) {
            Payload pl{SLOCK_UNIQUE_NTYPE, 0, SYSTEM_NOTIFY_CHANNEL};
            SWAIT_(mUniqueLock, pl, tm, Status::WAIT);
            if (Thread::getCurrent().getParams().status == Status::TIMEOUT)
                return false;
        }

        // Own the context
        mUniqueLock = true;

        // Wait for all shared lock to be released
        while (mSharedLock) {
            Payload pl{SLOCK_SHARED_NTYPE, 0, SYSTEM_NOTIFY_CHANNEL};
            SWAIT_(mSharedLock, pl, tm, Status::WAIT);
            if (Thread::getCurrent().getParams().status == Status::TIMEOUT)
                return false;
        }

        // Lock acquired
        return true;
    }

    bool Thread::Mutex::tryUniqueLock()
    {
        if (mUniqueLock)
            return false;

        mUniqueLock = true;
        return true;
    }

    bool Thread::Mutex::uniqueUnlock() noexcept
    {
        if (mUniqueLock) {
            mUniqueLock = false;
            Payload pl{SLOCK_UNIQUE_NTYPE, 0, SYSTEM_NOTIFY_CHANNEL};
            SNOTIFY_(mUniqueLock, pl, Status::WAIT, false);
            pl = {SLOCK_SHARED_NTYPE, 0, SYSTEM_NOTIFY_CHANNEL};
            SNOTIFY_(mSharedLock, pl, Status::WAIT, true);
        }

        return true;
    }

    // bool Thread::Mutex::sharedLock(const Timeout tm) noexcept;
    // bool Thread::Mutex::trySharedLock();
    // bool Thread::Mutex::sharedUnlock() noexcept;

    /* THREAD */
    Thread* Thread::next()
    {
        return mNext;
    }

    Iterator<Thread> Thread::begin()
    {
        return Iterator<Thread>(mBegin);
    }

    Iterator<Thread> Thread::end()
    {
        return Iterator<Thread>(nullptr);
    }

    const Params& Thread::getParams() const
    {
        return mDt;
    }

    void Thread::addThread(Thread& thread)
    {
        if (mBegin == nullptr) {
            mBegin = &thread;
            mEnd = mBegin;
        } else {
            mEnd->mNext = &thread;
            mEnd = mEnd->mNext;
        }

        thread.mNext = nullptr;
        mThreadCount++;
    }

    Thread& Thread::getCurrent()
    {
        return *mCurrent;
    }

    /* CONTEXT HANDLERS */
    Thread* Thread::scheduler()
    {
        if (mCurrent == nullptr)
            return nullptr;

        Tick now;
        Thread* candidate = nullptr;
        bool stop = false;

        Thread* th = mCurrent;
        size_t thCount = mThreadCount;

        while (thCount--) {
            th = th->mNext ? th->mNext : mBegin;

            TRACE(SCHEDULER,
                  "SELECT: [" << th << "." << th->getName() << "]: Status:" << statusName(th->mDt.status) << ", until:" << th->mDt.timeout.until()
                              << ", canTimout:" << th->mDt.timeout.hasTimeout());
            if (th->mDt.status == Status::STARTING) {
                candidate = th;
                candidate->mDt.timeout = now;
                stop = true;
                break;
            } else if (!candidate || (th->mDt.timeout.hasTimeout() && th->mDt.timeout < candidate->mDt.timeout)) {
                candidate = th;
            }
        }

        if (candidate != nullptr) {
            TRACE(SCHEDULER,
                  "Candidate: " << (candidate) << "." << candidate->getName() << ", next:" << candidate->mDt.timeout.until()
                                << ", value:" << candidate->mDt.timeout.value() << ", timeout:" << candidate->mDt.timeout.hasTimeout());

            if (candidate->mDt.timeout.hasTimeout() && candidate->mDt.timeout.until(now) > 0) {
                sleep(candidate->mDt.timeout.until(now));
                TRACE(SCHEDULER, "SLEEP: " << (candidate) << ", for:" << candidate->mDt.timeout.until(now));
            }

            if ((candidate->mDt.status == Status::WAIT || candidate->mDt.status == Status::SYNC_WAIT) && candidate->mDt.timeout) {
                candidate->mDt.status = Status::TIMEOUT;
            }
        }

        return candidate;
    }

    bool Thread::start()
    {
        bool ret = false;

        if (mBegin != nullptr) {
            mCurrent = mBegin;

            while (mCurrent != nullptr) {
                // Set the context for the start
                if (setjmp(mJmpStart) == 0) {
                    if (mCurrent->mDt.status == Status::STARTING) {
                        volatile uint8_t stackBegin = 0xAA;
                        mCurrent->mStackBegin = &stackBegin;

                        // Mark the new thread as running
                        mCurrent->mDt.status = Status::RUNNING;
                        // Execute the object
                        mCurrent->run();
                    } else {
                        // Jump back to the thread
                        if (mCurrent->mDt.status != Status::TIMEOUT)
                            mCurrent->mDt.status = Status::RUNNING;
                        longjmp(mCurrent->mJmpThread, 1);
                    }
                }

                // get the next thread, schedule can proactively sleep if no other
                // task is due to the time.
                mCurrent = scheduler();  // mCurrent->mNext ? mCurrent->mNext : mBegin;
            }
        }

        return ret;
    }

    bool Thread::yield(Tick tm, Status st)
    {
        if (mCurrent) {
            if (st == Status::RUNNING) {
                // Set running to sleep
                st = Status::PAUSED;
                // Update value for nice or custom (if tm is defined)
                tm = now() + static_cast<Tick_t>(tm == TICK_MAX ? mCurrent->mDt.nice : tm);
            } else if (st == Status::NOW) {
                tm = now();
            } else {
                tm = tm.value() == 0 ? Tick(now() + tm) : tm;
            }

            // Set timeout for the context switching thread
            mCurrent->mDt.timeout = tm;
            // Set status
            mCurrent->mDt.status = st;

            // Discover the end of the used stack;
            volatile uint8_t stackEnd = 0xBB;
            mCurrent->mStackEnd = &stackEnd;

            // Calculate the amount of stack used
            mCurrent->mDt.usedStackSize = static_cast<size_t>(mCurrent->mStackBegin - mCurrent->mStackEnd);

            TRACE(KERNEL,
                  "CTX: size:" << mCurrent->mDt.usedStackSize << ", Bgn:" << (size_t)mCurrent->mStackBegin
                               << ", end:" << (size_t)mCurrent->mStackEnd);

            // Backup the stack in the virutal stack memory of the thread
            memcpy((void*)&mCurrent->mVirtualStack, (const void*)mCurrent->mStackEnd, mCurrent->mDt.usedStackSize);

            // Execute the context switching
            if (setjmp(mCurrent->mJmpThread) == 0) {
                // Jump back for the start loop
                longjmp(mCurrent->mJmpStart, 1);
            }

            // Restore stack from the thread's virtual stack memory
            memcpy((void*)mCurrent->mStackEnd, (const void*)&mCurrent->mVirtualStack, mCurrent->mDt.usedStackSize);

            return true;
        }

        return false;
    }

}  // namespace atx
