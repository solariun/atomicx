/*

*/

#include "arduino.h"

#include "atomicx.hpp"

#include <stdio.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

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

class Consumer : public atomicx
{
public:
    Consumer(uint32_t nNice) :  atomicx (::GetStackSize(10), 10)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "Consumer";
    }

    ~Consumer()
    {
        Serial.print("Deleting Consumer: ");
        Serial.print (", ID: ");
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {
        size_t nCount=0;
        size_t nMessage = 0;

        do
        {
            if (Wait (nMessage, nDataCount, 1, 10000) == false)
            {
                Serial.print ("Consumer ID:");
                Serial.print (GetID());
                Serial.println (", Waiting for message timedout, resuming...");
            }
            else
            {
                Serial.print ("Consumer ID:");
                Serial.print (GetID());
                Serial.print (", Message: ");
                Serial.println (nMessage);
            }

            Serial.flush ();
        } while (Yield());
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


class Producer : public atomicx
{
public:
    Producer(uint32_t nNice) : atomicx ()
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "Producer";
    }

    ~Producer()
    {
        Serial.print("Deleting ");
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {
        size_t nNotified=0;

        do
        {
            nDataCount++;

            Serial.println (F("Notifying...."));
            Serial.println ();

            ListAllThreads();

            if (LookForWaitings (nDataCount, 1, 500, 3))
            {
                if ((nNotified = Notify (nDataCount, nDataCount, 1, NotifyType::all)) == 0)
                {
                    Serial.println ("Consumer: WARNING... Failed to notify any thread.");
                    Serial.println ("Producer: All consumer threads BUSY, trying again...");
                }
                else
                {
                    Serial.println ("--------------------------------------");
                    Serial.print ("All messages dispatched to ");
                    Serial.println (nNotified);
                    Serial.println ("--------------------------------------");
                }
            }
            else
            {
                Serial.print (HasWaitings (nDataCount, 1));
                Serial.println (F(" waits: At least 3 consumer must be notified"));
            }

            Serial.flush ();

        } while (Yield ());
    }

    void StackOverflowHandler(void) noexcept final
    {
        Serial.print (__FUNCTION__);
        Serial.print ("[");
        Serial.print (GetName ());
        Serial.print ((size_t) this);
        Serial.print ("] Stack used ");
        Serial.println (GetUsedStackSize());
        Serial.flush();
    }

private:
    // Using auto-stack instead
};

void ListAllThreads()
{
  size_t nCount=0;

   Serial.flush();

  Serial.println ("[THREAD]-----------------------------------------------");

  Serial.print ("Sizeof Producer:");
  Serial.println (sizeof (Producer));

  Serial.print ("Sizeof Consumer:");
  Serial.println (sizeof (Consumer));

  Serial.println ("---------------------------------------------------------");

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
      Serial.print (", Ref Lock: ");
      Serial.println (th.GetTagLock());
      Serial.flush();
  }

  Serial.println ("---------------------------------------------------------");
  Serial.flush();
}

Producer T1(500);

void setup()
{
  Serial.begin (115200);

  while (! Serial) delay (100);

  delay (2000);

  Serial.println ("Starting UP-----------------------------------------------\n");

  Serial.flush();

    Consumer E1(1);
    Consumer E2(1);
    Consumer E3(1);
    Consumer E4(1);

  ListAllThreads ();

  atomicx::Start();

  Serial.println ("Full lock detected...");

   ListAllThreads ();
}

void loop() {

}
