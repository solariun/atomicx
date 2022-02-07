
#include "arduino.h"
#include "utils.hpp"

// Global values

TermCommands termCmds = TermCommands::none;
SysCommands sysCmds = SysCommands::none;

// Global functions

void ListAllThreads()
{
    size_t nCount=0;

    Serial.flush();

    Serial.println (F("\e[K[THREAD]-----------------------------------------------"));

    Serial.printf ("\e[K>>> Free RAM: %ub\t\n", system_get_free_heap_size ());
    Serial.printf ("\e[KAtomicX context size: %zub\r\n", sizeof (thread::atomicx));

    Serial.println ("\e[K---------------------------------------------------------");

    for (auto& th : *(thread::atomicx::GetCurrent()))
    {
        Serial.printf ("\eK%c%-3u %-8zu '%-12s' nc:%-6u stk:(%6zub/%6zub) sts:(%-3u,%-3u) last: %ums\r\n",
            thread::atomicx::GetCurrent() == &th ? F("*  ") : F("   "), ++nCount, 
            (size_t) th.GetID(), th.GetName(),
            th.GetNice(), th.GetUsedStackSize(), th.GetStackSize(),
            th.GetStatus(), th.GetSubStatus(),
            th.GetLastUserExecTime()
        );

        Serial.flush();
    }

    Serial.println ("\e[K---------------------------------------------------------");
    Serial.flush(); Serial.flush();

}

// Namespaced functions and attributes
namespace util
{
    void InitSerial(void)
    {
        Serial.begin (115200);

        if (! Serial)
        {

            // Wait serial to proper start.
            while (! Serial);
        }
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
