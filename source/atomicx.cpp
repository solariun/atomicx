
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "atomicx.hpp"

namespace atomicx
{
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

    bool Thread::start()
    {
        bool ret = false;

        if (mBegin != nullptr) {
            mCurrent = mBegin;

            volatile uint8_t stackBegin = 0xAA;
            mStackBegin = &stackBegin;

            while (mCurrent) {
                if (setjmp(mJmpStart) == 0) {
                    switch (mCurrent->mStatus) {
                        case Status::STARTING:
                            mCurrent->mStatus = Status::RUNNING;
                            mCurrent->run();
                            break;

                        case Status::RUNNING:
                            longjmp(mCurrent->mJmpThread, 1);
                            break;
                        default:
                            break;
                    }
                }

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
            mCurrent->mUsedStackSize = static_cast<size_t>(mCurrent->mStackBegin - mCurrent->mStackEnd);

            memcpy((void*)&mCurrent->mVirtualStack, (const void*)mCurrent->mStackEnd, mCurrent->mUsedStackSize);

            if (setjmp(mCurrent->mJmpThread) == 0) {
                longjmp(mCurrent->mJmpStart, 1);
            }

            memcpy((void*)mCurrent->mStackEnd, (const void*)&mCurrent->mVirtualStack, mCurrent->mUsedStackSize);

            return true;
        }

        return false;
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
}  // namespace atomicx
