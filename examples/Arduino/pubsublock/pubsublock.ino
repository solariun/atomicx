/*

*/
#include "arduino.h"

#include "atomicx.hpp"

#include <stdio.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

using namespace thread;

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

size_t nDataCount=0;
atomicx::lock gLock;

String strTopic = "message/topic";

class Consumer : public atomicx
{
public:
    Consumer(uint32_t nNice, const char* pszName) : stack{}, atomicx (stack), pszName(pszName)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return pszName;
    }
    
    ~Consumer()
    {
        Serial.print("Deleting Consumer: ");
        Serial.print (", ID: ");
        Serial.println ((size_t) this);
    }

    bool IsSubscribed (const char* pszKey, size_t nKeyLenght) override
    {
        Serial.print (__FUNCTION__);
        Serial.print (":");
        Serial.print (pszKey);
        
        if (strncmp (pszKey, strTopic.c_str(), nKeyLenght) == 0)
        {
           Serial.println (":Has Subscriptions.....");
        }
        else
        {
          Serial.println (":Has NO Subscriptions.....");
        }

        Serial.flush();
        
        return true;
    }

    bool BrokerHandler(const char* pszKey, size_t nKeyLenght, Message& message)
    {
        Serial.print (GetName());
        Serial.print (":");
        Serial.print (pszKey);
        Serial.print (":");
        Serial.print (message.tag);
        Serial.print (":");
        Serial.print (message.message);
        Serial.println (": async Handling topic ....");
        Serial.flush();
        
        return true;  
    }
    
    void run() noexcept override
    {
        Message message={0,0};
        
        do
        {            
            WaitBrokerMessage (strTopic.c_str(), strTopic.length(), message);
    
            // --------------------------------------
            
            Serial.print ("Executing Consumer ");
            Serial.print (GetName());
            Serial.print (": ");
            Serial.print ((size_t) this);
            Serial.print (": Stack used: ");
            Serial.print (GetUsedStackSize());
            Serial.print (", locks/shared:");
            Serial.print (gLock.IsLocked());
            Serial.print ("/");
            Serial.print (gLock.IsShared());
            Serial.print (", nTag: ");
            Serial.print (message.tag);
            Serial.print (", Message: ");
            Serial.println (message.message);
            
            Serial.flush();
            
        } while (Yield());
    }

    void StackOverflowHandler(void) noexcept final
    {
        Serial.print (__FUNCTION__);
        Serial.print ("[");
        Serial.print (GetName ());
        Serial.print ((size_t) this);
        Serial.print (": Stack used ");
        Serial.print (GetUsedStackSize());
        Serial.print ("/");
        Serial.println (GetStackSize());
        Serial.flush();
    }

private:
    uint8_t stack[sizeof(size_t) * 50];
    const char* pszName;
};


class Producer : public atomicx
{
public:
    Producer(uint32_t nNice, const char* pszName) : stack{}, atomicx (stack), pszName(pszName)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return pszName;
    }
    
    ~Producer()
    {
        Serial.print("Deleting ");
        Serial.println ((size_t) this);
    }
    
    void run() noexcept override
    {
        size_t nCount=0;
        size_t nTag = GetID ();
        
        do
        {
            Serial.print ("Executing ");
            Serial.print (GetName());
            Serial.print (": ");
            Serial.print (GetName ());
            Serial.print ((size_t) this);
            Serial.print (", locks/shared:");
            Serial.print (gLock.IsLocked());
            Serial.print ("/");
            Serial.print (gLock.IsShared());

            Serial.print (", Counter: ");
            Serial.println (nCount++);
            
            Serial.flush();
                        
            Publish (strTopic.c_str(), strTopic.length(), {nTag, nCount});
                        
            //atomicx::smart_ptr<Consumer> Consumer_thread (new Consumer(100, "t::Consumer"));
                        
        } while (Yield ());
    }

    void StackOverflowHandler(void) noexcept final
    {
        Serial.print (__FUNCTION__);
        Serial.print ("[");
        Serial.print (GetName ());
        Serial.print ((size_t) this);
        Serial.print (": Stack used ");
        Serial.println (GetUsedStackSize());
        Serial.flush();
    }

   void ListAllThreads()
   {
      size_t nCount=0;

      Serial.print ("Context: "); 
      Serial.print (sizeof (atomicx));
      Serial.println ("---------------------------------");
      
      for (auto& th : *this)
      {
          Serial.print (++nCount);
          Serial.print (":'");
          Serial.print (th.GetName());
          Serial.print ("' ");
          Serial.print ((size_t) &th);
          Serial.print (", Nice: ");
          Serial.print (th.GetNice());
          Serial.print ("\t, Stack: ");
          Serial.print (th.GetStackSize());
          Serial.print ("\t, UsedStack: ");
          Serial.print(th.GetUsedStackSize());
          Serial.print ("\t, Status: ");
          Serial.println (th.GetStatus());

          Serial.flush();
      }
  }

private:
    uint8_t stack[sizeof(size_t) * 50];
    const char* pszName;
};

void setup() 
{
  Serial.begin (115200);

  while (! Serial) delay (100);

  delay (2000);
      
  Serial.println ("-----------------------------------------------\n");
  Serial.println ("Starting up...");
  Serial.println ("-----------------------------------------------\n");
  Serial.flush(); 

  
  Producer T1(100, "Thread 1");
  Consumer E1(100, "Consumer 1");
  Consumer E2(250, "Consumer 2");
  Consumer E4(500, "Consumer 3");

  T1.ListAllThreads ();

  atomicx::Start();

  Serial.println ("Full lock detected...");

  T1.ListAllThreads ();
}

void loop() {
    
}
