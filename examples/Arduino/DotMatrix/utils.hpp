
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <ESP8266WiFi.h>
#include "atomicx.hpp"
#include "Arduino.h"

#include <cstdint>

#include "Logger.hpp"

extern LoggerStream logger;

constexpr std::size_t operator "" _z ( unsigned long long n )
    { return n; }


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
    none=0,
    ThermalCam_Update,
    ThreadList_Update
};

extern TermCommands termCmds;

enum class SysCommands
{
    none = 0,
    Matrix_Ready=10
};

extern SysCommands sysCmds;

// Global functions
void ListAllThreads(Stream& client);

// Namespaced functions and attributes
namespace util
{
    constexpr size_t GetStackSize(size_t sizeRef)
    {
        return sizeRef * sizeof (size_t);
    }

    void InitSerial(void);
    
    void ltrim(std::string &s);

    void rtrim(std::string &s);

    void trim(std::string &s);

    bool SetDisplayMessage (const std::string& strMessage, bool wait = true);

    const std::string GetDisplayMessage (void);
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