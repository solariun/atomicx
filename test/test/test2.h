
#include "atomicx.hpp"

extern uint8_t notify;

class tth : atomicx::Thread
{
    private:
        volatile size_t nStack [20];

    public:

    tth () : Thread (200, nStack)
    {
        // Serial.print ((size_t) this);
        // Serial.println (F(": Initiating."));
        // Serial.flush ();
    }

    virtual ~tth ()
    {
        Serial.print ((size_t) this);
        Serial.print (F(": TTH Deleting."));
        Serial.print (F(", th#:"));
        Serial.println (GetThreadCount ());
        Serial.flush ();
        
    }

    virtual void run ()
    {
        int nValue = 0;

        size_t message;

        while (true)
        {
            Serial.print ((size_t) this);
            Serial.print (F(": TTH Vall:"));
            Serial.print (nValue++);
            Serial.print (F(", stk:"));
            Serial.println (GetStackSize ());
            Serial.flush ();

            if (Wait (notify, 1, message, 1, 500) == false)
            {
                Serial.print ((size_t) this);
                Serial.println (F(": TTH ERROR NOT NOTIFIED."));
            }
            else 
            {
                Serial.print ((size_t) this);
                Serial.print (F(": TTH NOTIFIED..."));
                Serial.println (message);
            }

            Serial.println (F(""));
            Serial.flush ();

           Yield (0);
        }

        Serial.print ((size_t) this);
        Serial.print (F(": Ending thread."));
        Serial.flush ();
    }

    virtual const char* GetName () final
    {
        return "TTh ";
    }
};

