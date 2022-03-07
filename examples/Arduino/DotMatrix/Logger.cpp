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

// initializing static list
std::set<std::unique_ptr<LoggerInterface>> LoggerStreamBuffer::loggerList = {};

LoggerStream logger (LogType::Debug);

LoggerInterface::~LoggerInterface ()
{}

void LoggerInterface::flush (LogType logType, const std::string& strMessage)
{
    (void) logType; (void) strMessage;
}

void LoggerInterface::init ()
{

}

LoggerStreamBuffer::LoggerStreamBuffer(LogType minimalLogType) : 
    m_msgLogType (minimalLogType), m_logType(minimalLogType),m_strMessage{}
{}

LoggerStreamBuffer::~LoggerStreamBuffer()
{}

std::streamsize LoggerStreamBuffer::FlushBuffer()
{
    std::streamsize length = m_strMessage.length ();

    if (m_msgLogType <= m_logType)
    {
        for (auto& loggerItem : loggerList)
        {
            loggerItem->flush (m_msgLogType, m_strMessage);
        }
    }

    m_strMessage = "";
    m_msgLogType = LogType::Debug;

    return length;
}

 const char* LoggerStreamBuffer::GetLogTypeName ()
 {
     return LOG::GetLogTypeName (m_msgLogType);
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

bool LoggerStreamBuffer::AddLogger (LoggerInterface* externalLogger)
{
    auto ret = loggerList.insert (std::unique_ptr<LoggerInterface> (externalLogger));

    if (ret.second)
    {
        externalLogger->init ();
    }

    return true; //ret.second;
}

bool LoggerStream::AddLogger (LoggerInterface* externalLogger)
{
    if (externalLogger == nullptr)
    {
        return false;
    }

    return sbuffer.AddLogger (externalLogger);
}

LoggerStream::LoggerStream (LogType logType) : sbuffer (logType)
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