/**
 * @file watchdog.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "watchdog.hpp"

Watchdog::Item::Item (atomicx& thread, atomicx_time allowedTime, bool isCritical):
    nThreadId (&thread), 
    nextAllarm (),
    allowedTime (allowedTime), 
    isCritical (isCritical), 
    recoverCounter(0)            
{
    this->nextAllarm.Set (allowedTime);
    Watchdog::GetInstance().AttachThread (*this);
}

Watchdog::Item::~Item ()
{
    Watchdog::GetInstance().DetachThread (*this);
}

Watchdog& Watchdog::GetInstance()
{
    static Watchdog instance;
    return instance;
}

const char* Watchdog::GetName ()
{
    return "Watchdog";
}

Watchdog::~Watchdog()
{}

uint8_t Watchdog::GetAllarmCounter (size_t threadId)
{
    auto* pwtdItem = FindThread (threadId);
    uint8_t nRet = 0;

    if (pwtdItem)
    {
        nRet = (*pwtdItem)().recoverCounter;
    }

    return nRet;
}

Watchdog::Item* Watchdog::FindThread (size_t threadId)
{
    for (auto& wtdItem : list)
    {
        if ((size_t) wtdItem().nThreadId == threadId)
        {
            return &(wtdItem());
        }
    }

    return nullptr;
}

bool Watchdog::Feed ()
{
    auto* pwtdItem = FindThread (thread::atomicx::GetCurrent()->GetID ());

    if (pwtdItem != nullptr)
    {
        pwtdItem->nextAllarm.Set(pwtdItem->allowedTime);
        pwtdItem->recoverCounter=0;
        return true;
    }

    return false;
}

bool Watchdog::AttachThread (Item& thread)
{
    list.AttachBack (thread);
}

bool Watchdog::DetachThread (Item& thread)
{
    list.Detach (thread);
}

void Watchdog::abort (Item& item, const char* abortMessage)
{
    Serial.print (F("Watchdog: "));
    Serial.print (item.nThreadId);
    Serial.print (": counter ");
    Serial.print (item.recoverCounter);
    Serial.print (": ");
    Serial.println (abortMessage);
    Serial.flush ();

    Serial.println (F(">>> RESETING\n\n"));
    Serial.flush ();

    void (*resetptr)(void) = 0x0000;
    resetptr();
}

void Watchdog::run()
{
    do
    {
        for (auto& wtdItem : list)
        {
            if (wtdItem().nextAllarm.IsTimedout ())
            {
                if (wtdItem().recoverCounter == 3)
                {
                    if (wtdItem().isCritical)
                    {
                        abort (wtdItem(), "Maximum recovery time reached, restarting");
                    }
                    else
                    {
                        atomicx* pthread = thread::atomicx::GetThread (wtdItem().nThreadId);

                        if (pthread && pthread->IsStopped () == false)
                        {
                            pthread->Restart ();
                            wtdItem().nextAllarm.Set (wtdItem().allowedTime);
                            wtdItem().recoverCounter = 0;
                        }
                    }
                }
                else
                {
                    wtdItem().nextAllarm.Set ((wtdItem().allowedTime)/2);
                    wtdItem().recoverCounter++;
                }
            }
        }

        Serial.println ("----");
        Serial.flush ();

    } while (Yield());
}

void Watchdog::StackOverflowHandler(void)
{
    Serial.print (__FUNCTION__);
    Serial.print (F("["));
    Serial.print (GetName ());
    Serial.print ((size_t) this);
    Serial.print (F("] Stack used "));
    Serial.print (GetUsedStackSize());
    Serial.print (F("/"));
    Serial.println (GetStackSize());
    Serial.flush();
}

Watchdog::Watchdog() : atomicx (CALCSTACKSIZE(20), 10)
{
    SetNice(1000);
}
