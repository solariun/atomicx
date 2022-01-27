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
atomicx::lock gLock;

void ListAllThreads();

atomicx_time Atomicx_GetTick(void)
{
    ::yield();
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

class Eventual : public atomicx
{
 public:
    Eventual(uint32_t nNice, const char* pszName) :  atomicx (m_stack), m_stack{}
    {
        m_threadName = pszName;

        SetNice(nNice);

        started = true;
    }

    const char* GetName () override
    {
        return m_threadName.c_str();
    }

    ~Eventual()
    {
        Serial.print (F("Deleting: "));
        Serial.print (GetName());
        Serial.print (F(", ID: "));
        Serial.println ((size_t) this);
        started = false;
    }

    void run() noexcept override
    {
        int nCount=0;

        Serial.println (F("Starting EVENTUAL....."));

        for (nCount=0; nCount < 10; nCount ++)
        {
            Serial.print (F("\n\n[[[[[[[[[[[["));
            Serial.print (GetName ());
            Serial.print (F(":"));
            Serial.print ((size_t) this);
            Serial.print (F(" - "));
            Serial.print (F(" Eventual counter "));
            Serial.print (nCount);
            Serial.println (F("]]]]]]]]]]]\n\n"));

            Yield ();
        }
    }

    void finish () noexcept override
    {
        Serial.println (F("\n\n>>>>>>>DELETING EVENTUAL.... \n\n\n"));
        delete this;
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

    static bool GetStarted()
    {
        return started;
    }

private:
    uint8_t m_stack[::GetStackSize(40)];
    String m_threadName="";
    static bool started;
};

bool Eventual::started = false;

class Consumer : public atomicx
{
public:
    Consumer(uint32_t nNice, const char* pszName) :  atomicx (m_stack), m_stack{}
    {
        m_threadName = pszName;

        SetNice(nNice);
    }

    const char* GetName () override
    {
        return m_threadName.c_str();
    }

    ~Consumer()
    {
        Serial.print (F("Deleting Consumer: "));
        Serial.print (F(", ID: "));
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {
        size_t nCount=0;

        do
        {
            Serial.println (F("Wait for SharedLock"));
            SmartLock sLock(gLock);
            sLock.SharedLock ();

            Serial.println (F(">>> SheredLock accquired..."));

            nCount = nDataCount;

            ListAllThreads();

            Yield();
            // --------------------------------------

            Serial.print (F("Executing Consumer "));
            Serial.print (GetName());
            Serial.print (F(": "));
            Serial.print ((size_t) this);
            Serial.print (F(": Stack used: "));
            Serial.print (GetUsedStackSize());
            Serial.print (F(", locks/shared:"));
            Serial.print (gLock.IsLocked());
            Serial.print (F("/"));
            Serial.print (gLock.IsShared());
            Serial.print (F(", Counter: "));
            Serial.println (nCount);

            Serial.flush();

            Serial.println (F("SharedUlocking"));
        } while (Yield());
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
    uint8_t m_stack[::GetStackSize(45)];
    String m_threadName="";
};


class Producer : public atomicx
{
public:
    Producer(uint32_t nNice, const char* pszName) : atomicx (m_stack), m_stack{}, m_threadName{pszName}
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return m_threadName.c_str();
    }

    ~Producer()
    {
        Serial.print("Deleting ");
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {
        size_t nCount=0;

        do
        {
            Serial.println (F("Wait for Lock"));

            SmartLock sLock(gLock);
            sLock.Lock();

            Serial.println (F(">>> Locked..."));

            Serial.flush ();

            if (Eventual::GetStarted() == false)
            {
                Serial.println (F(">>>> Creating EVENTUAL thread... <<<<"));
                Serial.flush();
                new Eventual (1000, "Eventual");
            }
            else
            {
                Serial.println (F(">>>> EVENTUAL IS TRUE... <<<<"));
                Serial.println ();
            }

            Serial.print (F("Executing "));
            Serial.print (GetName());
            Serial.print (F(": "));
            Serial.print (GetName ());
            Serial.print ((size_t) this);
            Serial.print (F(", locks/shared:"));
            Serial.print (gLock.IsLocked());
            Serial.print (F(", "));
            Serial.print (sLock.IsLocked());
            Serial.print (F("/"));
            Serial.print (gLock.IsShared());
            Serial.print (F(", Counter: "));
            Serial.print (nCount);
            Serial.print (F(", Eventual: "));
            Serial.println (Eventual::GetStarted());

            Serial.flush();

            nDataCount++;

            Yield (5000);

            Serial.println (F("Unlocking"));

            //atomicx::smart_ptr<Consumer> Consumer_thread (new Consumer(100, "t::Consumer"));

        } while (Yield ());
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
    uint8_t m_stack[::GetStackSize(40)];
    String m_threadName;
};

void ListAllThreads()
{
  size_t nCount=0;

   Serial.flush();

  Serial.println (F("[THREAD]-----------------------------------------------"));
  Serial.print (F("IsLocked: "));
  Serial.print ((int) gLock.IsLocked());
  Serial.print (F(", IsShared: "));
  Serial.println ((int) gLock.IsShared());
  Serial.println (F("---------------------------------------------------------"));

  Serial.print (F("Sizeof Producer:"));
  Serial.println (sizeof (Producer));

  Serial.print (F("Sizeof Consumer:"));
  Serial.println (sizeof (Consumer));

  Serial.print (F("Sizeof Eventual:"));
  Serial.println (sizeof (Eventual));

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


  Producer T1(100, u8"Producer 1");
  Consumer E1(100, u8"Consumer 1");
  Consumer E4(300, u8"Consumer 3");

  ListAllThreads ();

  atomicx::Start();

  Serial.println (F("Full lock detected..."));

   ListAllThreads ();
}

void loop() {

}
