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
#include "TerminalInterface.hpp"

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

#ifndef STASSID
#define STASSID "WhiteKingdom2.4Ghz"
#define STAPSK "Creative01000"
#endif

// Utilities
void yield()
{
    if (thread::atomicx::GetCurrent() != nullptr) 
    { 
        thread::atomicx::GetCurrent()->Yield (1);
    }
}

// Functions

atomicx_time Atomicx_GetTick(void)
{
    return millis();
}

void Atomicx_SleepTick(atomicx_time nSleep)
{
    delay(nSleep);
}

/*
  External Threads ---------------------------------------------------
*/

TextScroller Matrix (10);


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
            if (Wait (sysCmds, 1, GetNice ()))
            {
                if (sysCmds == SysCommands::Matrix_Ready)
                {
                    Matrix = "";

                    switch (nScrollStage)
                    {
                        case 0:
                            Matrix = Matrix() + "Free memory: " + std::to_string(system_get_free_heap_size()) + "bytes";

                            break;

                        case 1:
                            Matrix = Matrix() + "AtomicX, powerful and revolutionary...";

                            break;

                        default:
                            Matrix = Matrix() + "AtomicX, running threads: " + std::to_string(GetThreadCount ());
                            nScrollStage = -1;
                    }

                    nScrollStage++;
                }
            }

            //Serial.println ("System routine.");
        }
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow();
    }
};

void setup()
{
    util::InitSerial();

    vt100::ResetColor ();
    //vt100::ClearConsole ();
    vt100::HideCursor ();

    vt100::SetLocation (1,1);
    Serial.println (F(ATOMIC_VERSION_LABEL));
    
    Matrix.SetSpeed (3);
    
    TerminalInterface TerminalInterface(10, Serial);

    System system (100);

    Serial.println (F("Thermal Camera Demo ------------------------------------"));
    Serial.flush ();

    thread::atomicx::Start();

    Serial.println (F("FULL DEADLOCK ------------------------------------"));
    Serial.flush ();

}

void loop() 
{
}

#endif // __DOTMATRIX_H__