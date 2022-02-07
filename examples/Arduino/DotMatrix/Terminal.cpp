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

// Designed for NodeMCU ESP8266
//################# DISPLAY CONNECTIONS ################
// LED Matrix Pin -> ESP8266 Pin
// Vcc            -> 3v  (3V on NodeMCU 3V3 on WEMOS)
// Gnd            -> Gnd (G on NodeMCU)
// DIN            -> D7  (Same Pin for WEMOS)
// CS             -> D4  (Same Pin for WEMOS)
// CLK            -> D5  (Same Pin for WEMOS)

#include "TerminalCommand.hpp"

#include "Terminal.hpp"

// Static initializations
CommandTerminalMap Terminal::mapCommands={};

// Default initializations for commands
commands::Help l_helpCommandInstance;
commands::System l_systemCommand;


//  Default methods implementations
CommandTerminalInterface::CommandTerminalInterface(std::string strCommandName) : strCommandName (strCommandName)
{
    GetMapCommands().insert(std::make_pair(strCommandName, this));
}

CommandTerminalInterface::~CommandTerminalInterface()
{
    GetMapCommands().erase ((const std::string) strCommandName);
}

const std::string& CommandTerminalInterface::GetCommandName()
{
    return strCommandName;
}

CommandTerminalMap& CommandTerminalInterface::GetMapCommands()
{
    return Terminal::GetMapCommands ();
}

Terminal::Terminal(atomicx_time nNice) : atomicx(500, 50)
{
    SetNice (nNice);
}

CommandTerminalMap& Terminal::GetMapCommands ()
{
    return Terminal::mapCommands;
}

// Default command implementation

commands::Help::Help () : CommandTerminalInterface ("help")
{
    return;
}

const char* commands::Help::GetCommandDescription()
{
    return "List this help text";
}

bool commands::Help::Execute(Stream& client, const std::string& commandLine)
{
    CommandTerminalMap& map = GetMapCommands ();

    client.println ("Help");
    client.println ("--------------------------------------");

    for (auto& item : map)
    {
        client.printf ("%-15s %s\n", item.first.c_str(), item.second->GetCommandDescription ());
    }

    return true;
}