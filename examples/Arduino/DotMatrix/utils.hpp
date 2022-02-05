
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include "arduino.h"

// global functions and attributes

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

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

enum class TermCommands
{
    ThermalCam_Update,
    ThreadList_Update
};

static TermCommands termCmds;

enum class SysCommands
{
    Matrix_Ready=10
};

static SysCommands sysCmds;

// Namespaced functions and attributes
namespace util
{
    void InitSerial(void);

    constexpr size_t GetStackSize(size_t sizeRef)
    {
        return sizeRef * sizeof (size_t);
    }
}

namespace vt100
{
    void SetLocation (uint16_t nY, uint16_t nX);

    // works with 256 colors
    void SetColor (const uint8_t nFgColor, const uint8_t nBgColor);

    void ResetColor ();

    void HideCursor ();

    void ShowCursor ();

    void ClearConsole ();

    void ReverseColor ();
}

#endif