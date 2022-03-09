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

#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <cstdint>
#include <iostream>
#include <string>
#include <streambuf>
#include <set>
#include <memory>

enum class LogType : uint8_t
{
    Emergency,
    Alert,
    Critical,
    Error,
    Warning,
    Info,
    Debug,
};

namespace  LOG
{
    static const char* GetLogTypeName (LogType logType)
    {
        switch (logType)
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
} // namespace  LOG

class LoggerInterface
{
    public:
        virtual ~LoggerInterface ();

        virtual void flush (LogType LogType, const std::string& strMessage);
        virtual void init ();
};

class LoggerStreamBuffer : public std::streambuf
{
public:
    LoggerStreamBuffer () = delete;
    LoggerStreamBuffer(LogType minimalLogType);
     
    ~LoggerStreamBuffer();
    
     std::streamsize FlushBuffer();

protected:

    friend class LoggerStream; 

     /**
     * @brief   streambuf virtual method to process a buffer of bytes
     *
     * @param   streamBuffer      The provided bytes buffer
     * @param   length            The size of the bytes buffer
     *
     * @return std::streamsize  The total utilized bytes from the buffer
     */
    std::streamsize xsputn(const char_type* streamBuffer, std::streamsize length) override;

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
    int_type overflow(int_type character) override;

    void SetMessageType (LogType logType);

    void SetMinimalLogType (LogType logType);

    const char* GetLogTypeName ();

    bool AddLogger (LoggerInterface* externalLogger);

private:
    LogType m_msgLogType;
    LogType m_logType;
    std::string m_strMessage;

    static std::set<std::unique_ptr<LoggerInterface>> loggerList;
};

class LoggerStream : public std::ostream
{
public:
    LoggerStream () = delete;

    LoggerStream (LogType logType = LogType::Debug);

    void SetMessageType (LogType logType);
    void SetMinimalLogType (LogType logType);

    void flush ();

    const char* GetLogTypeName ();
    
    bool AddLogger (LoggerInterface* externalLogger);

protected:
private:
    LoggerStreamBuffer sbuffer;
};

extern LoggerStream logger;

namespace  LOG
{
    static std::ostream& EMERGENCY (std::ostream& os)
    {
        LoggerStream& log = static_cast<LoggerStream&>(os);
            
        log.SetMessageType (LogType::Emergency);

        return os;
    }

    static std::ostream& ALERT (std::ostream& os)
    {
        LoggerStream& log = static_cast<LoggerStream&>(os);
            
        log.SetMessageType (LogType::Alert);

        return os;
    }

    static std::ostream& CRITICAL (std::ostream& os)
    {
        LoggerStream& log = static_cast<LoggerStream&>(os);
            
        log.SetMessageType (LogType::Critical);

        return os;
    }

    static std::ostream& ERROR (std::ostream& os)
    {
        LoggerStream& log = static_cast<LoggerStream&>(os);
            
        log.SetMessageType (LogType::Error);

        return os;
    }

    static std::ostream& WARNING (std::ostream& os)
    {
        LoggerStream& log = static_cast<LoggerStream&>(os);
            
        log.SetMessageType (LogType::Warning);

        return os;
    }

    static std::ostream& INFO (std::ostream& os)
    {
        LoggerStream& log = static_cast<LoggerStream&>(os);
            
        log.SetMessageType (LogType::Info);

        return os;
    }

    static std::ostream& DEBUG (std::ostream& os)
    {
        LoggerStream& log = static_cast<LoggerStream&>(os);
            
        log.SetMessageType (LogType::Debug);

        return os;
    }
    
} // namespace  LOG
#endif