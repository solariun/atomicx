/**
 * @file Listner.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "Arduino.h"
#include "atomicx.hpp"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <sstream>

#include "TelnetTerminal.hpp"

#define MAX_SRV_CLIENTS 5

class ListenerServer : public thread::atomicx
{
public:
    ListenerServer(atomicx_time nNice) : thread::atomicx (250, 50), m_tcpServer(m_tcpPort)
    {
        SetNice (nNice);
    }

protected:

    bool InitiateNetwork (Timeout timeout)
    {
        util::SetDisplayMessage ("Connecting to WiFi");

        WiFi.begin (m_ssid, m_pass);

        while (WiFi.status () != WL_CONNECTED && timeout.IsTimedout() == false)
        {
            Yield (500);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            m_wifiStatus = true;

            util::SetDisplayMessage ("Connected to WIFI.");
        }
        else
        {
            util::SetDisplayMessage ("Failed to connect.");
        }

        return m_wifiStatus;
    }

    bool StartUdpServices (Timeout timeout)
    {
        while ((m_updServerStatus = m_udp.begin (m_udpPort)) == false && ! timeout.IsTimedout ())
        {
            Yield (200);
        }

        return m_updServerStatus;
    }

    bool StartingTcpServices (Timeout timeout)
    {
        do
        {
            m_tcpServer.begin ();    /* code */
        } while (m_tcpServer.status () != LISTEN && timeout.IsTimedout ());
        
        if (m_tcpServer.status () == LISTEN) m_tcpServer.setNoDelay (true);

        return timeout.IsTimedout () ? false : true;
    }

    void run() noexcept final
    {
        char readBuffer [50] = "";
        ssize_t nAvaliableData = 0;
        std::string strBuffer(0,' '); 

        strBuffer.reserve (sizeof (readBuffer));

        do
        {
            if (m_wifiStatus == false && InitiateNetwork (60000))
            {
                if (m_updServerStatus == false)
                {
                    StartingTcpServices (1000);
                    Serial.printf ("TCP status: %u\n", m_tcpServer.status ());
                    Serial.flush ();
                }

                if (m_updServerStatus == false)
                {
                    StartUdpServices (1000);
                }

                util::SetDisplayMessage (WiFi.localIP ().toString().c_str ());
            }
            else
            {
                if ((nAvaliableData = (ssize_t) m_udp.parsePacket ()) > 0)
                {
                    strBuffer.clear ();
                    ssize_t nRead;

                    do
                    {
                        memset ((void*) readBuffer, 0, sizeof (readBuffer));
                        
                        nRead = m_udp.read (readBuffer, sizeof (readBuffer)-1);
                        strBuffer.append (readBuffer, nRead);

                        nAvaliableData  -= nRead;
                        
                    } while (m_udp.available ());

                    m_udp.flush ();

                    util::SetDisplayMessage (strBuffer, false);

                    Serial.print ("\nReceived: ");
                    Serial.println (strBuffer.c_str ());
                    Serial.flush ();
                }
                
                if (m_tcpServer.hasClient ())
                {
                    Serial.println ("\n\nStarting up a remote client...");
                    Serial.flush ();

                    // Starting a new thread
                    new TelnetTerminal (10, m_tcpServer);
                }
            }            
        } while (Yield ());
    }

    void StackOverflowHandler(void) noexcept override
    {
        PrintStackOverflow();
    }

    const char* GetName ()
    {
        return "NetServices";
    }

private:
    bool m_wifiStatus = false;

    const char *m_ssid = "WhiteKingdom2.4Ghz";
    const char *m_pass = "Creative01000";
    const unsigned int m_localTelnetPort = 22;

    union Ip
    {
        uint8_t num [4];
        uint32_t address;
    };

    // UDP Server
    bool m_updServerStatus = false;
    bool m_telnetServerStatus = false;

    const uint m_udpPort = 2221;
    WiFiUDP m_udp;

    // Telnet IP server
    const uint m_tcpPort = 23;

    WiFiServer m_tcpServer;
};
