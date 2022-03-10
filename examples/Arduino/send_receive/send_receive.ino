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

// signal
uint8_t dataPoint = 0; 

struct transf
{
    int Counter;
    char pszData[20];
};

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
        size_t nReceived;
        transf tr = {19, ""};

        for (;;)
        {
            Serial.print (GetStackSize ()); 
            Serial.print ("/");
            Serial.print (GetUsedStackSize ());
            Serial.print ("b stack size, ");
            Serial.print (GetLastUserExecTime());
            Serial.println (F("ms last exec time, Receiving data...")); Serial.flush ();

            if ((nReceived = Receive(dataPoint, (uint8_t*) &tr, (uint16_t) sizeof (tr), 10000)))
            {
                Serial.print (F("Received: val: "));
                Serial.print (tr.Counter);
                Serial.print (F(", String: "));
                Serial.println (tr.pszData);
                Serial.flush ();
            }
            else
            {
                Serial.println (F("Failed to receive information."));
                Serial.println (tr.Counter);
                Serial.flush ();
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
    size_t m_stack[65];
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
         transf tr {0, ""};
         int16_t nResponse;

        for(;;)
        {
            tr.Counter ++;
            snprintf(tr.pszData, sizeof (tr.pszData), "Counter: %u", tr.Counter);

            Serial.print (GetStackSize ()); 
            Serial.print ("/");
            Serial.print (GetUsedStackSize ());
            Serial.print ("b stack size, ");
            Serial.print (GetLastUserExecTime());
            Serial.print (F("ms last exectime, Sending data...:")); Serial.println (tr.pszData); Serial.flush ();
   
            if ((nResponse = Send (dataPoint, (uint8_t*) &tr, (uint16_t) sizeof (tr), 10000)) > 0)
            {
                Serial.print (F("Data Sent:")); Serial.println (nResponse); Serial.flush ();
            }
            else
            {
                Serial.print (F("Failed to send data..."));  Serial.println (nResponse); Serial.flush ();
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
        Serial.println (GetUsedStackSize());
        Serial.flush();
    }

private:
    size_t m_stack[65];
    String m_threadName;
};

void setup()
{
  Serial.begin (115200);

  while (! Serial) delay (100);

  delay (2000);

  Serial.println (F("Starting UP-----------------------------------------------\n"));

  Serial.flush();

  Producer T1(500);
  Consumer E1(500);
  
  atomicx::Start();

  Serial.println (F("Full lock detected..."));
}

void loop() {

}
