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

#ifndef __TERMINAL_HPP__
#define __TERMINAL_HPP__

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
#include <assert.h>
#include <string>
#include <map>

#include "utils.hpp"

class Terminal;
class CommandTerminalInterface;

using CommandTerminalMap = std::map<const std::string, CommandTerminalInterface*>;

class CommandTerminalInterface
{
public:
    virtual const char* GetCommandDescription() = 0;

protected:

    friend class Terminal;

    CommandTerminalInterface() = delete;
    CommandTerminalInterface(std::string strCommandNme);

    virtual ~CommandTerminalInterface();

    const std::string& GetCommandName();

    CommandTerminalMap& GetMapCommands();

    virtual bool Execute(Stream& client, const std::string& commandLine) = 0;
    
private:
    const std::string strCommandName;
};

class Terminal : public thread::atomicx
{
public:
    Terminal(atomicx_time nNice);

protected:
    friend class CommandTerminalInterface;

    static CommandTerminalMap& GetMapCommands ();

    void run() noexcept final
    {
        for (;;)
        {
            mapCommands ["help"]->Execute (Serial, (const std::string&) strCommandLine);

            yield ();
            
            mapCommands ["system"]->Execute (Serial, (const std::string&) strCommandLine);

            Yield (2000);
        }
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow();
    }

    const char* GetName ()
    {
        return "Terminal";
    }

private:
    static CommandTerminalMap mapCommands;
    std::string strCommandLine="";
};


namespace commands
{
    class Help : public CommandTerminalInterface
    {
        public:
            Help();

        protected:
            const char* GetCommandDescription() final;
    
            bool Execute(Stream& client, const std::string& commandLine);
    };

}
#endif