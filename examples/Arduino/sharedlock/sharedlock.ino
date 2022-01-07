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
        Serial.print ("Deleting: ");
        Serial.print (GetName());
        Serial.print (", ID: ");
        Serial.println ((size_t) this); 
        started = false;
    }
    
    void run() noexcept override
    {
        int nCount=0;

        Serial.println ("Starting EVENTUAL.....");
        
        for (nCount=0; nCount < 10; nCount ++)
        {
            Serial.print ("\n\n[[[[[[[[[[[[");
            Serial.print (GetName ());
            Serial.print (":");
            Serial.print ((size_t) this);
            Serial.print (" - ");
            Serial.print (" Eventual counter ");
            Serial.print (nCount);
            Serial.println ("]]]]]]]]]]]\n\n");

            ListAllThreads();
            
            Yield ();
        }
    }

    void finish () noexcept override
    {
        Serial.println ("\n\n>>>>>>>DELETING EVENTUAL.... \n\n\n");        
        delete this;
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

    static bool GetStarted()
    {
        return started;
    }
    
private:
    uint8_t m_stack[::GetStackSize(30)];
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
            SmartLock sLock(gLock);
            sLock.SharedLock ();
            
            nCount = nDataCount;

            Yield();            
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

            Serial.println ("SharedUlocking");
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
    uint8_t m_stack[::GetStackSize(50)];
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
            Serial.println ("Lock");

            SmartLock sLock(gLock);
            sLock.Lock();

            if (Eventual::GetStarted() == false)
            {
                Serial.println (">>>> Creating EVENTUAL thread... <<<<");
                new Eventual (1000, "Eventual");
            }
            else
            {
                Serial.println (">>>> EVENTUAL IS TRUE... <<<<");
                Serial.println ();
            }

            Serial.print ("Executing ");
            Serial.print (GetName());
            Serial.print (": ");
            Serial.print (GetName ());
            Serial.print ((size_t) this);
            Serial.print (", locks/shared:");
            Serial.print (gLock.IsLocked());
            Serial.print (", ");
            Serial.print (sLock.IsLocked());
            Serial.print ("/");
            Serial.print (gLock.IsShared());
            Serial.print (", Counter: ");
            Serial.print (nCount);                                              
            Serial.print (", Eventual: ");
            Serial.println (Eventual::GetStarted());

            Serial.flush();
                        
            nDataCount++; 

            Serial.println ("Unlocking");
                       
            //atomicx::smart_ptr<Consumer> Consumer_thread (new Consumer(100, "t::Consumer"));
                        
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

  Serial.print ("Sizeof Producer:");
  Serial.println (sizeof (Producer));

  Serial.print ("Sizeof Consumer:");
  Serial.println (sizeof (Consumer));

  Serial.print ("Sizeof Eventual:");
  Serial.println (sizeof (Eventual));

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
      
  Serial.println ("Starting UP-----------------------------------------------\n");
  
  Serial.flush(); 

  
  Producer T1(100, u8"Producer 1");
  Consumer E1(100, u8"Consumer 1");
  Consumer E4(300, u8"Consumer 3");

  ListAllThreads ();

  atomicx::Start();

  Serial.println ("Full lock detected...");

   ListAllThreads ();
}

void loop() {
    
}
