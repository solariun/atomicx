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

atomicx::Tick atomicx::Tick::now(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    return (atomicx::Tick)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void atomicx::Tick::sleep(atomicx::Tick nSleep)
{
    usleep((useconds_t)nSleep * 1000);
}

class Test : public atomicx::Thread
{
public:
    Test() : Thread(0, mStack)
    {}

protected:
    size_t add(size_t nValue)
    {
        yield();
        nValue++;

        return nValue;
    }

    void run()
    {
        size_t nValue{0};
        atomicx::Tick tk{};
        while (true) {
            {
                auto& dt = getParams();
                std::cout << mId << ": time:" << tk.diff() << "ms: Value:" << nValue << ", Nice:" << dt.nice << ", Stack:" << dt.usedStackSize << "/"
                          << dt.stackSize << "b"
                          << ", Tick_t:" << sizeof(atomicx::Tick_t) << std::endl
                          << std::flush;
            }

            tk = atomicx::Tick::now();
            nValue = add(nValue);

            // yield(0, atomicx::Status::NOW);
        }
    }

private:
    size_t mStack[64]{};
    static size_t mIdCounter;
    size_t mId{mIdCounter++};
};

size_t Test::mIdCounter{0};

Test t1[1000];

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

    printf("The size of Tick_t = %zu\n", sizeof(atomicx::Tick_t));

    for (auto& th : t1[0]) {
        auto dt = th.getParams();
        std::cout << "Thread:" << &th << ", StackSize:" << dt.stackSize << std::endl;
    }

    atomicx::Thread::start();

    return 0;
}