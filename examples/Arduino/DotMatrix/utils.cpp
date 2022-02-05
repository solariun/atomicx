
#include "arduino.h"
#include "utils.hpp"

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
