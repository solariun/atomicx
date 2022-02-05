///
/// @author   GUSTAVO CAMPOS
/// @author   GUSTAVO CAMPOS
/// @date     05/02/2020
/// @version  0.1
///
/// @copyright  (c) GUSTAVO CAMPOS, 2019
/// @copyright  Licence
///
/*
    MIT License

    Copyright (c) 2021 Gustavo Campos

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef __DOTMATRIX_H__
#define __DOTMATRIX_H__



#include <ESP8266WiFi.h>

// Designed for NodeMCU ESP8266
//################# DISPLAY CONNECTIONS ################
// LED Matrix Pin -> ESP8266 Pin
// Vcc            -> 3v  (3V on NodeMCU 3V3 on WEMOS)
// Gnd            -> Gnd (G on NodeMCU)
// DIN            -> D7  (Same Pin for WEMOS)
// CS             -> D4  (Same Pin for WEMOS)
// CLK            -> D5  (Same Pin for WEMOS)

#include "Arduino.h"
#include "user_interface.h"


#include "atomicx.hpp"
#include <assert.h>

#include <Wire.h>
#include <string>

#include "utils.hpp"
#include "TextScroller.hpp"

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

#ifndef STASSID
#define STASSID "WhiteKingdom2.4Ghz"
#define STAPSK "Creative01000"
#endif

// Utilities

// Functions

atomicx_time Atomicx_GetTick(void)
{
    ::yield();
    return millis();
}

void Atomicx_SleepTick(atomicx_time nSleep)
{
    delay(nSleep);
}

/*
  TERMINAL INTERFACE ---------------------------------------------------
*/

class Terminal : public thread::atomicx
{
public:
    Terminal(atomicx_time nNice) : atomicx(250, 50)
    {
        SetNice (nNice);
        SetDynamicNice (true);
    }

    char* GetName()
    {
        return "Terminal";
    }

protected:

    void run (void) noexcept override
    {
        for (;;)
        {
            Yield();

            Serial.flush ();
        }
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow();
    }


private:
    char szPixel[15] = "";
} ;

Terminal terminal(10);

/*
  SYSTEM CLASS ---------------------------------------------------
*/

class System : public thread::atomicx
{
public:
    System (atomicx_time nNice) : atomicx(250,50)
    {
        Serial.println (F("Starting up System..."));
        Serial.flush ();

        SetNice (nNice);
    }

    char* GetName()
    {
        return "System";
    }

protected:

    void run() noexcept override
    {
        atomicx_time last = GetCurrentTick () + 2000;
        uint8_t nScrollStage = 0;

        //Matrix.SetSpeed (2);

        for(;;)
        {
            Yield ();

            if (Wait (sysCmds, 1, 50))
            {
                if (sysCmds == SysCommands::Matrix_Ready)
                {
                    // strMatrixText.remove(0);

                    // switch (nScrollStage)
                    // {
                    //     case 0:
                    //         strMatrixText.concat (F("Free memory: "));
                    //         strMatrixText.concat (system_get_free_heap_size ());
                    //         strMatrixText.concat (F("bytes"));

                    //         break;

                    //     case 1:
                    //         strMatrixText.concat (F("AtomicX, powerful and revolutionary..."));

                    //         break;
                    //     default:
                    //         strMatrixText.concat (F("AtomicX"));
                    //         nScrollStage = ~0;
                    // }

                    nScrollStage++;
                }
            }

            ListAllThreads ();
        }
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow();
    }
};

void ListAllThreads()
{
    size_t nCount=0;

    Serial.flush();

    Serial.println (F("\e[K[THREAD]-----------------------------------------------"));

    Serial.print (F("\e[K>>> Free RAM: ")); Serial.println (system_get_free_heap_size ());
    Serial.print (F("\e[KAtomicX context size: ")); Serial.println (sizeof (thread::atomicx));
    Serial.print (F("\e[KSystem Context:")); Serial.println (sizeof (System));
    Serial.print (F("\e[KTerminal Context:")); Serial.println (sizeof (Terminal));

    Serial.println ("\e[K---------------------------------------------------------");

    for (auto& th : *(thread::atomicx::GetCurrent()))
    {
        Serial.print (F("\e[K"));
        Serial.print (thread::atomicx::GetCurrent() == &th ? F("*  ") : F("   "));
        Serial.print (++nCount);
        Serial.print (F(":'"));
        Serial.print (th.GetName());
        Serial.print (F("' "));
        Serial.print ((size_t) th.GetID());
        Serial.print (F("\t| Nice: "));
        Serial.print (th.GetNice());
        Serial.print (F("\t| Stack: "));
        Serial.print (th.GetStackSize());
        Serial.print ("\t| UsedStack: ");
        Serial.print (th.GetUsedStackSize());
        Serial.print (F("\t\t| Stat: "));
        Serial.print (th.GetStatus());
        Serial.print (F("\t| SStat: "));
        Serial.print (th.GetSubStatus());
        Serial.print (F("\t| LstExecTime: "));
        Serial.print (th.GetLastUserExecTime());
        Serial.println (F("ms"));
        Serial.flush(); Serial.flush();
    }

    Serial.println ("\e[K---------------------------------------------------------");
    Serial.flush(); Serial.flush();

}

void setup()
{
    util::InitSerial();

    vt100::ResetColor ();
    //vt100::ClearConsole ();
    vt100::HideCursor ();

    vt100::SetLocation (1,1);
    Serial.println (F(ATOMIC_VERSION_LABEL));


    
    TextScroller Matrix (5);
    Matrix.SetSpeed (2);
    
    System system (100);

    Serial.println (F("Thermal Camera Demo ------------------------------------"));
    Serial.flush ();

    thread::atomicx::Start();

    Serial.println (F("FULL DEADLOCK ------------------------------------"));
    Serial.flush ();

}

void loop() {
}

#endif // __DOTMATRIX_H__