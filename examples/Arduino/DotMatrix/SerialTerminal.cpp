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

#include "atomicx.hpp"
#include "Arduino.h"
#include <assert.h>
#include <string>

#include "utils.hpp"
#include "TerminalInterface.hpp"

#include "SerialTerminal.hpp"

SerialTerminal::SerialTerminal(atomicx_time nNice) : TerminalInterface(nNice, Serial)
{ 
    void InitSerial(void);
}

void SerialTerminal::PrintMOTD ()
{
    client().println ("-------------------------------------");
    client().println ("Dotmatrix OS");
    client().println ("Welcome to Serial based terminal session.");
    client().println ("-------------------------------------");
    client().flush();
}

bool SerialTerminal::IsConnected ()
{
    return ((! Serial) == false);
}

const char* SerialTerminal::GetName ()
{
    return "SerialTerm";
}

void SerialLogger::init ()
{ 
}

void SerialLogger::flush (LogType logType, const std::string& strMessage)
{
    if (Serial)
    {
        Serial.printf ("\r\e[K%-12u", millis ());
        Serial.print (" [");
        Serial.print (LOG::GetLogTypeName (logType));
        Serial.print ("] ");
        Serial.println (strMessage.c_str ());
        Serial.flush ();
    }
}