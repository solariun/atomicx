
#include <string>

#include "Arduino.h"
#include "TextScroller.hpp"
#include "utils.hpp"

// Global values

TermCommands termCmds = TermCommands::none;
SysCommands sysCmds = SysCommands::none;

// Global functions    
void ListAllThreads(Stream& client)
{
    size_t nCount=0;

    client.flush();

    client.println (F("\e[K[THREAD]-----------------------------------------------"));

    client.printf ("\e[K>>> Free RAM: %ub\r\n", system_get_free_heap_size ());
    client.printf ("\e[KAtomicX context size: %zub\r\n", sizeof (thread::atomicx));

    client.println ("\e[K---------------------------------------------------------");

    for (auto& th : *(thread::atomicx::GetCurrent()))
    {
        thread::atomicx::GetCurrent()->Yield (0); 

        client.printf ("\eK%c%-3u %-8zu '%-20s' nc:%-6u stk:(%6zub/%6zub) sts:(%-3u,%-3u) last: %ums\r\n",
            thread::atomicx::GetCurrent() == &th ? '* ' : ' ', ++nCount, 
            (size_t) th.GetID(), th.GetName(),
            th.GetNice(), th.GetUsedStackSize(), th.GetStackSize(),
            th.GetStatus(), th.GetSubStatus(),
            th.GetLastUserExecTime()
        );
    }

    client.println ("\e[K---------------------------------------------------------");

    client.flush();
}

extern std::string strSystemNextMessage;
extern TextScroller Matrix;

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

    // trim from start (in place)
    void ltrim(std::string &s) 
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    void rtrim(std::string &s) 
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    void trim(std::string &s) 
    {
        ltrim(s);
        rtrim(s);
    }

    bool SetDisplayMessage (const std::string& strMessage, bool wait)
    {
        int nRet = 0;
        
        strSystemNextMessage = strMessage;
        
        /**
         * @brief It will wait up to 5 minutes in line to be notified
         */
        if ((! wait || ! thread::atomicx::IsKernelRunning ()) || thread::atomicx::GetCurrent()->SyncNotify (Matrix, TEXTSCROLLER_NOTIFY_NEW_TEXT, 300000)) 
        {
            nRet = true;
        }

        return nRet;
    }

    const std::string GetDisplayMessage (void)
    {
        return Matrix().str();
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
