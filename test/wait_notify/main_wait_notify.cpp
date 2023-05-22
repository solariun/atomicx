//
//  main.cpp
//  atomic
//
//  Created by GUSTAVO CAMPOS on 29/08/2021.
//

#include "atomicx.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <iostream>

atx::Tick atx::now(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    return (atx::Tick)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void atx::sleep(atx::Tick nSleep)
{
    usleep((useconds_t)nSleep * 1000);
}

uint8_t endpoint{0};
atx::Thread::Mutex mutex;

class TestRecv : public atx::Thread
{
public:
    TestRecv() : Thread(100, mStack)
    {}

protected:
    void add()
    {
        atx::Tick tk{};
        //yield();

        atx::Payload payload{1, 1};
        wait(endpoint, payload, 100);

        {
            auto& dt = getParams();
            TRACE(INFO,
                  statusName(dt.status) << ":" << mId << ": time:" << tk.diff() << "ms, Nice:" << dt.nice
                                        << ", Stack:" << dt.usedStackSize << "/" << dt.stackSize << "b"
                                        << ", Tick_t:" << sizeof(atx::Tick_t) << ", MSG:" << payload.message);
            // if (dt.status == atx::Status::TIMEOUT)
            //     exit(1);
        }
    }

    void run() override
    {
        while (true) {
            add();
        }
    }

    const char* getName() const override
    {
        return "TH:WAIT";
    }

private:
    size_t mStack[64]{};
    static size_t mIdCounter;
    size_t mId{mIdCounter++};
};

size_t TestRecv::mIdCounter{0};

class Test : public atx::Thread
{
public:
    Test() : Thread(10, mStack)
    {}

protected:
    size_t add(size_t nValue)
    {
        yield();

        notify(endpoint, {1, nValue}, 100);
        nValue++;

        return nValue;
    }

    void run() override
    {
        size_t nValue{0};
        atx::Tick tk{};
        while (true) {
            {                
                auto& dt = getParams();
                TRACE(INFO,
                      statusName(dt.status) << mId << ": time:" << tk.diff() << "ms: Value:" << nValue << ", Nice:" << dt.nice
                                            << ", Stack:" << dt.usedStackSize << "/" << dt.stackSize << "b"
                                            << ", Tick_t:" << sizeof(atx::Tick_t));

                if (dt.status == atx::Status::TIMEOUT)
                    exit(1);
            }

            tk = atx::now();
            nValue = add(nValue);
        }
    }

    const char* getName() const override
    {
        return "TH:NOTF";
    }

private:
    size_t mStack[64]{};
    static size_t mIdCounter;
    size_t mId{mIdCounter++};
};

size_t Test::mIdCounter{0};

Test t1[1];
TestRecv r1[1];

int main()
{
    printf("The number of bits in a byte %d\n", CHAR_BIT);

    printf("The minimum value of SIGNED CHAR = %d\n", SCHAR_MIN);
    printf("The maximum value of SIGNED CHAR = %d\n", SCHAR_MAX);
    printf("The maximum value of UNSIGNED CHAR = %d\n", UCHAR_MAX);

    printf("The minimum value of SHORT INT = %d\n", SHRT_MIN);
    printf("The maximum value of SHORT INT = %d\n", SHRT_MAX);

    printf("The minimum value of INT = %d\n", INT_MIN);
    printf("The maximum value of INT = %d\n", INT_MAX);

    printf("The minimum value of CHAR = %d\n", CHAR_MIN);
    printf("The maximum value of CHAR = %d\n", CHAR_MAX);

    printf("The minimum value of LONG = %ld\n", LONG_MIN);
    printf("The maximum value of LONG = %ld\n", LONG_MAX);

    printf("The size of Tick_t = %zu\n", sizeof(atx::Tick_t));

    for (auto& th : t1[0]) {
        auto dt = th.getParams();
        std::cout << "Thread:" << &th << ", StackSize:" << dt.stackSize << std::endl;
    }

    atx::Thread::start();

    return 0;
}