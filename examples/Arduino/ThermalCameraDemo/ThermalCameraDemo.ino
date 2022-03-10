/*
    Thermal Camera Demo for AtomicX

    Before starting, please install

    arduino-cli lib install "Adafruit AMG88xx Library"
    arduino-cli lib install "LedControl"
*/

#include "atomicx.hpp"

#include "Arduino.h"

#include <assert.h>

#include <Adafruit_AMG88xx.h>
#include <LedControl.h>
#include <Wire.h>

const uint64_t byteImages[] PROGMEM = {
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
        0x00380c0c070c0c38, 0x0c0c0c0c0c0c0c0c, 0x00070c0c380c0c07, 0x0000000000003b6e};

const int byteImagesLen = sizeof (byteImages) / 8;

#define PrintStackOverflow() \
        Serial.print (__FUNCTION__); \
        Serial.print ("["); \
        Serial.print (GetName ()); \
        Serial.print (F("/")); \
        Serial.print ((size_t) this); \
        Serial.print ("]:  StackSize: ["); \
        Serial.print (GetStackSize ()); \
        Serial.print ("]. Stack used ["); \
        Serial.print (GetUsedStackSize()); \
        Serial.println (F("]")); \
        Serial.flush(); \


atomicx_time Atomicx_GetTick(void)
{
    return millis();
}

void Atomicx_SleepTick(atomicx_time nSleep)
{
    delay(nSleep);
}

extern int __heap_start, *__brkval;
#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

namespace util
{
    void InitSerial(void)
    {
        Serial.begin (230400);

        if (! Serial)
        {

            // Wait serial to proper start.
            while (! Serial);
        }
    }

    size_t GetFreeRam ()
    {
        int v;
        return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
    }

    constexpr size_t GetStackSize(size_t sizeRef)
    {
        return sizeRef * sizeof (size_t);
    }
}

namespace vt100
{
    void SetLocation (uint16_t nY, uint16_t nX)
    {
        Serial.print (F("\033["));
        Serial.print (nY);
        Serial.print (F(";"));
        Serial.print (nX);
        Serial.print (F("H"));
        Serial.flush ();
    }

    // works with 256 colors
    void SetColor (const uint8_t nFgColor, const uint8_t nBgColor)
    {
        Serial.print (F("\033["));
        Serial.print (nFgColor + 30);
        Serial.print (F(";"));
        Serial.print (nBgColor + 40);
        Serial.print (F("m"));
        Serial.flush ();
    }

    void ResetColor ()
    {
        Serial.print (F("\033[0m")); Serial.flush ();
    }

    void HideCursor ()
    {
        Serial.print (F("\033[?25l")); Serial.flush ();
    }

    void ShowCursor ()
    {
        Serial.print (F("\033[?25h")); Serial.flush ();
    }

    void ClearConsole ()
    {
        Serial.print (F("\033[2J")); Serial.flush ();
    }

    void ReverseColor ()
    {
        Serial.print (F("\033[7m")); Serial.flush ();
    }
}

enum class TermCommands
{
    ThermalCam_Update,
    ThreadList_Update
} termCmds;

enum class SysCommands
{
    Matrix_Ready=10
} sysCmds;

/*
  MATRIX TEXT SCROLL ---------------------------------------------------
*/

String strMatrixText;

#define MAX_LED_MATRIX 4

#define DIN 12  // MISO
#define CS  11  // MOSI
#define CLK 10  // SS

class TextScroller : public thread::atomicx
{
private:
    int nNumberDigits = 15;
    uint8_t nOffset   = 0;
    int nIndex        = nNumberDigits * (-1);

    uint8_t nSpeed;

    LedControl lc = LedControl (DIN, CLK, CS, MAX_LED_MATRIX);

    uint64_t getLetter (int nIndex, const char* pszMessage, uint16_t nMessageLen)
    {
        int nCharacter = nIndex > nMessageLen || nIndex < 0 ? ' ' : pszMessage[nIndex];

        return getImage (nCharacter - ' ');
    }

    uint64_t getImage (int nIndex)
    {
        uint64_t nBuffer = 0xAA;

        nIndex = nIndex > byteImagesLen || nIndex < 0 ? 0 : nIndex;

        memcpy_P (&nBuffer, byteImages + nIndex, sizeof (uint64_t));

        return nBuffer;
    }

    void printScrollBytes (uint16_t nDigit, const uint64_t charLeft, const uint64_t charRight, uint8_t nOffset)
    {
        int i = 0;

        for (i = 0; i < 8; i++)
        {
            printRow (nDigit, i, (((uint8_t*)&charLeft)[i] << (8 - nOffset) | ((uint8_t*)&charRight)[i] >> nOffset));
        }
    }

protected:

    void printRow (uint16_t nDigit, uint8_t nRowIndex, uint8_t nRow)
    {
        lc.setRow (nDigit, 7 - nRowIndex, nRow);
    }

    void run(void) noexcept override
    {
        begin();

        for(;;)
        {
            if (! show (strMatrixText.c_str(), strMatrixText.length()))
            {
                sysCmds = SysCommands::Matrix_Ready;
                SyncNotify (sysCmds, 1, 1000);
            }
        }
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow ();
    }

    char* GetName (void)
    {
        return "Matrix";
    }

public:
    TextScroller (atomicx_time nNice) : atomicx(m_stack), nNumberDigits (MAX_LED_MATRIX), nOffset (0), nSpeed (2)
    {
        nIndex = (nSpeed == 0) ? nNumberDigits * (-1) : 0;

        SetNice (nNice);

        SetDynamicNice(true);

        strMatrixText.concat (F("Welcome to AtomicX Thermal Camera demo."));
    }

    TextScroller () = delete;

    void SetSpeed(int nSpeed)
    {
        this->nSpeed = nSpeed;
    }

    bool begin()
    {
        for (int nCount = 0; nCount < nNumberDigits; nCount++)
        {
            lc.shutdown (nCount, false);  // The MAX72XX is in power-saving mode on startup
            lc.setIntensity (nCount, 2);  // Set the brightness to maximum value
            lc.clearDisplay (nCount);     // and clear the display
        }
    }

    bool show (const char* pszMessage, const uint16_t nMessageLen)
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
            printScrollBytes (
                    nCount,
                    getLetter (nIndex + 1, pszMessage, nMessageLen),
                    getLetter (nIndex, pszMessage, nMessageLen),
                    (uint8_t)nOffset);

            nIndex = (int)nIndex + 1;

            Yield (0);
        }

        nIndex = (int)nIndex - (nCount - (nSpeed / 8));

        return true;
    }
private:
    size_t m_stack [35]={};

} Matrix (10);

/*
  THERMAL CAM ---------------------------------------------------
*/

// Global Array for the Thermal Camera
float pixels[AMG88xx_PIXEL_ARRAY_SIZE]={};

class ThermalCam : public thread::atomicx
{
public:
    ThermalCam (atomicx_time nNice) : atomicx()
    {
        SetNice (nNice);
    }

    ~ThermalCam ()
    {

    }

    float GetMaxTemperature ()
    {
        return fMax;
    }

    float GetMinTemperature ()
    {
        return fMin;
    }

    bool Status()
    {
        return isStarted;
    }

    char* GetName()
    {
        return "ThermalCam";
    }

protected:
    bool begin()
    {
        // Starting the AMG88
        // default settings
        isStarted = amg.begin ();

        if (!isStarted)
        {
            abort ();
        }
    }

    void run() noexcept override
    {
        begin();

        for (;;)
        {
            Yield ();

            // read all the pixels
            amg.readPixels (pixels);

            fMin = 255; fMax = 0;

            for (int i = AMG88xx_PIXEL_ARRAY_SIZE; i > 0; i--)
            {
                fMin = MIN (fMin, pixels[i - 1]);
                fMax = MAX (fMax, pixels[i - 1]);
            }

            termCmds = TermCommands::ThermalCam_Update;

            SyncNotify (termCmds, 1, 1000);

        }
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow ();
    }

private:
    Adafruit_AMG88xx amg;

    bool isStarted = false;

    float fMin=0, fMax=~0;
} thermal(1);

/*
  TERMINAL INTERFACE ---------------------------------------------------
*/

class Terminal : public thread::atomicx
{
public:
    Terminal(atomicx_time nNice) : atomicx()
    {
        SetNice (nNice);
        SetDynamicNice (true);
    }

    char* GetName()
    {
        return "Terminal";
    }

protected:
    void PrintThermalImage (float& fMin, float& fMax)
    {
        vt100::SetLocation(12, 1); Serial.print (F("\e[K"));

        for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++)
        {
            snprintf (szPixel, sizeof (szPixel)-1, "\033[48;5;%um  ", map (pixels[i], fMin, fMax, 232, 255));
            Serial.print (szPixel); Serial.flush ();

            if ((i + 1) % 8 == 0)
            {
                vt100::ResetColor ();
                Serial.flush ();
                Yield (0);
                vt100::SetLocation (12-((i) / 8), 1);
                Serial.print (F("\e[K"));
                Serial.flush ();
            }
        }

        vt100::ResetColor ();
        Serial.flush ();
    }

    void run (void) noexcept override
    {
        for (;;)
        {
            Yield();

            if (Wait (termCmds, 1, 100))
            {
                vt100::ResetColor(); Serial.flush ();

                if (termCmds == TermCommands::ThermalCam_Update)
                {
                    float fMin = thermal.GetMinTemperature ();
                    float fMax = thermal.GetMaxTemperature ();

                    PrintThermalImage (fMin, fMax);

                    vt100::SetLocation (6,22);
                    Serial.print (F("Temperature: Min:"));
                    Serial.print (fMin);
                    Serial.print (F("c, Max:"));
                    Serial.print (fMax);
                    Serial.print (F("c"));
                    Serial.flush ();
                    Serial.flush ();
                }
                else if (termCmds == TermCommands::ThreadList_Update)
                {
                    vt100::SetLocation(14,1);
                    ListAllThreads ();
                }
            }

            Serial.flush ();
            Serial.flush ();
        }
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow();
    }


private:
    char szPixel[15] = "";
} terminal(10);

/*
  SYSTEM CLASS ---------------------------------------------------
*/

class System : public thread::atomicx
{
public:
    System (atomicx_time nNice) : atomicx()
    {
        Serial.println (F("Starting up System..."));
        Serial.flush ();

        SetNice (nNice);
        SetDynamicNice (true);
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

        Matrix.SetSpeed (2);

        for(;;)
        {
            Yield ();

            if (Wait (sysCmds, 1, 50))
            {
                if (sysCmds == SysCommands::Matrix_Ready)
                {
                    strMatrixText.remove(0);

                    switch (nScrollStage)
                    {
                        case 0:
                            strMatrixText.concat (F("Temp: Min:"));
                            strMatrixText.concat (thermal.GetMinTemperature ());
                            strMatrixText.concat (F("c, Max:"));
                            strMatrixText.concat (thermal.GetMaxTemperature ());
                            strMatrixText.concat (F("c"));

                            break;
                        case 1:
                            strMatrixText.concat (F("Free memory: "));
                            strMatrixText.concat (util::GetFreeRam ());
                            strMatrixText.concat (F("bytes"));

                            break;

                        case 2:
                            strMatrixText.concat (F("AtomicX, powerful and revolutionary..."));

                            break;
                        default:
                            strMatrixText.concat (F("AtomicX"));
                            nScrollStage = ~0;
                    }

                    nScrollStage++;
                }
            }

            if ((GetCurrentTick () - last) > 1000)
            {
                termCmds = TermCommands::ThreadList_Update;

                SyncNotify (termCmds, 1, 100);

                last = GetCurrentTick ();
            }
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

    Serial.print (F("\e[K>>> Free RAM: ")); Serial.println (util::GetFreeRam());
    Serial.print (F("\e[KAtomicX context size: ")); Serial.println (sizeof (thread::atomicx));
    Serial.print (F("\e[KSystem Context:")); Serial.println (sizeof (System));
    Serial.print (F("\e[KThermal Context:")); Serial.println (sizeof (ThermalCam));
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
    vt100::ClearConsole ();
    vt100::HideCursor ();

    vt100::SetLocation (1,1);
    Serial.println (F(ATOMIC_VERSION_LABEL));

    System system (100);

    Serial.println (F("Thermal Camera Demo ------------------------------------"));
    Serial.flush ();

    thread::atomicx::Start();

    Serial.println (F("FULL DEADLOCK ------------------------------------"));
    Serial.flush ();

}

void loop() {
}
