
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

#include <ESP8266WiFi.h>

// Designed for NodeMCU ESP8266
//################# DISPLAY CONNECTIONS ################
// LED Matrix Pin -> ESP8266 Pin
// Vcc            -> 3v  (3V on NodeMCU 3V3 on WEMOS)
// Gnd            -> Gnd (G on NodeMCU)
// DIN            -> D7  (Same Pin for WEMOS)
// CS             -> D4  (Same Pin for WEMOS)
// CLK            -> D5  (Same Pin for WEMOS)

#include "atomicx.hpp"
#include "Arduino.h"
#include "user_interface.h"
#include <LedControl.h>
#include <assert.h>
#include <string>

#include "utils.hpp"
#include "TextScroller.hpp"

uint64_t TextScroller::getLetter (int nIndex, const char* pszMessage, uint16_t nMessageLen)
{
    int nCharacter = nIndex > nMessageLen || nIndex < 0 ? ' ' : pszMessage[nIndex];

    return getImage (nCharacter - ' ');
}

uint64_t TextScroller::getImage (int nIndex)
{
    uint64_t nBuffer = 0xAA;

    nIndex = nIndex > byteImagesLen || nIndex < 0 ? 0 : nIndex;

    memcpy_P (&nBuffer, byteImages + nIndex, sizeof (uint64_t));

    return nBuffer;
}

void TextScroller::printScrollBytes (uint16_t nDigit, const uint64_t charLeft, const uint64_t charRight, uint8_t nOffset)
{
    int i = 0;

    for (i = 0; i < 8; i++)
    {
        printRow (nDigit, i, (((uint8_t*)&charLeft)[i] << (8 - nOffset) | ((uint8_t*)&charRight)[i] >> nOffset));
    }
}


void TextScroller::printRow (uint16_t nDigit, uint8_t nRowIndex, uint8_t nRow)
{
    lc.setRow (nDigit, 7 - nRowIndex, nRow);
}

void TextScroller::run(void)
{
     begin();

    for (;;)
    {
        Yield ();

        if (show (strMatrixText.c_str(), strMatrixText.length()) == false)
        {
            sysCmds = SysCommands::Matrix_Ready;
            
            if (! SyncNotify (sysCmds, 1, 1000))
            {
                Serial.printf (">>> ERROR, failed to notify System... (%zp)\r\n", &sysCmds);
                delay (2000);
            }
        }
    }

    return;
}

void TextScroller::StackOverflowHandler(void)
{
    PrintStackOverflow ();
}

char* TextScroller::GetName (void)
{
    return "Matrix";
}

bool TextScroller::begin()
{
    // Intentionally delaying to give time to the SPI to start
    delay (1000);

    uint8_t nCounter=nNumberDigits;

    if (nCounter) do
    {            
        lc.shutdown (nCounter, false);  // The MAX72XX is in power-saving mode on startup
        lc.setIntensity (nCounter, 1);  // Set the brightness to maximum value
        lc.clearDisplay (nCounter);     // and clear the display

    } while (nCounter--);

    return true;
}

TextScroller::TextScroller (atomicx_time nNice) : atomicx(250, 50), nOffset (0), nSpeed (2),  lc(LedControl (DIN, CLK, CS, MAX_LED_MATRIX))
{
    nNumberDigits = MAX_LED_MATRIX;
    nIndex = (nSpeed == 0) ? nNumberDigits * (-1) : 0;

    SetNice (nNice);

    strMatrixText = "Welcome to AtomicX DotMatrix WiFi server.";
}

void TextScroller::SetSpeed(int nSpeed)
{
    this->nSpeed = nSpeed;
}

bool TextScroller::show (const char* pszMessage, const uint16_t nMessageLen)
{
    uint8_t nCount;

    if (nSpeed > 0 && nSpeed % 8 == 0) nSpeed++;

    if (nSpeed > 0) do
        {
            if (nOffset >= 7)
            {
                nIndex  = nIndex + 1 > (int)nMessageLen ? (int)nNumberDigits * (-1) : nIndex + 1;
                nOffset = 0;

                if ((int)nNumberDigits * (-1) == nIndex) return false;
            }
            else
            {
                nOffset = (int)nOffset + (nSpeed % 8);
            }
        } while (nOffset >= 8);


    for (nCount = 0; nCount < nNumberDigits; nCount++)
    {
        Yield ();

        printScrollBytes (
                nCount,
                getLetter (nIndex + 1, pszMessage, nMessageLen),
                getLetter (nIndex, pszMessage, nMessageLen),
                (uint8_t)nOffset);

        nIndex = (int)nIndex + 1;
    }

    nIndex = (int)nIndex - (nCount - (nSpeed / 8));

    return true;
}

std::string& TextScroller::operator= (const std::string& strValue)
{
    strMatrixText = strValue;

    return strMatrixText;
}

std::string& TextScroller::operator() ()
{
    return strMatrixText;
}