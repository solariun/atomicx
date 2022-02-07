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
#include <map>

#include "utils.hpp"

#include "TerminalCommand.hpp"

// Methods implementations
commands::System::System() : CommandTerminalInterface ("system")
{return;}

const char* commands::System::GetCommandDescription()
{
    return "Print system information";
}

bool commands::System::Execute(Stream& client, const std::string& commandLine)
{
    client.println ("ESP8266 System ------------------");
    client.printf ("%-20s: [%u]\r\n", "Processor ID", system_get_chip_id ());
    client.printf ("%-20s: [%s]\r\n", "SDK Version", system_get_sdk_version ());
    client.printf ("%-20s: [%uMhz]\r\n", "CPU Freequency", system_get_cpu_freq ());
    client.printf ("%-20s: [%u Bytes]\r\n", "Memory", system_get_free_heap_size ());
    // if (WiFi.status () != WL_NO_SHIELD)
    // {
    //     client.println ("-[Connection]----------------------");
    //     //client.printf ("%-20s: [%s] (%u)\r\n", "Status", GetWiFiStatus (), WiFi.status ());
    //     client.printf ("%-20s: [%s]\r\n", "Mac Address", WiFi.macAddress ().c_str ());
    //     if (WiFi.status () == WL_CONNECTED)
    //     {
    //         client.printf ("%-20s: [%s]\r\n", "SSID", WiFi.SSID ().c_str ());
    //         client.printf ("%-20s: [%d dBm]\r\n", "Signal", WiFi.RSSI ());
    //         client.printf ("%-20s: [%u Mhz]\r\n", "Channel", WiFi.channel ());
    //         client.printf ("%-20s: [%s]\r\n", "IPAddress", WiFi.localIP ().toString ().c_str ());
    //         client.printf ("%-20s: [%s]\r\n", "Net Mask", WiFi.subnetMask ().toString ().c_str ());
    //         client.printf ("%-20s: [%s]\r\n", "Gateway", WiFi.gatewayIP ().toString ().c_str ());
    //         client.printf ("%-20s: [%s]\r\n", "DNS", WiFi.dnsIP ().toString ().c_str ());
    //     }
    //     else
    //     {
    //         client.printf ("Not connected to WiFi...");
    //     }
    // }
    client.println ("-[Process]----------------------");
    ListAllThreads ();

    return true;
}


