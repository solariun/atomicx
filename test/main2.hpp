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

class th : atomicx::Thread
{
    private:

        volatile size_t nStack [100];

    public:

    th () : Thread (100, nStack)
    {
        std::cout << (size_t) this << ": Initiating." << std::endl;
    }

    ~th ()
    {
        std::cout << (size_t) this << ": Deleting." << std::endl;
    }

    void yield_in ()
    {
       Yield ();
    }

    void yield ()
    {
        yield_in ();
    }

    virtual void run ()
    {
        int nValue = 0;

        yield ();
        
        while (true)
        {
            yield ();

            TRACE (TRACE, ">>> Val: " << nValue++);

            break;
        }

        TRACE (TRACE, "Leaving thread.");
    }

    virtual const char* GetName () final
    {
        return "Thread Test";
    }
};

int main ()
{
    th th1;
    th th2;
    th th3;
    th th4;

    std::cout << "Beging Application" << std::endl << std::endl;

    std::cout << "-------------------------------" << std::endl;

    for (atomicx::Thread& a : atomicx::kernel)
    {
        std::cout << (size_t) &a << " thread" << std::endl;
    }

    std::cout << "-------------------------------" << std::endl << std::endl;

    atomicx::kernel.Join ();
    
    std::cout << "End Application" << std::endl << std::endl;
       
    return 0;
}