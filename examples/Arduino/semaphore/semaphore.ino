/*

*/
#include "arduino.h"

#include "atomicx.hpp"

#include <stdio.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

using namespace thread;

void ListAllThreads();

size_t GetFreeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

atomicx_time Atomicx_GetTick(void)
{
    return millis();
}

void Atomicx_SleepTick(atomicx_time nSleep)
{
   //ListAllThreads();
    delay(nSleep);
}

constexpr size_t GetStackSize(size_t sizeRef)
{
    return sizeRef * sizeof (size_t);
}

size_t GlobalCounter=0;
atomicx::semaphore sem(2);

class Consumer : public atomicx
{
public:
    Consumer(uint32_t nNice) :  atomicx (m_stack), m_stack{}
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "consumer";
    }

    ~Consumer()
    {
        Serial.print (F("Deleting Consumer: ID: "));
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {
        bool  bTimeout = 0;

        while (true)
        {
            if ((bTimeout = sem.acquire (10000)))
            {
                Serial.print (GetName());
                Serial.print (F(":"));
                Serial.print ((size_t) this);
                Serial.print (F(" ACCQUIRED"));
                Serial.print (F(": GlobalCounter: "));
                Serial.print (GlobalCounter);
            }
            else
            {
                Serial.print (GetName());
                Serial.print (F(":"));
                Serial.print ((size_t) this);
                Serial.print (F(" TIMEOUT"));
            }

            Serial.print (F(", Ram: "));
            Serial.print (GetFreeRam ());
            Serial.print (F(", Stack used: "));
            Serial.print (GetUsedStackSize());
            Serial.print (F(", Max:"));
            Serial.print (sem.GetMaxAcquired ());
            Serial.print (F(", Acquired: "));
            Serial.print (sem.GetCount());
            Serial.print (F(", Waiting: "));
            Serial.print (sem.GetWaitCount ());
            Serial.print (F(",ExecTime: "));
            Serial.print (GetLastUserExecTime ());
            Serial.println (F("ms"));

            Serial.flush();

            Yield (GetNice ());

            if (bTimeout == true)
            {
                sem.release ();
            }
        }
    }

    void StackOverflowHandler(void) noexcept final
    {
        Serial.print (__FUNCTION__);
        Serial.print (F("["));
        Serial.print (GetName ());
        Serial.print ((size_t) this);
        Serial.print (F(": Stack used "));
        Serial.print (GetUsedStackSize());
        Serial.print (F("/"));
        Serial.println (GetStackSize());
        Serial.flush();
    }

private:
    uint8_t m_stack[::GetStackSize(20)];
};

class Producer : public atomicx
{
public:
    Producer(uint32_t nNice) : atomicx (m_stack), m_stack{}
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "Producer";
    }

    ~Producer()
    {
        Serial.print("Deleting Producer");
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {

        while (Yield ())
        {
            GlobalCounter++;
        }
    }

    void StackOverflowHandler(void) noexcept final
    {
        Serial.print (__FUNCTION__);
        Serial.print (F("["));
        Serial.print (GetName ());
        Serial.print ((size_t) this);
        Serial.print (F(": Stack used "));
        Serial.println (GetUsedStackSize());
        Serial.flush();
    }

private:
    uint8_t m_stack[::GetStackSize(20)];
    String m_threadName;
};

void ListAllThreads()
{
  size_t nCount=0;

   Serial.flush();

  Serial.println (F("[THREAD]-----------------------------------------------"));
  Serial.println (GetFreeRam ());
  Serial.println (F("---------------------------------------------------------"));

  Serial.print (F("Sizeof Producer:"));
  Serial.println (sizeof (Producer));

  Serial.print (F("Sizeof Consumer:"));
  Serial.println (sizeof (Consumer));

  Serial.println (F("---------------------------------------------------------"));

  for (auto& th : *(atomicx::GetCurrent()))
  {
      Serial.print (atomicx::GetCurrent() == &th ? "*  " : "   ");
      Serial.print (++nCount);
      Serial.print (F(":'"));
      Serial.print (th.GetName());
      Serial.print (F("' "));
      Serial.print ((size_t) &th);
      Serial.print (F(", Nice: "));
      Serial.print (th.GetNice());
      Serial.print (F(", Stack: "));
      Serial.print (th.GetStackSize());
      Serial.print (F(", UsedStack: "));
      Serial.print(th.GetUsedStackSize());
      Serial.print (F(", Status: "));
      Serial.print (th.GetStatus());
      Serial.print (F(", Ref Lock: "));
      Serial.println (th.GetTagLock());
      Serial.flush();
  }

  Serial.println (F("---------------------------------------------------------"));
  Serial.flush();
}


void setup()
{
  Serial.begin (115200);

  while (! Serial) delay (100);

  delay (2000);

  Serial.println (F("Starting UP-----------------------------------------------\n"));

  Serial.flush();

  Producer T1(500);
  Consumer E1(500);
  Consumer E2(500);
  Consumer E3(500);
  Consumer E4(500);
  Consumer E5(500);
  Consumer E6(500);
  Consumer E7(500);
  Consumer E8(500);

  ListAllThreads ();

  atomicx::Start();

  Serial.println (F("Full lock detected..."));

   ListAllThreads ();
}

void loop() {

}
