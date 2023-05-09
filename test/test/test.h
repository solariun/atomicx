
#include "atomicx.hpp"

#include "test2.h"

extern void yield_in ();

extern uint8_t notify;

class th : public atomicx::Thread
{
    private:
        volatile size_t nStack [20];

    public:

    th () : Thread (0, nStack)
    {
        // Serial.print ((size_t) this);
        // Serial.println (F(": Initiating."));
        // Serial.flush ();
    }

    virtual ~th ()
    {
        Serial.print ((size_t) this);
        Serial.println (F(": Deleting."));
        Serial.flush ();
    }

    void yield ()
    {
        yield_in ();
    }

    virtual void run ()
    {
        int nValue = 0;
        tth* th =  nullptr;
        size_t nNotified;

        while (true)
        {
            nNotified = Notify (notify, 1, (size_t) this, 1, GetNice (), true);

            //Yield ();

            nValue++;

            Serial.print ((size_t) this);

            if (GetStatus() == atomicx::Status::timeout)
            {
                Serial.print (F(":TEST Timed out notifying"));
            }
            else
            {
                Serial.print (F(":TEST notified: "));
                Serial.print (nNotified);
                Serial.print ("      \r");
            }

            Serial.flush ();

            // Serial.print ((size_t) this);
            // Serial.print (F(":TEST Vall:"));
            // Serial.print (nValue);
            // Serial.print (F(", st#:"));
            // Serial.print (GetStackSize ());
            // Serial.print (F(", th#:"));
            // Serial.println (GetThreadCount ());
            // Serial.flush ();

            if (nValue && nValue % 1000 == 0) 
            {
                if (!th)
                {
                    th = new tth ();
                    delay(100);
                }
                else
                {
                    delete th;
                    th = nullptr,
                    delay(100);
                }
            }
        }

        Serial.print ((size_t) this);
        Serial.print (F(": Ending thread."));
        Serial.flush ();
    }

    virtual const char* GetName () final
    {
        return "Thread Test";
    }
};
