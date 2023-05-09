//
//  main.cpp
//  atomic
//
//  Created by GUSTAVO CAMPOS on 29/08/2021.
//

#include "atomicx.hpp"

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>

#include <stdlib.h>
#include <iostream>

atomicx::Tick atomicx::Thread::getTick (void)
{
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (atomicx::Tick)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void atomicx::Thread::sleepTick(atomicx::Tick nSleep)
{
    usleep ((useconds_t)nSleep * 1000);
}

class Test : public atomicx::Thread {
private:
  size_t mStack[256]{};

protected:
  size_t add(size_t nValue) {
    yield();
    nValue++;

    return nValue;
  }
  void run() {
    size_t nValue{0};

    while (yield()) {
      std::cout << this << ": Value:" << nValue << std::endl << std::flush;

      nValue = add(nValue);
    }
  }

public:
  Test() : Thread(0, mStack) {}
};

Test t1[10];

int main() {
  for (auto &th : t1[0]) {
    std::cout << "Thread:" << &th << std::endl;
  }

  atomicx::Thread::start();

  return 0;
}