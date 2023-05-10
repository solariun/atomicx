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

atomicx::Tick atomicx::Tick::getTick(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    return (atomicx::Tick)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void atomicx::Tick::sleepTick(atomicx::Tick nSleep)
{
    usleep((useconds_t)nSleep * 1000);
}

class Test : public atomicx::Thread
{
private:
    size_t mStack[256]{};

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
        atomicx::Tick tk{0};
        while (yield()) {
            auto& dt = getParams();
            std::cout << this << ": time:" << tk.diff() << "ms: Value:" << nValue << ", Nice:" << dt.nice << ", Stack:" << dt.usedStackSize << "/"
                      << dt.stackSize << "b" << std::endl
                      << std::flush;
            nValue = add(nValue);

            tk = atomicx::Tick::getTick();
        }
    }

public:
    Test() : Thread(10, mStack)
    {}
};

Test t1[5];

int main()
{
    for (auto& th : t1[0]) {
        auto dt = th.getParams();
        std::cout << "Thread:" << &th << ", StackSize:" << dt.stackSize << std::endl;
    }

    atomicx::Thread::start();

    return 0;
}