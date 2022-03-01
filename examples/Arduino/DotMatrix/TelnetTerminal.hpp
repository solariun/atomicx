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

#ifndef __TELNETTERMINAL_HPP__
#define __TELNETTERMINAL_HPP__

#include "atomicx.hpp"
#include "Arduino.h"

#include <ESP8266WiFi.h>

#include <assert.h>
#include <string>
#include <sstream>

#include "utils.hpp"
#include "TerminalInterface.hpp"

class TelnetTerminal : public TerminalInterface
{
public:
    TelnetTerminal() = delete;
    TelnetTerminal (atomicx_time nNice, WiFiServer& server);

protected:
    void PrintMOTD() final;

    virtual void finish() noexcept override;

    virtual bool IsConnected () override;

    virtual const char* GetName () override;

private:
    WiFiClient m_telnetClient;
    char pszName [20];
};

#endif