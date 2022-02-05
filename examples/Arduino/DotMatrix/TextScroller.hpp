
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

#ifndef __TEXTSCROLLER_HPP__
#define __TEXTSCROLLER_HPP__

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

// const data

static const uint64_t byteImages[] PROGMEM = {
        0x0000000000000000, 0x00180018183c3c18, 0x0000000012246c6c, 0x0036367f367f3636, 0x000c1f301e033e0c, 0x0063660c18336300, 0x006e333b6e1c361c,
        0x0000000000030606, 0x00180c0606060c18, 0x00060c1818180c06, 0x0000663cff3c6600, 0x00000c0c3f0c0c00, 0x060c0c0000000000, 0x000000003f000000,
        0x000c0c0000000000, 0x000103060c183060, 0x003e676f7b73633e, 0x003f0c0c0c0c0e0c, 0x003f33061c30331e, 0x001e33301c30331e, 0x0078307f33363c38,
        0x001e3330301f033f, 0x001e33331f03061c, 0x000c0c0c1830333f, 0x001e33331e33331e, 0x000e18303e33331e, 0x000c0c00000c0c00, 0x060c0c00000c0c00,
        0x00180c0603060c18, 0x00003f00003f0000, 0x00060c1830180c06, 0x000c000c1830331e, 0x001e037b7b7b633e, 0x6666667e66663c00, 0x3e66663e66663e00,
        0x3c66060606663c00, 0x3e66666666663e00, 0x7e06063e06067e00, 0x0606063e06067e00, 0x3c66760606663c00, 0x6666667e66666600, 0x3c18181818183c00,
        0x1c36363030307800, 0x66361e0e1e366600, 0x7e06060606060600, 0xc6c6c6d6feeec600, 0xc6c6e6f6decec600, 0x3c66666666663c00, 0x06063e6666663e00,
        0x603c766666663c00, 0x66361e3e66663e00, 0x3c66603c06663c00, 0x18181818185a7e00, 0x7c66666666666600, 0x183c666666666600, 0xc6eefed6c6c6c600,
        0xc6c66c386cc6c600, 0x1818183c66666600, 0x7e060c1830607e00, 0x001e06060606061e, 0x00406030180c0603, 0x001e18181818181e, 0x0000000063361c08,
        0x003f000000000000, 0x0000000000180c06, 0x7c667c603c000000, 0x3e66663e06060600, 0x3c6606663c000000, 0x7c66667c60606000, 0x3c067e663c000000,
        0x0c0c3e0c0c6c3800, 0x3c607c66667c0000, 0x6666663e06060600, 0x3c18181800180000, 0x1c36363030003000, 0x66361e3666060600, 0x1818181818181800,
        0xd6d6feeec6000000, 0x6666667e3e000000, 0x3c6666663c000000, 0x06063e66663e0000, 0xf0b03c36363c0000, 0x060666663e000000, 0x3e603c067c000000,
        0x1818187e18180000, 0x7c66666666000000, 0x183c666600000000, 0x7cd6d6d6c6000000, 0x663c183c66000000, 0x3c607c6666000000, 0x3c0c18303c000000,
        0x00380c0c070c0c38, 0x0c0c0c0c0c0c0c0c, 0x00070c0c380c0c07, 0x0000000000003b6e, 0x5554545050404000, 0x3f21212121212121, 0x3f212d2121212121,
        0x3f212d212d212121, 0x3f212d212d212d21, 0x3f212d2d2d212121, 0x3f212d2d2d2d2d2d, 0x00040a1120408000, 0x081c3e7f1c1c1c1c, 0x0010307fff7f3010,
        0x1c1c1c1c7f3e1c08, 0x00080cfefffe0c08, 0x40e040181e0f0f07, 0x939398cc6730180f, 0xffa9a9b7d9eff1ff, 0x1800ff7a3a3a3c18, 0x3c428199b985423c,
        0x18423ca5a53c4218, 0x0000ff81864c3800, 0x1428ff81864c3800, 0x0000ff81864f3e05, 0x0000ff81864f3e05, 0x18141010fe7c3800, 0x08081c1c3e363e1c,
        0x00aa000000000000, 0x000a080000000000, 0x002a282000000000, 0x00aaa8a0840a1000, 0x3c46e7e7e3ff663c, 0x0082442810284482, 0x003844aa92aa4438,
        0x003864f2ba9e4c38, 0xff8199a5c3422418, 0x7e3cdbc383343624, 0xff8199a5c3ff0000, 0x3c66c39999db5a18, 0xff000001010000ff, 0xff000003030000ff,
        0xff000006060000ff, 0xff00000c0c0000ff, 0xff000018180000ff, 0xff000030300000ff, 0xff000060600000ff, 0xff0000c0c00000ff, 0xff000080800000ff};

const int byteImagesLen = sizeof (byteImages) / 8;

/*
  MATRIX TEXT SCROLL ---------------------------------------------------
*/

static const uint8_t MAX_LED_MATRIX=4;

static const int DIN = D4;  // MISO - NodeMCU - D4 (TXD1)
static const int CS = D7;   // MOSI  - NodeMCU - D7 (HMOSI)
static const int CLK = D5;  // SS    - NodeMCU - D5 (HSCLK)

class TextScroller : public thread::atomicx
{
public:

    TextScroller (atomicx_time nNice);

    TextScroller () = delete;

    void SetSpeed(int nSpeed);

    bool show (const char* pszMessage, const uint16_t nMessageLen);

protected:

    void printRow (uint16_t nDigit, uint8_t nRowIndex, uint8_t nRow);

    void run(void) noexcept override;

    void StackOverflowHandler(void) noexcept final;

    char* GetName (void);

    bool begin();

private:

    uint64_t getLetter (int nIndex, const char* pszMessage, uint16_t nMessageLen);

    uint64_t getImage (int nIndex);

    void printScrollBytes (uint16_t nDigit, const uint64_t charLeft, const uint64_t charRight, uint8_t nOffset);

    uint8_t m_stack [250]={};
    
    int nNumberDigits = MAX_LED_MATRIX;
    uint8_t nOffset   = 0;
    int nIndex = nNumberDigits * (-1);

    uint8_t nSpeed;
    
    std::string strMatrixText="";

    LedControl lc;
};

#endif // __TEXTSCROLLER_H__