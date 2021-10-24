/*

*/
#include "arduino.h"

#include "atomic.hpp"

#include <stdio.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

using namespace thread;

size_t nDataCount=0;
atomic::lock gLock;

void ListAllThreads();

atomic_time Atomic_GetTick(void)
{    
    ::yield();
    return millis();
}

void Atomic_SleepTick(atomic_time nSleep)
{   
    ListAllThreads();
    delay(nSleep);
}

constexpr size_t GetStackSize(size_t sizeRef)
{
    return sizeRef * sizeof (size_t);  
}

class Consumer : public atomic
{
public:
    Consumer(uint32_t nNice, const char* pszName) :  atomic (m_stack), m_stack{}
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
        Serial.print("Deleting Consumer: ");
        Serial.print (", ID: ");
        Serial.println ((size_t) this); 
    }
    
    void run() noexcept override
    {
        size_t nCount=0;
        
        do
        {
            Serial.println ("SharedLock");
            gLock.SharedLock ();
            
            nCount = nDataCount;

            Yield();

            gLock.SharedUnlock();
            Serial.println ("SharedUlocking");
            
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
    uint8_t m_stack[200];
    String m_threadName="";
};


class Producer : public atomic
{
public:
    Producer(uint32_t nNice, const char* pszName) : atomic (m_stack), m_stack{}, m_threadName{pszName}
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
            Serial.println (nCount);
                                              
            Serial.println ("Lock");
            gLock.Lock();
            
            nDataCount++; 

            gLock.Unlock();
            Serial.println ("Unlock");
                       
            //atomic::smart_ptr<Consumer> Consumer_thread (new Consumer(100, "t::Consumer"));
                        
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

private:
    uint8_t m_stack[::GetStackSize(50)];
    String m_threadName;
};

void ListAllThreads()
{
  size_t nCount=0;

   Serial.flush();    

  Serial.println ("[THREAD]-----------------------------------------------");
  Serial.print ("IsLocked: ");
  Serial.print ((int) gLock.IsLocked());
  Serial.print (", IsShared: ");
  Serial.println ((int) gLock.IsShared());
  Serial.println ("---------------------------------------------------------");
  
  for (auto& th : *(atomic::GetCurrent()))
  {
      Serial.print (atomic::GetCurrent() == &th ? "*  " : "   ");
      Serial.print (++nCount);
      Serial.print (":'");
      Serial.print (th.GetName());
      Serial.print ("' ");
      Serial.print ((size_t) &th);
      Serial.print (", Nice: ");
      Serial.print (th.GetNice());
      Serial.print (", Stack: ");
      Serial.print (th.GetStackSize());
      Serial.print (", UsedStack: ");
      Serial.print(th.GetUsedStackSize());
      Serial.print (", Status: ");
      Serial.print (th.GetStatus());
      Serial.print (", Ref Lock: ");
      Serial.println (th.GetTagLock());
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
      
  Serial.println ("-----------------------------------------------\n");
  
  Serial.flush(); 

  
  Producer T1(100, u8"Producer 1");
  Consumer E1(100, u8"Consumer 1");
  Consumer E2(500, u8"Consumer 2");
  Consumer E4(300, u8"Consumer 3");

  ListAllThreads ();

  atomic::Start();

  Serial.println ("Full lock detected...");

   ListAllThreads ();
}

void loop() {
    
}
