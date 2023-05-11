
#include "atomicx.hpp"

uint8_t notify = 0;

atomicx::Tick atomicx::Tick::now(void)
{
    return millis();
}

void atomicx::Tick::sleep(atomicx::Tick nSleep)
{
    delay(nSleep);
}

class Test : public atomicx::Thread
{
public:
    Test() : Thread(0, mStack), mId(mIdCounter++)
    {}

protected:
    size_t add(size_t nValue)
    {
        atomicx::Tick tk;
        
        yield();
        nValue++;
        
        auto& dt = getParams();
        Serial.print((size_t)mId);
        Serial.print(F(": Yield Time:"));
        Serial.print(tk.diff());
        Serial.print(F("ms: value:"));
        Serial.print(nValue);
        Serial.print(F(", Stack:"));
        Serial.print(dt.usedStackSize);
        Serial.print(F("/"));
        Serial.print(dt.stackSize);
        Serial.print(F("b"));
        Serial.print(F(", Tick_t:"));
        Serial.print(sizeof(atomicx::Tick_t));
        Serial.println(F(""));
        Serial.flush();

        return nValue;
    }

    void run()
    {
        size_t nValue{(size_t)this};

        while (true) {
            nValue =  add(nValue);
        }
    }
private:
    size_t mId;
    size_t mStack[15]{};
    static size_t mIdCounter;
};

template <class T = uint32_t>
class example
{
public:
    bool sum(){ return true;}
private:

};

size_t Test::mIdCounter{0};

Test th[2];

void setup()
{
    Serial.begin(115200);

    Serial.println(F(""));
    Serial.println(F("Starting atomicx 3 demo."));
    Serial.flush();
    delay(1000);

    Serial.println("-------------------------------------");

    for (auto& th : th[0]) {
        auto& dt = th.getParams();
        Serial.print(__func__);
        Serial.print(": Listing thread: ");
        Serial.print(", ID:");
        Serial.println((size_t)&th);
        Serial.flush();
    }

    Serial.println("-------------------------------------");

    atomicx::Thread::start();
}

void loop()
{}