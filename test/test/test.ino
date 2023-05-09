
#include "atomicx.hpp"

#include "test.h"

uint8_t notify = 0;

atomicx_time atomicx::Thread::GetTick(void)
{
    return millis();
}

void atomicx::Thread::SleepTick(atomicx_time nSleep)
{
    delay(nSleep);
}

void yield_in ()
{
    atomicx::Thread::Yield ();
}

th th1;
th th2;
th th3;
th th4;

void setup()
{
   
    Serial.begin (115200);

    Serial.println (F(""));
    Serial.println (F("Starting atomicx 3 demo."));
    Serial.flush ();
    delay(1000);

    Serial.println ("-------------------------------------");

    for (auto& th : th1)
    {
        Serial.print (__func__);
        Serial.print (": Listing thread: ");
        Serial.print (th.GetName ());
        Serial.print (", ID:");
        Serial.println ((size_t) &th);
        Serial.flush ();
    }

    Serial.println ("-------------------------------------");

    atomicx::Thread::Join ();
}

void loop() {
}