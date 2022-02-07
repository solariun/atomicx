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
#include "TerminalInterface.hpp"

// Static initializations
CommandTerminalMap TerminalInterface::m_mapCommands={};

// Default initializations for commands
commands::Help l_helpCommandInstance;
commands::System l_systemCommand;

// Helpper functions
uint8_t ParseOption (const std::string& commandLine, uint8_t nCommandIndex, std::string& returnText, bool countOnly=false)
{
    uint8_t nCommandOffSet = 0;

    nCommandIndex++;

    enum class state : uint8_t
    {
        NoText,
        Word,
        Text,
    } currentState;

    currentState = state::NoText;
    returnText = "";

    bool boolScape = false;

    std::string strBuffer="";

    for (char chChar : commandLine)
    {
        if (currentState == state::NoText)
        {
            if (chChar == '"' || chChar == '\'')
            {
                // set Text state
                currentState = state::Text;
                nCommandOffSet++;
                continue;
            }
            else if (chChar != ' ')
            {
                // Set Word state
                currentState = state::Word;
                nCommandOffSet++;
            }
        }

        if (currentState != state::NoText)
        {
            if (boolScape == false)
            {
                if (currentState == state::Text && chChar == '"' || chChar == '\'')
                {
                    currentState = state::NoText;
                }
                else if (currentState == state::Word && chChar == ' ')
                {
                    currentState = state::NoText;
                }
                if (chChar == '\\')
                {
                    boolScape = true;
                    strBuffer.pop_back ();
                    continue;
                }
            }
            else if (!isdigit (chChar))
            {
                boolScape = false;
            }

            if (currentState == state::NoText)
            {
                if (countOnly == false && nCommandIndex == nCommandOffSet)
                {
                    break;
                }

                returnText = "";
            }
            else
            {
                if (countOnly == false)
                {
                    if (boolScape == true and isdigit (chChar))
                    {
                        strBuffer += chChar;
                    }
                    else
                    {
                        // To add special char \000\ 00 = number only
                        if (strBuffer.length () > 0)
                        {
                            returnText += (static_cast<char> (atoi (strBuffer.c_str ())));
                            strBuffer = "";
                        }
                        else
                        {
                            returnText += (static_cast<char>(chChar)); /* code */
                        }
                    }
                }
            }
        }
    }

    if (nCommandIndex != nCommandOffSet)
    {
        returnText = "";
    }

    return nCommandOffSet;
}

//  Default methods implementations

TerminalInterface::TerminalInterface(atomicx_time nNice, Stream& client) : atomicx(500, 50), m_client(client)
{
    SetNice (nNice);
}

CommandTerminalMap& TerminalInterface::GetMapCommands ()
{
    return TerminalInterface::m_mapCommands;
}

int TerminalInterface::WaitForSerialData ()
{
    int nChars = 0;

    for (;(nChars = Serial.available ()) == 0; Yield (0));

    return nChars;
}

bool TerminalInterface::ReadCommandLine (std::string& readCommand)
{
    uint8_t chChar = 0;
    int nChars = 0;

    readCommand = "";

    while (nChars || (nChars = WaitForSerialData ()) > 0)
    {
        chChar = Serial.read ();

        if (chChar == TERMINAL_BS && readCommand.length () > 0)
        {
            Serial.print ((char)TERMINAL_BS);
            Serial.print (' ');
            Serial.print ((char)TERMINAL_BS);

            //readCommand.remove (readCommand.length ()-1);
            readCommand.pop_back ();
        }
        else if (chChar == '\r')
        {
            Serial.println (F(""));
            break;
        }
        else if (chChar >= 32 && chChar < 127)
        {
            Serial.print ((char)chChar);
            Serial.flush ();
            Serial.flush ();
            readCommand += ((char) chChar);
        }

        nChars--;
    }

    if (! Serial)
    {
        return false;
    }

    util::trim(readCommand);

    return true;
}

void TerminalInterface::run()
{
    std::string strCommandLine = "";
    std::string strCommand = "";

    for (;;)
    {
        strCommandLine = ""; strCommand = "";

        m_client.print ("Terminal> ");
        m_client.flush ();

        if (ReadCommandLine (strCommandLine) && strCommandLine.length ())
        {
            if (ParseOption (strCommandLine, 0, strCommand))
            {
                if (m_mapCommands.find (strCommand) != m_mapCommands.end())
                {
                    m_mapCommands [strCommand]->Execute(m_client, strCommandLine);
                }
                else
                {
                    m_client.print ("Received Command: ");
                    m_client.println (strCommand.c_str ());
                    m_client.println ("Command not found, here is the list of avaliable commands:");
                    m_mapCommands ["help"]->Execute(m_client, strCommandLine);
                }
            }
        }

        m_client.flush ();

        Yield ();
    }
}

void TerminalInterface::StackOverflowHandler(void)
{
    PrintStackOverflow();
}

const char* TerminalInterface::GetName ()
{
    return "TerminalInterface";
}

// CommandTerminalInterface implementations
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
    return TerminalInterface::GetMapCommands ();
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
        client.printf ("%-15s %s\r\n", item.first.c_str(), item.second->GetCommandDescription ());
    }

    return true;
}

void TerminalInterface::PrintMOTD ()
{
    m_client.println ("-------------------------------------");
    m_client.println ("Dotmatrix OS");
    m_client.println ("Welcome to the terminal session.");
    m_client.println ("-------------------------------------");
    m_client.flush();
}

Stream& TerminalInterface::client (void)
{
    return m_client;
}