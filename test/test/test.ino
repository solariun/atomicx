
#include "atomicx.hpp"

uint8_t notify = 0;

atx::Tick atx::now(void)
{
    return millis();
}

void atx::sleep(atx::Tick nSleep)
{
    delay(nSleep);
}

uint8_t endpoint{0};

class TestRecv : public atx::Thread
{
public:
    TestRecv() : Thread(100, mStack), mId(mIdCounter++)
    {}

protected:
    void print()
    {
        static atx::Tick tk;

        auto& dt = getParams();
        Serial.print(F("WIT:"));
        Serial.print((size_t)mId);
        Serial.print(F(": Yield Time:"));
        Serial.print(tk.diff());
        Serial.print(F("ms, Stack:"));
        Serial.print(dt.usedStackSize);
        Serial.print(F("/"));
        Serial.print(dt.stackSize);
        Serial.print(F(", value:"));
        Serial.println(mValue);
        // Serial.print(F("b"));
        // Serial.print(F(", Status:"));
        // Serial.print(atx::statusName(dt.status));
        // Serial.print(F(", Tick_t:"));
        // Serial.print(sizeof(atx::Tick_t));
        // Serial.print(F(", St:"));
        // Serial.print((size_t)dt.status);
        // Serial.print(F(", Ret:"));
        // Serial.print(ret);
        // Serial.println(F(""));
        // Serial.flush();

        tk.update();
    }
    void add()
    {
        // atx::Payload payload = {.type = 1, .message = 0};
        // wait(endpoint, payload, 10);

        print();
    }

    void run()
    {
        while (true) {
            {
                atx::Payload payload = {.type = 1, .message = 0};
                wait(endpoint, payload, 100);
                mValue = payload.message;
            }

            //yield();
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
    size_t mValue;
};

size_t TestRecv::mIdCounter{0};

class Test : public atx::Thread
{
public:
    Test() : Thread(1, mStack), mId(mIdCounter++)
    {}

protected:
    void print() const
    {
        static atx::Tick tk;

        Serial.print(F("NOTF:"));
        Serial.print((size_t)mId);
        Serial.print(F(": Yield Time:"));
        Serial.print(tk.diff());
        Serial.print(F("ms, Stack:"));
        Serial.print(getParams().usedStackSize);
        Serial.print(F("/"));
        Serial.print(getParams().stackSize);
        Serial.println(F("b"));
        // Serial.print(F(", Tick_t:"));
        // Serial.print(sizeof(atx::Tick_t));
        // Serial.print(F(", St:"));
        // Serial.print((size_t)dt.status);
        // Serial.print(F(", Ret:"));
        // Serial.print(ret);
        // Serial.println(F(""));
        // Serial.flush();

        tk.update();
    }

    size_t add(size_t nValue)
    {
        print();

        nValue++;

        return nValue;
    }

    void run()
    {
        size_t nValue{(size_t)this};
        char testSzStr[] = "TESTE DE TAMANHO";
        while (true) {
            yield(10);
            //notify(endpoint, {1, nValue}, 10);

            atx::Tick tk;

            nValue = add(nValue);
        }
    }

    const char* getName() const override
    {
        return "TH:NOTF";
    }

private:
    size_t mId;
    size_t mStack[20]{};
    static size_t mIdCounter;
};

size_t Test::mIdCounter{0};

Test th[10];

void setup()
{
    //TestRecv threcv;

    Serial.begin(115200);

    Serial.println(F(""));
    Serial.println(F("Starting atx 3 demo."));
    Serial.flush();
    delay(1000);

    Serial.println("-------------------------------------");

    for (auto& th : atx::Thread::getCurrent()) {
        auto& dt = th.getParams();
        Serial.print(__func__);
        Serial.print(": Listing thread: ");
        Serial.print(", ID:");
        Serial.println((size_t)&th);
        Serial.flush();
    }

    Serial.println("-------------------------------------");

    atx::Thread::start();
}

void loop()
{}