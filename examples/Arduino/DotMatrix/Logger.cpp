/**
 * @file Logger.hpp
 * @author Gustavo Campos (lgustavocampos@gmail.com)
 * @brief  General Logger facility for c++
 * @version 0.1
 * @date 2022-03-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "Arduino.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <streambuf>

#include "Logger.hpp"

LoggerStream logger (Serial);

LoggerStreamBuffer::LoggerStreamBuffer(Stream& stream, LogType minimalLogType) : 
    m_msgLogType (minimalLogType), m_logType(minimalLogType), m_stream(stream), m_strMessage{}
{}

LoggerStreamBuffer::~LoggerStreamBuffer()
{}

std::streamsize LoggerStreamBuffer::FlushBuffer()
{
    std::streamsize length = m_strMessage.length ();

    if (m_msgLogType <= m_logType)
    {
        m_stream.printf ("\r\e[K%-8u:[%s]:%s\n", millis (), GetLogTypeName (), m_strMessage.c_str ());
        m_stream.flush ();
    }

    m_strMessage = "";
    m_msgLogType = LogType::Debug;

    return length;
}

 const char* LoggerStreamBuffer::GetLogTypeName ()
 {
     switch (m_msgLogType)
     {
         case LogType::Emergency: return "Emergency";
         case LogType::Alert: return "Alert";
         case LogType::Critical: return "Critical";
         case LogType::Error: return "Error";
         case LogType::Warning: return "Warning";
         case LogType::Info: return "Info";
         case LogType::Debug: return "Debug";
     };

     return "Undefined";
 }
 
 /**
 * @brief   streambuf virtual method to process a buffer of bytes
 *
 * @param   streamBuffer      The provided bytes buffer
 * @param   length            The size of the bytes buffer
 *
 * @return std::streamsize  The total utilized bytes from the buffer
 */
std::streamsize LoggerStreamBuffer::xsputn(const char_type* streamBuffer, std::streamsize length)
{
    m_strMessage.append (streamBuffer, length);
    return length;
}

    /**
 * @brief   streambuf virtual method to process a single character
 *
 * @param   character   The single character to process
 *
 * @return  int_type    The processed character
 * 
 * @note    The type can only be defined once by each message.
 *          any other attempt will be discarded
 */
int LoggerStreamBuffer::overflow(int_type character)
{
    if (character == '\n')
    {
        FlushBuffer ();
    }
    else
    {
        m_strMessage.append ((char*) &character, 1);
    }

    return 1;
}

void LoggerStreamBuffer::SetMessageType (LogType logType)
{
    m_msgLogType = logType;
}

void LoggerStreamBuffer::SetMinimalLogType (LogType logType)
{
    m_logType = logType;
}

LoggerStream::LoggerStream (Stream& stream, LogType logType) : sbuffer (stream, logType)
{
    this->rdbuf (&sbuffer);
}

void LoggerStream::SetMessageType (LogType logType)
{
    sbuffer.SetMessageType (logType);
}

void LoggerStream::flush ()
{
    sbuffer.FlushBuffer ();
}

const char* LoggerStream::GetLogTypeName ()
{
    return sbuffer.GetLogTypeName ();
}

void LoggerStream::SetMinimalLogType (LogType logType)
{
    sbuffer.SetMinimalLogType (logType);
}