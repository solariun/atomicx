
#include "atomicx.hpp"

namespace atomicx
{
    /* STATIC INITIALIZATIONS */
    Thread* Thread::mBegin{nullptr};
    Thread* Thread::mEnd{nullptr};
    Thread* Thread::mCurrent{nullptr};
    size_t Thread::mThreadCount{0};

    /* PAYLOAD INITIALIZATION */
    Thread::Payload::Payload(): type(0), message(0)
    {
        channel = 0;
    }
    Thread::Payload::Payload(size_t type, size_t message) : type(type), message(message) {}
    Thread::Payload::Payload(size_t channel, size_t type, size_t message) : type(type), message(message)
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

    Timeout::Timeout(Tick nTimeoutValue) : m_timeoutValue(nTimeoutValue ? nTimeoutValue + Tick::now() : 0)
    {}

    bool Timeout::operator>(Timeout& tm)
    {
        return m_timeoutValue > tm.m_timeoutValue;
    }

    bool Timeout::operator<(Timeout& tm)
    {
        TRACE(KERNEL, "operator<: " << m_timeoutValue << " < " << tm.m_timeoutValue);
        return m_timeoutValue < tm.m_timeoutValue;
    }

    bool Timeout::operator!=(Timeout& tm)
    {
        return m_timeoutValue != tm.m_timeoutValue;
    }

    bool Timeout::operator==(Timeout& tm)
    {
        TRACE(KERNEL, "operator==: " << m_timeoutValue << " == " << tm.m_timeoutValue);
        return m_timeoutValue == tm.m_timeoutValue;
    }

    void Timeout::set(Tick nTimeoutValue)
    {
        m_timeoutValue = nTimeoutValue ? nTimeoutValue + Tick::now() : 0;
    }

    bool Timeout::operator=(Tick tm)
    {
        return (m_timeoutValue = tm);
    }

    void Timeout::update()
    {
        m_timeoutValue = Tick::now();
    }

    bool Timeout::hasTimeout() const
    {
        return (m_timeoutValue == 0 ? false : true);
    }

    Timeout::operator bool() const
    {
        return (m_timeoutValue == 0 || m_timeoutValue > Tick::now()) ? false : true;
    }

    Tick Timeout::until() const
    {
        return (m_timeoutValue - Tick::now());
    }

    Tick Timeout::until(Tick now) const
    {
        return (m_timeoutValue - now);
    }

    const Tick& Timeout::value()
    {
        return m_timeoutValue;
    }

    /* THREAD */
    jmp_buf Thread::mJmpStart{};

    volatile uint8_t* Thread::mStackBegin{nullptr};

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

    const Thread::Params& Thread::getParams() const
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

    Thread* Thread::scheduler()
    {
        if (mCurrent == nullptr) return nullptr;

        Tick now;
        Thread* candidate = nullptr;
        bool stop = false;

        Thread* th = mCurrent;
        size_t thCount = mThreadCount;

        while (thCount--) {
            th = th->mNext ? th->mNext : mBegin;

            TRACE(SCHEDULER, "SELECT: [" << th << "]: Status:" << static_cast<size_t>(th->mDt.status) << ", until:" << th->mDt.timeout.until());
            switch (th->mDt.status) {
                case Status::STARTING:
                case Status::NOW:
                    // If NOW it will be executed now
                    candidate = th;
                    candidate->mDt.timeout.update();
                    stop = true;
                    break;
                case Status::PAUSED:
                case Status::RUNNING:
                case Status::WAIT:
                case Status::SYNC_WAIT:
                    // if candidate == null or th will timout sooner than candidate, use th
                    if (!candidate || th->mDt.timeout < candidate->mDt.timeout) candidate = th;
                    break;
            }

            if (stop) break;
        }

        TRACE(SCHEDULER, "Candidate: " << (candidate) << ", next:" << candidate->mDt.timeout.until());

        if (candidate->mDt.timeout.until(now) > 0) {
            TRACE(SCHEDULER, "SLEEP: " << (candidate) << ", for:" << candidate->mDt.timeout.until());
            Tick::sleep(candidate->mDt.timeout.until());
        }

        return candidate;
    }

    bool Thread::start()
    {
        bool ret = false;

        if (mBegin != nullptr) {
            mCurrent = mBegin;

            volatile uint8_t stackBegin = 0xAA;
            mStackBegin = &stackBegin;

            while (mCurrent != nullptr) {
                // Set the context for the start
                if (setjmp(mJmpStart) == 0) {
                    switch (mCurrent->mDt.status) {
                        case Status::STARTING:
                            // Mark the new thread as running
                            mCurrent->mDt.status = Status::RUNNING;
                            // Execute the object
                            mCurrent->run();
                            break;
                        default:
                            break;
                    }
                    // Jump back to the thread
                    longjmp(mCurrent->mJmpThread, 1);
                }

                // get the next thread, schedule can proactively sleep if no other
                // task is due to the time.
                mCurrent = scheduler();  // mCurrent->mNext ? mCurrent->mNext : mBegin;
            }
        }

        return ret;
    }

    inline void Thread::contextChange()
    {
        {
            volatile uint8_t stackEnd = 0xBB;
            // Discover the end of the used stack;
            mCurrent->mStackEnd = &stackEnd;
            // Calculate the amount of stack used
            mCurrent->mDt.usedStackSize = static_cast<size_t>(mCurrent->mStackBegin - mCurrent->mStackEnd);
            // Backup the stack in the virutal stack memory of the thread
            memcpy((void*)&mCurrent->mVirtualStack, (const void*)mCurrent->mStackEnd, mCurrent->mDt.usedStackSize);
        }

        // Execute the context switching
        if (setjmp(mCurrent->mJmpThread) == 0) {
            // Jump back for the start loop
            longjmp(mCurrent->mJmpStart, 1);
        }

        // Restore stack from the thread's virtual stack memory
        memcpy((void*)mCurrent->mStackEnd, (const void*)&mCurrent->mVirtualStack, mCurrent->mDt.usedStackSize);
    }

    bool Thread::yield(Tick tm, Status st)
    {
        if (mCurrent) {
            if (st == Status::RUNNING) {
                // Set running to sleep
                st = Status::PAUSED;
                // Update value for nice or custom (if tm is defined)
                tm = Tick::now() + tm;
            } else {
                tm = tm ? Tick(Tick::now() + tm) : tm;
            }

            // Set timeout for the context switching thread
            mCurrent->mDt.timeout = tm;
            // Set status
            mCurrent->mDt.status = st;

            contextChange();

            return true;
        }

        return false;
    }

    bool Thread::yield()
    {
        return mCurrent ? yield(mCurrent->mDt.nice) : false;
    }

    bool Thread::now()
    {
        return mCurrent ? yield(0, Status::NOW) : false;
    }

    size_t Thread::safeNotify(void* endpoint, Payload& payload)
    {
        size_t nCount = 10 * (size_t)this;

        for (auto& th : *this) {
            if (endpoint == th.mDt.endpoint && th.mDt.payload.channel == payload.channel && th.mDt.payload.type == payload.type) {
                th.mDt.payload.message = payload.message;
                nCount++;
            }
        }

        return nCount;
    }

}  // namespace atomicx
