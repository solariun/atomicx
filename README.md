# AtomicX

Version 2.7.0 release

![image](https://user-images.githubusercontent.com/1805792/125191254-6591cf80-e239-11eb-9e89-d7500e793cd4.png)

What is AtomicX? AtomicX is a general purpose **cooperative** thread lib for embedded applications (single core or confined within other RTOS) that allows you partition your application "context" (since core execution) into several controlled context using cooperative thread. So far here nothing out of the ordinary, right? Lets think again:

* **DO NOT DISPLACE STACK, IT WILL STILL AVAILABLE FOR PROCESSING**, the *Stack Page* will only hold a backup of the most necessary information needed, allowing stacks in few bites most if the time. This implementation if highly suitable for Microcontrollers like ATINY85, for example, that only has 512 bites, and you can have 5 or more threads doing things for you, only backup the most important context information.
    * *IMPORTANT*: DO NOT USE CONTEXT MEMORY POINTER to exchange information to other threads, wait/notify and etc. All threads will use the *dafault stack memory* to execute, instead use Global variables, allocated memory or atomicx_smart_ptr objects.
    
* Since it implements Cooperative thread every execution will atomic between *atomicx* thrteads.

* CoreX **DO NOT DISPLACE STACK**, yes, it will use a novel technique that allow you to use full stack memory freely, and once done, just call `Yield()` to switch the context.
    1. Allow you to use all your stack during thread execution and only switch once back to an appropriate place
    ``` 
        Stack memory
        *-----------*
        |___________| Yield()
        |___________|    thread 0..N
        |___________|     |       .  - After context execution
        |___________|     |      /|\   is done, the developer can
        |___________|     |       |    choose where to switch 
        |___________|     |       |    context, saving only what is
        |___________|    \|/      |    necessary
        |___________|     ---------
        |           |     - During context
        *-----------*       can goes deeper as 
                            necessary
    ```
* Due to the **zero stack-displacement** technology, developers can ensure minimal stack memory page, allowing ultra sophisticated designes and execution stack diving and only backing up to the stack memory page what is necessary.
                           
* Full feature for IPC (_Inter Process Communication_)
    * Thread safe Queues for data/object transporting.
    * EVERY Smart Lock can transport information (atomicx::message)
    * Message is composed by "size_t `atomix::message` and a "size_t tag"
        * This novel concept of "tag"s for an atomicx::message gives the message meaning.
        * Since `atomicx::message` uses `size_t` messages can also transport pointers
    * Smart Locks can Lock and Shared Lock in the same object, making
    * Full QUEUE capable to transport objects. 

* Full feature for IPN (_Inter Process Notification_)
    * Thread can wait for an event to happen.
    * On event notification a `atomix::message` can be sent/received
    
* A message broker based on observer pattern
    * A thread can use `WaitBroker Message` to wait for any specifc topic asynchronously. 
    * Instead of having a `Subcrib` call, the developer will provide a `IsSubscribed` method that the kernel will use to determine if the object/thread is subscribed to a given topic.
    * Broker uses `atomicx::message` to transport information. For inter process Object transport, please use atomicx::queue.

* ALL *WAIT* actions will block the thread, on kernel level (setting thread to a waiting state), until the notification occurs. Alternatively the notification can be transport a `atomicx::message` structure (tag/message)
    * _WAIT_ and _NOTIFY_ (one or all) will use *any pointer* as the signal input, virtually any valid address pointer can  be used. *IMPORTANT*: Unless you know what you are doing, do *NOT* use context pointer (execution stack memory), use a global or allocated memory instead (including `atomicx::smart_prt`)
 
* **IMPORTANT** since all threads will be executed in the "_default_" stack memory, it will not be jailed in the stack size memory page, *DO NOT USE STACK ADDRESS TO COMMUNICATE* with another threads, use only global or alloced memory pointers to communicate

* **IMPORTANT** In order to operate with precision, specialise ticks by providing either `atomicx_time Atomicx_GetTick (void)` and `void Atomicx_SleepTick(atomicx_time nSleep)` to work within the timeframe (milleseconds, nanoseconds, seconds.. etc). Since AtomicX, also, provice, Sleep Tick functionality (to handle idle time), depending on the sleep time, to developer can redude the processor overall consuption to minimal whenever it is not necessary.
 
    * Since it will be provided by the developer, it gives the possibility to use external clocks, hardware sleep or lower consumptions and fine tune power and resource usages.  
 
    * If not specialization is done, the source code will use a simple and non-deterministic loop cycle to count ticks.
    
``` C++
//
//  main.cpp
//  atomicx
//
//  Created by GUSTAVO CAMPOS on 28/08/2021.
//

#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <cstdint>
#include <iostream>
#include <setjmp.h>
#include <string>

#include "atomicx.hpp"

using namespace thread;

#ifdef FAKE_TIMER
uint nCounter=0;
#endif

void ListAllThreads();

/*
 * Define the default ticket granularity
 * to milliseconds or round tick if -DFAKE_TICKER
 * is provided on compilation
 */
atomicx_time Atomicx_GetTick (void)
{
#ifndef FAKE_TIMER
    usleep (20000);
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (atomicx_time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
#else
    nCounter++;

    return nCounter;
#endif
}

/*
 * Sleep for few Ticks, since the default ticket granularity
 * is set to Milliseconds (if -DFAKE_TICKET provide will it will 
 * be context switch countings), the thread will sleep for 
 * the amount of time needed till next thread start.
 */ 
void Atomicx_SleepTick(atomicx_time nSleep)
{
#ifndef FAKE_TIMER
    usleep ((useconds_t)nSleep * 1000);
#else
    while (nSleep); usleep(100);
#endif
}

/*
 * Object that implements thread
 */
class Thread : public atomic
{
public:
    Thread(atomicx_time nNice, const char* pszName) : stack{}, atomic (stack), m_pszName(pszName)
    {
        SetNice(nNice);
    }

    ~Thread()
    {
        std::cout << "Deleting " << GetName() << ": " << (size_t) this << std::endl;
    }
    
    void run() noexcept override
    {
        size_t nCount=0;
        
        do
        {
            std::cout << __FUNCTION__ << ", Executing " << GetName() << ": " << (size_t) this << ", Counter: " << nCount << std::endl << std::flush
                        
            nCount++;
                        
        }  while (Yield());

    }

    void StackOverflowHandler (void) override
    {
        std::cout << __FUNCTION__ << ":" << GetName() << "_" << (size_t) this << ": needed: " << GetUsedStackSize() << ", allocated: " << GetStackSize() << std::endl;
    }

    const char* GetName (void) override
    {
        return m_pszName;
    }

private:
    uint8_t stack[1024];
    const char* m_pszName;
};


int main()
{
    Thread t1(200, "Producer 1");
    Thread t2(500, "Producer 2");
    

    std::cout << "Start contexts for thread 3" << std::endl;

    // This must creates threads and destroy on leaving {} context 
    {
        Thread t3_1(0, "Eventual 3.1");
        Thread t3_2(0, "Eventual 3.2");
        Thread t3_3(0, "Eventual 3.3");
    }
    
    
    std::cout << "end context" << std::endl;

    Thread t4(1000, "Producer 4");

    atomic::Start();
}

```
