/**
 * @file watchdog.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __WATCHDOG_HPP__
#define __WATCHDOG_HPP__

#include "arduino.h"

#include "atomicx.hpp"

using namespace thread;

class Watchdog: public atomicx
{
public:
    /**
     * @brief Item class used to make any atomicx thread attachable
     */
    class Item : public LinkItem<Item>
    {
    public:
        Item () = delete;

        Item (atomicx& thread, atomicx_time allowedTime = 1000, bool isCritical = false);

        virtual ~Item ();

    public:
        friend class Watchdog;

        size_t nThreadId = 0;
        atomicx::Timeout nextAllarm;
        atomicx_time allowedTime=0;

        bool isCritical : 1;
        uint8_t recoverCounter : 2;
    };

    static Watchdog& GetInstance();

    const char* GetName () override;

    ~Watchdog();

    uint8_t GetAllarmCounter (size_t threadId);

    Item* FindThread (size_t threadId);

    bool Feed ();
    
    bool AttachThread (Item& thread);

    bool DetachThread (Item& thread);

protected:

    void abort (const char* abortMessage);

    void run() noexcept override;

    void StackOverflowHandler(void) noexcept final;

private:

    Watchdog();

    LinkList<Item> list;
};

#endif