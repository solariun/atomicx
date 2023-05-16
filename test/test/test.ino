
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

uint8_t endpoint{0};

class TestRecv : public atomicx::Thread
{
public:
    TestRecv() : Thread(100, mStack), mId(mIdCounter++)
    {}

protected:
    void add()
    {
        atomicx::Tick tk;

        yield(10);

        atomicx::Thread::Payload payload = {.type = 1, .message = 0};
        wait(endpoint, payload, 10);

        auto& dt = getParams();
        Serial.print(F("WIT:"));
        Serial.print((size_t)mId);
        Serial.print(F(": Yield Time:"));
        Serial.print(tk.diff());
        Serial.print(F("ms, Stack:"));
        Serial.print(dt.usedStackSize);
        Serial.print(F("/"));
        Serial.print(dt.stackSize);
        Serial.print(F("b"));
        Serial.print(F(", Status:"));
        Serial.print(atomicx::statusName(dt.status));
        // Serial.print(F(", Tick_t:"));
        // Serial.print(sizeof(atomicx::Tick_t));
        // Serial.print(F(", St:"));
        // Serial.print((size_t)dt.status);
        // Serial.print(F(", Ret:"));
        // Serial.print(ret);
        Serial.println(F(""));
        Serial.flush();
    }

    void run()
    {
        while (true) {
            add();
        }
    }

    const char* getName() const override
    {
        return "TH:WAIT";
    }

private:
    size_t mId;
    size_t mStack[25]{};
    static size_t mIdCounter;
};

size_t TestRecv::mIdCounter{0};

class Test : public atomicx::Thread
{
public:
    Test() : Thread(0, mStack), mId(mIdCounter++)
    {}

protected:
    size_t add(size_t nValue)
    {
        atomicx::Tick tk;

        //yield(10);

        size_t ret = notify(endpoint, {1, 0, nValue}, 1);

        nValue++;

        Serial.print(F("NOTF:"));
        Serial.print((size_t)mId);
        Serial.print(F(": Yield Time:"));
        Serial.print(tk.diff());
        Serial.print(F("ms: value:"));
        Serial.print(nValue);
        Serial.print(F(", Stack:"));
        Serial.print(getParams().usedStackSize);
        Serial.print(F("/"));
        Serial.print(getParams().stackSize);
        Serial.print(F("b"));
        // Serial.print(F(", Tick_t:"));
        // Serial.print(sizeof(atomicx::Tick_t));
        // Serial.print(F(", St:"));
        // Serial.print((size_t)dt.status);
        // Serial.print(F(", Ret:"));
        // Serial.print(ret);
        Serial.println(F(""));
        Serial.flush();

        return nValue;
    }

    void y(atomicx::Tick tm = atomicx::TICK_DEFAULT)
    {
        Serial.println(atomicx::TICK_DEFAULT);
        Serial.println(tm.value() == atomicx::TICK_DEFAULT);
        Serial.flush();
        exit(2);
    }
    void run()
    {
        size_t nValue{(size_t)this};
        char testSzStr[] = "TESTE DE TAMANHO";
        while (true) {
            yield();

            nValue = add(nValue);
        }
    }

    const char* getName() const override
    {
        return "TH:NOTF";
    }

private:
    size_t mId;
    size_t mStack[50]{};
    static size_t mIdCounter;
};

template <class T = uint32_t>
class example
{
public:
    bool sum()
    {
        return true;
    }

private:
};

size_t Test::mIdCounter{0};

template <typename func>
void doCalc(func calc)
{
    auto val = calc(10, 20);
}

int cal(int x, int y)
{
    return x * y;
}

Test th;

void setup()
{
    TestRecv threcv;

    auto t = [&](int x, int y) -> int { return x * y; };

    doCalc(t);
    doCalc(cal);

    Serial.begin(115200);

    Serial.println(F(""));
    Serial.println(F("Starting atomicx 3 demo."));
    Serial.flush();
    delay(1000);

    Serial.println("-------------------------------------");

    for (auto& th : atomicx::Thread::getCurrent()) {
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