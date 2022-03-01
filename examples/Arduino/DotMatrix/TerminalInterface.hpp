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

class TerminalInterface;
class CommandTerminalInterface;

using CommandTerminalMap = std::map<const std::string, CommandTerminalInterface*>;

#define TERMINAL_BS 8

/**
 * @brief Parse a command line option and return the command attribute data for a index or how much data present
 * 
 * @param commandLine       Command line text
 * @param nCommandIndex     index fo the desired command attribute
 * @param returnText        command attribute data
 * @param countOnly         if yes will return how much command attributes in the command line
 * 
 * @return uint8_t  the index of what was reported or the number or elements if countOnly = true
 */
uint8_t ParseOption (const std::string& commandLine, uint8_t nCommandIndex, std::string& returnText, bool countOnly=false);

/**
 * CommandTerminal Interdace class
 */
class CommandTerminalInterface
{
public:
    virtual const char* GetCommandDescription() = 0;

protected:

    friend class TerminalInterface;

    CommandTerminalInterface() = delete;
    CommandTerminalInterface(std::string strCommandNme);

    virtual ~CommandTerminalInterface();

    const std::string& GetCommandName();

    CommandTerminalMap& GetMapCommands();

    virtual bool Execute(Stream& client, const std::string& commandLine) = 0;
    
private:
    const std::string strCommandName;
};

/**
 * Terminal interface class
 */
class TerminalInterface : public thread::atomicx
{
public:
    TerminalInterface(atomicx_time nNice, Stream& client);
    Stream& client (void);

protected:
    virtual void PrintMOTD ();

    friend class CommandTerminalInterface;

    static CommandTerminalMap& GetMapCommands ();

    int WaitForClientData (int& nChars);

    bool ReadCommandLine (std::string& readCommand);

    void run() noexcept final;

    void StackOverflowHandler(void) noexcept override;

    const char* GetName ();

    virtual bool IsConnected () = 0;

private:
    static CommandTerminalMap m_mapCommands;
    Stream& m_client;
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