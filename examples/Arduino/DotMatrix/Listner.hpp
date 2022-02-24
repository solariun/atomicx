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

#include "arduino.h"
#include "atomicx.hpp"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <sstream>

class ListenerServer : public thread::atomicx
{
public:
    ListenerServer(atomicx_time nNice) : thread::atomicx (250, 50)
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
                    StartUdpServices (1000);

                    util::SetDisplayMessage ("UDP ready");
                }
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

    const unsigned int m_udpPort = 2221;
    WiFiUDP m_udp;
};
