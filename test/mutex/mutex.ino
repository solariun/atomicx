
#include "atomicx.hpp"

atomicx_time atomicx::Thread::GetTick(void)
{
    return millis();
}

void atomicx::Thread::SleepTick(atomicx_time nSleep)
{
    delay(nSleep);
}

atomicx::Mutex mutex;

uint32_t nValue = 0;


void PrintProcess (atomicx::Thread& endpoint)
{
    atomicx_time tm = atomicx::Thread::GetTick ();

    Serial.print ("------------------------------------- Now: ");
    Serial.println (tm);

    for (auto& th :endpoint)
    {
        Serial.print (&th == &endpoint ? '*' : ' ');
        Serial.print (th.GetName ());
        Serial.print ((size_t) &th);
        
        Serial.print (F("\t"));
        Serial.print (atomicx::GetStatusName (th.GetStatus ()));

        Serial.print (F("\t"));
        Serial.print (th.GetStackSize ());
        Serial.print (F("/"));
        Serial.print (th.GetMaxStackSize ());

        Serial.print (F("\t"));
        Serial.print ((int32_t) (th.GetNextEvent () - tm));
        Serial.print (F("/"));
        Serial.println (th.GetNextEvent ());

        Serial.flush ();
    }

    Serial.println ("-------------------------------------");
}

class Reader : public atomicx::Thread
{
    private:
        volatile size_t nStack [40];

    public:

    Reader () = delete;

    Reader (atomicx_time nNice) : Thread (nNice, nStack)
    {
        // Serial.print ((size_t) this);
        // Serial.println (F(": Initiating."));
        // Serial.flush ();
    }

    virtual ~Reader () final
    {
        Serial.print ((size_t) this);
        Serial.println (F(": Deleting."));
        Serial.flush ();
    }

    virtual void run () final 
    {
        Serial.print ((size_t) this);
        Serial.println (F(": Starting up thread."));
        Serial.flush ();

        while (Yield ())
        {
                Serial.print (GetName ());
                Serial.print ((size_t) this);
                Serial.print (F(": bExclusiveLock: "));
                Serial.print (mutex.GetExclusiveLockStatus ());
                Serial.print (F(", nSharedLockCount: "));
                Serial.println (mutex.GetSharedLockCount ());
                Serial.flush ();

            if (mutex.SharedLock (1000))
            {
                Serial.print ((size_t) this);
                Serial.print (F(": Read value: "));
                Serial.print (nValue);
                Serial.print (F(", Stack: "));
                Serial.println (GetStackSize ());

                Serial.flush ();
                
                Yield ();

                mutex.SharedUnlock ();
            }
        }

        Serial.print ((size_t) this);
        Serial.print (F(": Ending thread."));
        Serial.flush ();
    }

    virtual const char* GetName () final
    {
        return "Reader";
    }
};

class Writer : public atomicx::Thread
{
    private:
        volatile size_t nStack [40];

    public:

    Writer (atomicx_time nNice) : Thread (nNice, nStack)
    {
        // Serial.print ((size_t) this);
        // Serial.println (F(": Initiating."));
        // Serial.flush ();
    }

    virtual ~Writer () final
    {
        Serial.print ((size_t) this);
        Serial.println (F(": Deleting."));
        Serial.flush ();
    }

    virtual void run () final 
    {
        Serial.print ((size_t) this);
        Serial.print (F(": Starting up thread."));
        Serial.flush ();

        //SetPriority (255);

        while (true)
        {
            Serial.print (GetName ());
            Serial.print ((size_t) this);
            Serial.println (F(": Acquiring lock.. "));

            Serial.print (GetName ());
            Serial.print ((size_t) this);
            Serial.print (F(": bExclusiveLock: "));
            Serial.print (mutex.GetExclusiveLockStatus ());
            Serial.print (F(", nSharedLockCount: "));
            Serial.println (mutex.GetSharedLockCount ());
            Serial.flush ();

            if (mutex.Lock (10000))
            {
                PrintProcess (*this);
                
                nValue++;

                Serial.print (GetName ());
                Serial.print ((size_t) this);
                Serial.print (F(": >>>> Written value: "));
                Serial.print (nValue);
                Serial.print (F(", Stack: "));
                Serial.println (GetStackSize ());

                Serial.flush ();

                Yield ();
                
                mutex.Unlock ();
            }
        }

        Serial.print ((size_t) this);
        Serial.print (F(": Ending thread."));
        Serial.flush ();

        Yield ();
    }

    virtual const char* GetName () final
    {
        return "Writer";
    }
};


int g_nice = 100;

Writer w1(g_nice);
//Writer w2(g_nice);

Reader r1 (g_nice);
Reader r2 (g_nice);
// Reader r3 (g_nice);
// Reader r4 (g_nice);

void setup()
{
   
    Serial.begin (115200);

    Serial.println (F(""));
    Serial.println (F("Starting atomicx 3 demo."));
    Serial.flush ();
    delay(1000);

    PrintProcess (w1);

    atomicx::Thread::Join ();
}

void loop() {
}