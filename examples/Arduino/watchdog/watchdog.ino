/*

*/

#include "arduino.h"

#include "atomicx.hpp"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#include "watchdog.hpp"

using namespace thread;

size_t nDataCount=0;

void ListAllThreads();

atomicx_time Atomicx_GetTick(void)
{
    return millis();
}

void Atomicx_SleepTick(atomicx_time nSleep)
{
    delay(nSleep);
}

constexpr size_t GetStackSize(size_t sizeRef)
{
    return sizeRef * sizeof (size_t);
}
 
class ThreadAllarm : public atomicx, public Watchdog::Item
{
public:
    ThreadAllarm(uint32_t nNice) :  atomicx (::GetStackSize(20), 10), Item (GetThread (), 1000)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "ThreadFeeded";
    }

    virtual ~ThreadAllarm()
    {
        Serial.print("Deleting ThreadAllarm");
        Serial.print (", ID: ");
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {
        Serial.print (F(">>>>> Starting up: ")); Serial.println (GetName ()); Serial.flush ();
        delay (2000);

        do
        {
            Serial.print ("Executing: "); Serial.println (GetName ()); Serial.flush ();
            ListAllThreads ();

            if (Watchdog::GetInstance().GetAllarmCounter (GetID()) == 3)
            {
                Watchdog::GetInstance().Feed ();
            }
        } while (Yield(random (1000, 2000)));
    }

    void finish () noexcept override
    {
        Serial.print (F(">>>>>>>Detroying Thread, counter "));
        Serial.println (Watchdog::GetInstance().GetAllarmCounter (GetID()));
        Serial.print (F(": "));
        Serial.println (GetName ());
        Serial.flush ();
    }

    void StackOverflowHandler(void) noexcept final
    {
        Serial.print (__FUNCTION__);
        Serial.print ("[");
        Serial.print (GetName ());
        Serial.print ((size_t) this);
        Serial.print ("] Stack used ");
        Serial.print (GetUsedStackSize());
        Serial.print ("/");
        Serial.println (GetStackSize());
        Serial.flush();
    }

private:
};

class ThreadAllarm2 : public atomicx, Watchdog::Item
{
public:
    ThreadAllarm2(uint32_t nNice) :  atomicx (::GetStackSize(20), 10),  Item (GetThread (), 1800, true)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "ThreadStarved";
    }

    ~ThreadAllarm2()
    {
        Serial.print("Deleting ThreadAllarm");
        Serial.print (", ID: ");
        Serial.println ((size_t) this);
    }

protected:

    void run() noexcept override
    {
        Serial.print (F(">>>>> Starting up: ")); Serial.println (GetName ()); Serial.flush ();
        delay (2000);

        do
        {
            Serial.print ("Executing: "); Serial.println (GetName ()); Serial.flush ();
            ListAllThreads ();

            if (Watchdog::GetInstance().GetAllarmCounter (GetID()) == 3)
            {
                Watchdog::GetInstance().Feed ();
            }
        } while (Yield(random (1000, 2000)));
    }

    void finish () noexcept override
    {
        Serial.print (F(">>>>>>>Detroying Thread, counter "));
        Serial.println (Watchdog::GetInstance().GetAllarmCounter (GetID()));
        Serial.print (F(": "));
        Serial.println (GetName ());
        Serial.flush ();
    }

    void StackOverflowHandler(void) noexcept final
    {
        Serial.print (__FUNCTION__);
        Serial.print ("[");
        Serial.print (GetName ());
        Serial.print ((size_t) this);
        Serial.print ("] Stack used ");
        Serial.print (GetUsedStackSize());
        Serial.print ("/");
        Serial.println (GetStackSize());
        Serial.flush();
    }

private:
};

void ListAllThreads()
{
    size_t nCount=0;

    Serial.println ("[THREAD]-----------------------------------------------");
    Serial.print (F("Context size: ")); Serial.println (sizeof (thread::atomicx));
    Serial.println ("[List active threads]----------------------------------");

    Serial.flush();
    
    for (auto& th : *(atomicx::GetCurrent()))
    {
        Serial.print (atomicx::GetCurrent() == &th ? "*  " : "   ");
        Serial.print (++nCount);
        Serial.print (":'");
        Serial.print (th.GetName());
        Serial.print ("' ");
        Serial.print ((size_t) &th);
        Serial.print (", Nice: ");
        Serial.print (th.GetNice());
        Serial.print (", Stack: ");
        Serial.print (th.IsStackSelfManaged() ? 'A' : ' ');
        Serial.print (th.GetStackSize());
        Serial.print (", UsedStack: ");
        Serial.print(th.GetUsedStackSize());
        Serial.print (", Status: ");
        Serial.print (th.GetStatus());
        Serial.print (", WTD Allarm: ");
        Serial.println (Watchdog::GetInstance().GetAllarmCounter (th.GetID()));
        Serial.flush();
    }

    Serial.println ("");
    
    Serial.flush();
}


void setup()
{
    Serial.begin (115200);

    while (! Serial) delay (100);

    delay (2000);

    Serial.println ("Starting UP-----------------------------------------------\n");
    Serial.flush ();

    ThreadAllarm allarm1 (1000);
    ThreadAllarm2 allarm2 (1000);

    //ListAllThreads ();

    Serial.println ("running kernel\n");
    Serial.flush ();

    atomicx::Start();

    Serial.println ("Full lock detected...");

    ListAllThreads ();
}

void loop() {

}
