/*

*/
#include "arduino.h"

#include "atomicx.hpp"

#include <stdio.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

using namespace thread;

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)      /* set alignment to 1 byte boundary */
struct TalkAFrame
{
    uint16_t startFrame;
    uint8_t  majorVersion;
    uint8_t  minorVersion;
    uint8_t  messageType : 4;
    uint8_t  messageFlags : 4;
    uint8_t  multPartCounter;
    uint8_t  dataLen;
    uint32_t topicID;
    uint64_t data;
    uint16_t crc;
};
#pragma pack(pop)   /* restore original alignment from stack */

atomicx_time atomicx_GetTick(void)
{    
    ::yield();
    return millis();
}

void atomicx_SleepTick(atomicx_time nSleep)
{    
    delay(nSleep);
}

uint8_t nLockVar = 0;

atomicx::queue<size_t> q(5);

class Eventual : public atomicx
{
public:
    Eventual(uint32_t nNice, const char* pszName) : stack{}, atomicx (stack), pszName(pszName)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return pszName;
    }
    
    ~Eventual()
    {
        Serial.print("Deleting Eventual: ");
        Serial.print (", ID: ");
        Serial.println ((size_t) this);
    }
    
    void run() noexcept override
    {
        size_t nCount=0;
        
        do
        {
            nCount = q.Pop();
            
            Serial.print ("Executing Eventual ");
            Serial.print (GetName());
            Serial.print (": ");
            Serial.print ((size_t) this);
            Serial.print (": Stack used: ");
            Serial.print (GetUsedStackSize());
            Serial.print (", Counter: ");
            Serial.println (nCount);
            Serial.flush();
            
        } while (Yield());
    }

    void StackOverflowHandler(void) final
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


class Executor : public atomicx
{
public:
    Executor(uint32_t nNice, const char* pszName) : stack{}, atomicx (stack), pszName(pszName)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return pszName;
    }
    
    ~Executor()
    {
        Serial.print("Deleting ");
        Serial.println ((size_t) this);
    }
    
    void run() noexcept override
    {
        size_t nCount=0;
        
        do
        {
            Serial.print ("Executing ");
            Serial.print (GetName());
            Serial.print (": ");
            Serial.print (GetName ());
            Serial.print ((size_t) this);
            Serial.print (", Counter: ");
            Serial.println (nCount);
            //Serial.print (", Memory usaged:");
            //Serial.println (ESP.getFreeHeap());
            Serial.flush();
            
            nCount++;
            
            //atomicx::smart_ptr<Eventual> eventual_thread (new Eventual(100, "t::eventual"));

            q.PushBack (nCount);
            
        } while (Yield ());
    }

    void StackOverflowHandler(void) final
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

      for (auto& th : *this)
      {
          Serial.print (++nCount);
          Serial.print (":");
          Serial.print (" ");
          Serial.print ((size_t) &th);
          Serial.print (", Nice: ");
          Serial.print (th.GetNice());
          Serial.print ("\t, Stack: ");
          Serial.print (th.GetStackSize());
          Serial.print ("\t, UsedStack: ");
          Serial.println (th.GetUsedStackSize());
          Serial.print ("\t, Status: ");
          Serial.print (th.GetStatus());

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
    
  q.PushBack(1);
  q.PushBack(2);
  q.PushBack(3);
  q.PushBack(4);
  q.PushBack(5);
  q.PushBack(6);
  q.PushBack(7);
  q.PushBack(8);

  q.PushFront(-1);
  q.PushFront(-2);
  q.PushFront(-3);
  q.PushFront(-4);
  q.PushFront(-5);
  q.PushFront(-6);
    
  Serial.println ("-----------------------------------------------\n");
  
  Serial.flush(); 

  uint8_t start = 0;
  
  Executor T1(10, "Thread 1");
  Eventual E1(100, "consumer 1");
  Eventual E2(100, "consumer 2");
  //Eventual E4(300, "consumer 3");

  uint8_t stop=0;
  

  Serial.print ("Values: ");
  Serial.println (sizeof (E1));
  
  T1.ListAllThreads ();

  atomicx::Start();

  Serial.println ("Full lock detected...");

  T1.ListAllThreads ();
}

void loop() {
    
}
