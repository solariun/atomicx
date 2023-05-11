
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "atomicx.hpp"

namespace atomicx
{
    /* Tick object and integer emulator */
    Tick::Tick() : mTick(getTick()){};
    Tick::Tick(Tick_t val) : mTick(val){};

    // Tick& Tick::Tick::operator=(Tick& tick)
    // {
    //     mTick = tick.mTick;
    // }

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
        mTick = getTick();
        return *this;
    }

    Tick_t Tick::diffT(Tick tick) const
    {
        return (tick - mTick);
    }

    Tick_t Tick::diffT() const
    {
        return (getTick() - mTick);
    }

    Tick Tick::diff(Tick tick) const
    {
        return Tick(tick - mTick);
    }

    Tick Tick::diff() const
    {
        return Tick(getTick() - mTick);
    }

    /* TIMEOUT */

    /*
     * timeout methods implementations
     */

    Timeout::Timeout() : m_timeoutValue(0)
    {
        set(0);
    }

    Timeout::Timeout(Tick nTimeoutValue) : m_timeoutValue(nTimeoutValue ? nTimeoutValue + Tick::getTick() : 0)
    {}

    bool Timeout::operator>(Timeout& tm)
    {
        return m_timeoutValue > tm.m_timeoutValue;
    }

    bool Timeout::operator<(Timeout& tm)
    {
        return m_timeoutValue < tm.m_timeoutValue;
    }

    bool Timeout::operator!=(Timeout& tm)
    {
        return m_timeoutValue != tm.m_timeoutValue;
    }

    bool Timeout::operator==(Timeout& tm)
    {
        return m_timeoutValue == tm.m_timeoutValue;
    }

    // bool Timeout::operator<(Timeout& tm);
    // bool Timeout::operator!=(Timeout& tm);
    // bool Timeout::operator==(Timeout& tm);

    void Timeout::set(Tick nTimeoutValue)
    {
        m_timeoutValue = nTimeoutValue ? nTimeoutValue + Tick::getTick() : 0;
    }

    bool Timeout::operator=(Tick tm)
    {
        return (m_timeoutValue = tm);
    }

    void Timeout::update()
    {
        m_timeoutValue = Tick::getTick();
    }

    bool Timeout::hasTimeout() const
    {
        return (m_timeoutValue == 0 ? false : true);
    }

    Timeout::operator bool() const
    {
        return (m_timeoutValue == 0 || m_timeoutValue > Tick::getTick()) ? false : true;
    }

    Tick Timeout::until() const
    {
        auto nNow = Tick::getTick();

        return (m_timeoutValue && nNow < m_timeoutValue) ? m_timeoutValue - Tick::getTick() : 0;
    }

    Tick Timeout::since(Tick startTime)
    {
        return startTime - until();
    }

    Tick Timeout::since()
    {
        return Tick::getTick() - until();
    }

    /* THREAD */
    Thread* Thread::mBegin{nullptr};
    Thread* Thread::mEnd{nullptr};
    Thread* Thread::mCurrent{nullptr};

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
    }

    void Thread::scheduler()
    {
        if (mCurrent == nullptr) return;

        Tick now;
        Thread* candidate = nullptr;
        bool stop = false;

        for (auto& th : *mCurrent) {
            switch (mCurrent->mDt.status) {
                case Status::STARTING:
                case Status::NOW:
                    candidate = &th;
                    candidate->mDt.timeout.update();
                    stop = false;
                    break;
                case Status::PAUSED:
                case Status::RUNNING:
                case Status::WAIT:
                    if (!candidate || th.mDt.timeout < candidate->mDt.timeout) candidate = &th;
                    break;
            }

            if (stop) break;
        }
    }

    bool Thread::start()
    {
        bool ret = false;

        if (mBegin != nullptr) {
            mCurrent = mBegin;

            volatile uint8_t stackBegin = 0xAA;
            mStackBegin = &stackBegin;

            while (mCurrent) {
                if (setjmp(mJmpStart) == 0) {
                    switch (mCurrent->mDt.status) {
                        case Status::STARTING:
                            mCurrent->mDt.status = Status::RUNNING;
                            mCurrent->run();
                            break;
                        default:
                            break;
                    }

                    longjmp(mCurrent->mJmpThread, 1);
                }

                scheduler();
                mCurrent = mCurrent->mNext ? mCurrent->mNext : mBegin;
            }
        }

        return ret;
    }

    bool Thread::yield()
    {
        if (mCurrent) {
            volatile uint8_t stackEnd = 0xBB;
            mCurrent->mStackEnd = &stackEnd;
            mCurrent->mDt.usedStackSize = static_cast<size_t>(mCurrent->mStackBegin - mCurrent->mStackEnd);

            memcpy((void*)&mCurrent->mVirtualStack, (const void*)mCurrent->mStackEnd, mCurrent->mDt.usedStackSize);

            if (setjmp(mCurrent->mJmpThread) == 0) {
                mCurrent->mDt.status = Status::PAUSED;
                longjmp(mCurrent->mJmpStart, 1);
            }

            memcpy((void*)mCurrent->mStackEnd, (const void*)&mCurrent->mVirtualStack, mCurrent->mDt.usedStackSize);

            // mCurrent->mDt.status = Status::RUNNING;
            return true;
        }

        return false;
    }

}  // namespace atomicx
