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


class Watchdog: public atomicx
{
public:
    struct WtdgControlType
    {
        size_t nThreadId;
        thread::atomicx::Timeout nextAllarm;
        atomicx_time allowedTime;

        bool isRequired : 1;
        uint8_t recoverCounter : 2;
    };

    static Watchdog instance;

    const char* GetName () override
    {
        return "Wathdog";
    }

    ~Watchdog()
    {
        Serial.print("Deleting Consumer: ");
        Serial.print (", ID: ");
        Serial.println ((size_t) this);
    }

    uint8_t GetAllarmCounter (size_t threadId)
    {
        auto* pwtdItem = FindThread (threadId);
        uint8_t nRet = 0;

        if (pwtdItem)
        {
            nRet = pwtdItem->recoverCounter;
        }

        return nRet;
    }

    WtdgControlType* FindThread (size_t threadId)
    {
        for (auto& wtdItem : WtdgControl)
        {
            if ((size_t) wtdItem.nThreadId == threadId)
            {
                return &wtdItem;
            }
        }

        return nullptr;
    }

    bool Feed ()
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

    bool AttachThread (thread::atomicx& thread, atomicx_time allowedTime, bool isRequired)
    {
        auto* pwtdItem = FindThread (&thread);

        if (pwtdItem)
        {
            abort ("Thread already attached");
        }
        else if (! pwtdItem)
        {
            pwtdItem = FindThread (0);

            if (pwtdItem != nullptr)
            {
                pwtdItem->nThreadId = &thread;
                pwtdItem->isRequired = isRequired;
                pwtdItem->allowedTime = allowedTime;
                pwtdItem->nextAllarm.Set (pwtdItem->allowedTime);
                pwtdItem->recoverCounter = 0;

                return true;
            }

            abort ("Could not Attach thread to Watchdog.");

        }

        return false;
    }

    bool DetachThread ()
    {
        auto* pwtdItem = FindThread (GetID ());

        if (pwtdItem)
        {
            if (pwtdItem->isRequired == true)
            {
                abort ("The thread is required and cannot be destroyed.");
            }

            if (memset ((void*) pwtdItem, 0, sizeof (WtdgControlType)) == nullptr)
            {
                abort ("Failed to set the Watchdog controller item clean.");
            }
        }
    }

protected:

    void abort (const char* abortMessage)
    {
        Serial.print ("Watchdog: ");
        Serial.print (GetCurrent()->GetID ());
        Serial.print (":");
        Serial.print (abortMessage);
        Serial.flush ();

        Serial.println (">>> RESETING\n\n");
        Serial.flush ();

        void (*resetptr)(void) = 0x0000;
        resetptr();
    }

    void run() noexcept override
    {
        do
        {
            for (auto& wtdItem : WtdgControl)
            {
                if (wtdItem.nextAllarm.IsTimedout ())
                {
                    if (wtdItem.recoverCounter == 3)
                    {
                        abort ("Maximum recovery time reached, restarting");
                    }
                    else
                    {
                        wtdItem.nextAllarm.Set (wtdItem.allowedTime/2);
                        wtdItem.recoverCounter++;
                    }
                }
            }
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

    Watchdog() : atomicx (::GetStackSize(20), 10)
    {
        SetNice(1000);

        if (memset((void*) &WtdgControl, 0, sizeof (WtdgControl)) == nullptr)
        {
            abort ("Failed to start the Watchdor controller");
        }
    }

    WtdgControlType WtdgControl [10];
};

Watchdog Watchdog::instance;

class ThreadAllarm : public atomicx
{
public:
    ThreadAllarm(uint32_t nNice) :  atomicx (::GetStackSize(20), 10)
    {
        SetNice(nNice);

        Watchdog::instance.AttachThread (*this, 1500, true);
    }

    const char* GetName () override
    {
        return "ThreadFeeded";
    }

    ~ThreadAllarm()
    {
        Serial.print("Deleting ThreadAllarm");
        Serial.print (", ID: ");
        Serial.println ((size_t) this);

        Watchdog::instance.DetachThread ();
    }

    void run() noexcept override
    {
        do
        {
            ListAllThreads ();

            if (Watchdog::instance.Feed () == false)
            {
                Serial.println ("Failed to feed...");
                Serial.flush ();
            }

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

class ThreadAllarm2 : public atomicx
{
public:
    ThreadAllarm2(uint32_t nNice) :  atomicx (::GetStackSize(20), 10)
    {
        SetNice(nNice);

        Watchdog::instance.AttachThread (*this, 2000, true);
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

        Watchdog::instance.DetachThread ();
    }

    void run() noexcept override
    {
        do
        {
            ListAllThreads ();
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

void ListAllThreads()
{
  size_t nCount=0;

   Serial.flush();

  Serial.println ("[THREAD]-----------------------------------------------");

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
      Serial.print (", WTD Allarm: ");
      Serial.println (Watchdog::instance.GetAllarmCounter (th.GetID()));
      Serial.flush();
  }

  Serial.println ("---------------------------------------------------------");
  Serial.flush();
}


void setup()
{
    Serial.begin (115200);

    while (! Serial) delay (100);

    delay (2000);

    Serial.println ("Starting UP-----------------------------------------------\n");

    ThreadAllarm allarm1 (1000);
    ThreadAllarm2 allarm2 (1000);

    ListAllThreads ();

    atomicx::Start();

    Serial.println ("Full lock detected...");

    ListAllThreads ();
}

void loop() {

}
