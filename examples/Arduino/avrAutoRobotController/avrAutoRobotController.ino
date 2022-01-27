/*

*/

#include "arduino.h"

#include "atomicx.hpp"

#include <stdio.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

using namespace thread;

size_t nDataCount=0;

// Used for accumulate the commands.
String readCommand = "";

void ListAllThreads();

#define PrintStackOverflow() \
        Serial.print (__FUNCTION__); \
        Serial.print ("["); \
        Serial.print (GetName ()); \
        Serial.print (F("/")); \
        Serial.print ((size_t) this); \
        Serial.print ("]:  StackSize: ["); \
        Serial.print (GetStackSize ()); \
        Serial.print ("]. Stack used ["); \
        Serial.print (GetUsedStackSize()); \
        Serial.println (F("]")); \
        Serial.flush(); \


// -------------------------------------------------------------------------------------------

size_t GetFreeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

atomicx_time Atomicx_GetTick(void)
{
    ::yield();
    return millis();
}

void Atomicx_SleepTick(atomicx_time nSleep)
{
    delay(nSleep);
}

constexpr size_t GetStackSize(size_t sizeRef)
{
    return sizeRef * sizeof (size_t);
}

// -------------------------------------------------------------------------------------------

uint8_t ParseOption (const String& commandLine, uint8_t nCommandIndex, String& returnText, bool countOnly=false)
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

    String strBuffer="";

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
                    strBuffer.remove(0, strBuffer.length());
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

                returnText.remove(0,returnText.length());
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
                            returnText.concat (static_cast<char> (atoi (strBuffer.c_str ())));
                            strBuffer.remove(0, strBuffer.length());
                        }
                        else
                        {
                            returnText.concat (static_cast<char>(chChar)); /* code */
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

// -------------------------------------------------------------------------------------------

// ==================== MOTOR DEFINITIONS ====================
const int stepX = 2;
const int dirX  = 5;

const int stepY = 3;
const int dirY  = 6;

const int stepZ = 4;
const int dirZ  = 7;

const int stepA = 12;
const int dirA = 13;

const int enPin = 8;
//=============================================================

enum class MotorStatus
{
    ready,
    busy,
    error,
    disabled,
    response,
    request,
    moveRequest,
    moveComplete
};

const uint8_t motorsContext = 4;

struct MotorData
{
    MotorData(): encoder{0}, movement {0}, volts{0}, status{MotorStatus::ready}
    {}

    float encoder;
    float movement;
    float volts;

    MotorStatus status;
} g_motors [motorsContext];


void MoveMotor (struct MotorData& motor, size_t nDistance)
{
    (void) motor;

    while (nDistance--)
    {
        atomicx::GetCurrent()->Yield(10);
    }
}

/**
 * @brief SERIAL COMM
 *
 */
class Motor : public atomicx
{
public:
    Motor(uint32_t nNice, char motorLetter, MotorData& motor, int dirPin, int stepperPin) : atomicx (0, 10), m_motor(motor), m_directionPin(dirPin), m_stepperPin(stepperPin)
    {
        SetNice(nNice);

        // Setup output pins to control motor driver
        pinMode(stepperPin,OUTPUT);
        pinMode(dirPin,OUTPUT);

        // Set Initial direction
        digitalWrite(dirPin,HIGH);

    }

    const char* GetName () override
    {
        return "Motor";
    }

    ~Motor()
    {
        Serial.print("Deleting ");
        Serial.println ((size_t) this);
    }

    void run() noexcept override
    {
        size_t nNotified=0;
        uint8_t nCurrentMotor = ((size_t)&m_motor - (size_t)&g_motors) / sizeof (struct MotorData);

        do
        {
            size_t nMessage = 0;

            if (Wait (nMessage, motorsContext, (size_t) MotorStatus::request, 100))
            {
                Serial.print ("Motor "); Serial.print ((char) ('A' + nCurrentMotor));
                Serial.println (":Notification received.");

                if ((MotorStatus) nMessage == MotorStatus::moveRequest && m_motor.movement > 0)
                {
                    // Processing Motor
                    Serial.print (F("Thread "));
                    Serial.print (GetID ());
                    Serial.print (F(": Moving motor "));
                    Serial.print (m_motor.movement);
                    Serial.print (F(" from: "));
                    Serial.print (m_motor.encoder);
                    Serial.print (F(", to: "));
                    Serial.print (m_motor.movement + m_motor.encoder);
                    Serial.print (F(", moving:"));
                    Serial.println (m_motor.movement);
                    Serial.flush();

                    MoveMotor (m_motor, m_motor.movement);

                    m_motor.encoder +=  m_motor.movement;
                }

                if (SyncNotify ((size_t) nCurrentMotor, motorsContext, (size_t) MotorStatus::response, 1000) == 0)
                {
                    Serial.print (F("ERROR Motor "));
                    Serial.print ((char) ('A' + nCurrentMotor));
                    Serial.println (F(": System was not notified.\n"));
                }

                Serial.flush ();
            }

            m_motor.volts = static_cast<float>(random (2000, 2700)) / 100.0;

        } while (true);
    }

    void StackOverflowHandler(void) noexcept final
    {
        PrintStackOverflow ();
    }

private:
    struct MotorData& m_motor;
    int m_directionPin;
    int m_stepperPin;
};


// -------------------------------------------------------------------------------------------
enum class terminalStatus : size_t
{
    command=10,
    done,
    status,
    ok,
    error
};

enum class TerminalCommands : size_t
{
    none,
    system,
    move,
};

uint8_t ConvertStrCommand (String& strCommand)
{
    TerminalCommands ret = TerminalCommands::none;
#define IFCOMMAND(cmd) if (strCommand == #cmd) ret = TerminalCommands::cmd

    if (strCommand.length())
    {
        IFCOMMAND (system);
        else IFCOMMAND (move);
    }

    return static_cast<uint8_t>(ret);
}

// -------------------------------------------------------------------------------------------

#define TERMINAL_BS 8

/**
 * @brief SERIAL COMM
 *
 */
class Terminal : public atomicx
{
public:
    Terminal(uint32_t nNice) : atomicx (0, 10)
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "Terminal";
    }

    ~Terminal()
    {
        Serial.print("Deleting ");
        Serial.println ((size_t) this);
    }

    bool WaitForSerialData (atomicx_time nTime)
    {
        do
        {
            Yield (nTime);

            if (! Serial)
            {
                return false;
            }
        } while (Serial.available () == 0);

        return true;
    }

    bool ReadCommandLine (String& readCommand)
    {
        uint8_t chChar = 0;

        readCommand = "";

        while (WaitForSerialData (readCommand.length() ? 0 : 100) == true)
        {
            Serial.readBytes (&chChar, 1);
            // Serial.printf ("Read: (%u)-> [%c]\n\r", chChar, chChar >= 32 && chChar < 127 ? chChar : '.');

            if (chChar == TERMINAL_BS && readCommand.length () > 0)
            {
                Serial.print ((char)TERMINAL_BS);
                Serial.print (' ');
                Serial.print ((char)TERMINAL_BS);

                readCommand.remove (readCommand.length ()-1);
            }
            else if (chChar == '\r')
            {
                Serial.println (F(""));
                break;
            }
            else if (chChar >= 32 && chChar < 127)
            {
                Serial.print ((char)chChar);
                readCommand.concat ((char) chChar);
            }
        }

        if (! Serial)
        {
            return false;
        }

        readCommand.trim();

        return true;
    }

    void run() noexcept override
    {
        size_t nNotified=0;

        do
        {
            Serial.print (F("free:"));
            Serial.print (GetFreeRam ());
            Serial.print (F(" bytes, Robot>"));
            Serial.flush();

            if (ReadCommandLine (readCommand) && readCommand != "")
            {
                nDataCount++;

                // SyncNotify System to execute a command
                if ((nNotified = SyncNotify ((size_t) terminalStatus::command, readCommand, 1, 1000)) == 0)
                {
                    Serial.println (F("Terminal: System is BUSY, please try again."));
                }
                else
                {
                    // Wait the command to be executed, only a single notification is sent
                    if (Wait (readCommand, (size_t) terminalStatus::done) == false)
                    {
                        Serial.println (F("ERROR, Failed to query command completion."));
                    }
                    else
                    {
                        Serial.println (F("Done"));
                    }
                }
            }

            Serial.flush ();

        } while (Yield ());
    }

    void StackOverflowHandler(void) noexcept final
    {
        PrintStackOverflow ();
    }
};


// -------------------------------------------------------------------------------------------


/**
 * @brief SYSTEM THREAD
 *
 */
class System : public atomicx
{
public:
    System(uint32_t nNice) :  atomicx ()
    {
        SetNice(nNice);
    }

    const char* GetName () override
    {
        return "System";
    }

    ~System()
    {
        Serial.print (F("Deleting Consumer: ID:"));
        Serial.println ((size_t) this);
    }

    inline void ExecuteSystemCommand (String& strCommand)
    {
        ListAllThreads ();

        Serial.println (F("[Motors] ---------------------------------"));

        for (int nCount=0; nCount < motorsContext; nCount++)
        {
            Serial.print (F("Motor ")); Serial.print ((char) ('A' + nCount));
            Serial.print (F(": Enc: ")); Serial.print (g_motors[nCount].encoder);
            Serial.print (F(", Mv: ")); Serial.print (g_motors[nCount].movement);
            Serial.print (F(", Volts: ")); Serial.print (g_motors[nCount].volts);
            Serial.print (F("v, Sts:")); Serial.println ((size_t) g_motors[nCount].status);
        }
    }

    inline void ExecuteMoveCommand (String& strCommand)
    {
        String strParam = F("");
        uint8_t nParams = ParseOption (readCommand, 1, strParam, true);
        size_t nNofied = 0;

        if (nParams == 5)
        {
            ParseOption (readCommand, 1, strParam);
            g_motors[0].movement = strParam.toFloat();

            ParseOption (readCommand, 2, strParam);
            g_motors[1].movement = strParam.toFloat();

            ParseOption (readCommand, 3, strParam);
            g_motors[2].movement = strParam.toFloat();

            ParseOption (readCommand, 4, strParam);
            g_motors[3].movement = strParam.toFloat();

            /**
             * @brief   Instead of using SyncNotify que look for at least 1 Wait call blocked, it uses
             *          LookForWaitings to block until at least 4 threads are blocked Waiting
             */
            if (LookForWaitings (motorsContext, (size_t) MotorStatus::request, 10000, 4))
            {
                if ((nNofied = Notify ((size_t) MotorStatus::moveRequest, motorsContext, (size_t) MotorStatus::request, NotifyType::all)) == false)
                {
                    Serial.println (F("ERROR, failed send command to all Motors"));
                    return;
                }
            }

            Serial.print (F("\n\nNotified motors: "));
            Serial.print (nNofied);
            Serial.print (F(", has:"));
            Serial.println (IsWaiting (motorsContext, (size_t) MotorStatus::request));
            Serial.flush ();

            // ------------------------------------------------
            //Asynchronously wait for the motors to finish
            // ------------------------------------------------

            uint8_t nRspCounter=0;
            size_t nMotorResponse=0;

            if (nNofied == motorsContext)
            {
                while (nRspCounter < motorsContext)
                {
                    if (Wait (nMotorResponse, motorsContext, (size_t) MotorStatus::response, 1000))
                    {
                        Serial.print (F(">> Motor ")); Serial.print ((char)('A' + (char) nMotorResponse));
                        Serial.println (F(": Reported completion. "));
                        Serial.flush ();

                        nRspCounter++;
                    }
                }
            }
            else
            {
                ListAllThreads ();
            }
        }
        else
        {
            Serial.println (F("MOVE: Please use move motorA motorB motorC motorD"));
            Serial.println (F("ERROR"));
        }
    }

    inline void ExecuteCommand(String& strCommand, uint8_t& nCommandCode)
    {
        switch (nCommandCode)
        {
            case (uint8_t) TerminalCommands::system:
                ExecuteSystemCommand (strCommand);
                break;

            case (uint8_t) TerminalCommands::move:
                ExecuteMoveCommand (strCommand);
                break;

            default:
                Serial.println (F("Command not implemented."));
        }

        return;
    }

    void UpdateMotorVoltages(void)
    {
        for (int nCount=0; nCount < motorsContext; nCount++)
        {
            g_motors[nCount].volts = static_cast<float>(random (2000, 2700)) / 100.0;
        }
    }

    void run() noexcept override
    {
        size_t nCount=0;
        size_t nMessage = 0;

        do
        {
            if (Wait (nMessage, readCommand, 1, GetNice ()))
            {
                if (nMessage == (size_t) terminalStatus::command)
                {
                    String strCommandName = F("");
                    auto numParams = ParseOption (readCommand, 0, strCommandName, true);
                    ParseOption (readCommand, 0, strCommandName);
                    auto nCommandCode = ConvertStrCommand (strCommandName);

                    Serial.print (F("System thread ID:"));
                    Serial.print (GetID());
                    Serial.print (F(", Num Params: "));
                    Serial.print (numParams);
                    Serial.print (F(", Command: "));
                    Serial.print (strCommandName);
                    Serial.print (F("("));
                    Serial.print ((int)nCommandCode);
                    Serial.print (F("), Message: "));
                    Serial.println (readCommand);

                    if (nCommandCode > 0)
                    {
                        ExecuteCommand (readCommand, nCommandCode);
                    }
                    else
                    {
                        Serial.print (F("Command, "));
                        Serial.print (strCommandName);
                        Serial.println (F(", is not valid."));
                    }

                    if (SyncNotify (readCommand, (size_t) terminalStatus::done, 1000) == 0)
                    {
                        Serial.println (F("Error, Terminal not responding. Please check."));
                    }
                }

                Serial.flush ();
            }
        } while (true);
    }

    void StackOverflowHandler(void) noexcept final
    {
        PrintStackOverflow ();
    }

};


// -------------------------------------------------------------------------------------------


void ListAllThreads()
{
    size_t nCount=0;

    Serial.flush();

    Serial.println (F("[THREAD]-----------------------------------------------"));

    Serial.print (F(">>> Free RAM: ")); Serial.println (GetFreeRam());
    Serial.print (F("AtomicX context size: ")); Serial.println (sizeof (atomicx));
    Serial.print (F("Terminal Context:")); Serial.println (sizeof (Terminal));
    Serial.print (F("Motor Context:")); Serial.println (sizeof (Motor));
    Serial.print (F("System Context:")); Serial.println (sizeof (System));

    Serial.println ("---------------------------------------------------------");

    for (auto& th : *(atomicx::GetCurrent()))
    {
        Serial.print (atomicx::GetCurrent() == &th ? "*  " : "   ");
        Serial.print (++nCount);
        Serial.print (F(":'"));
        Serial.print (th.GetName());
        Serial.print (F("' "));
        Serial.print ((size_t) th.GetID());
        Serial.print (F("\t| Nice: "));
        Serial.print (th.GetNice());
        Serial.print (F("\t| Stack: "));
        Serial.print (th.GetStackSize());
        Serial.print ("\t| UsedStack: ");
        Serial.print (th.GetUsedStackSize());
        Serial.print (F("\t\t| Stat: "));
        Serial.print (th.GetStatus());
        Serial.print (F("\t| SStat: "));
        Serial.print (th.GetSubStatus());
        Serial.print (F("\t| LstExecTime: "));
        Serial.print (th.GetLastUserExecTime());
        Serial.println (F("ms"));
        Serial.flush();
    }

    Serial.println ("---------------------------------------------------------");
    Serial.flush();
}


// -------------------------------------------------------------------------------------------


void setup()
{
    Serial.begin (115200);

    while (! Serial) delay (100);

    // if analog input pin 0 is unconnected, random analog
    // noise will cause the call to randomSeed() to generate
    // different seed numbers each time the sketch runs.
    // randomSeed() will then shuffle the random function.
    randomSeed(analogRead(0));

    //MOTD
    {
        delay (1000);
        Serial.println (F("\n\n"));
        Serial.println (F(ATOMIC_VERSION_LABEL));
        Serial.println (F("\n Starting Up...."));
        delay (500);

        Serial.println (F("-------------------------------------------------"));
        Serial.println (F(" ATOMICX DEMO FOR ROBOT CONTROL SYSTEM "));
        Serial.print (F("FREE RAM: ")); Serial.println (GetFreeRam ());
        Serial.println (F("-----------------------------------------------\n"));

        Serial.flush();
    }

    System systemThread (100);

    Terminal terminalThread (0);

    Motor motorsThreadA (200, 'A', g_motors[0], dirA, stepA);
    Motor motorsThreadB (200, 'B', g_motors[1], dirX, stepX);
    Motor motorsThreadC (200, 'C', g_motors[2], dirY, stepY);
    Motor motorsThreadD (200, 'D', g_motors[3], dirZ, stepZ);

    atomicx::Start();

    Serial.println (F("Full lock detected..."));

    ListAllThreads ();

}

void loop() {

}
