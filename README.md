# AtomicX

**Cooperative multitasking for embedded systems and beyond.**

![image](https://user-images.githubusercontent.com/1805792/125191254-6591cf80-e239-11eb-9e89-d7500e793cd4.png)

[![Version](https://img.shields.io/badge/version-1.3.0-blue.svg)](#)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Arduino%20%7C%20ESP8266%20%7C%20ESP32%20%7C%20STM32%20%7C%20Linux-green.svg)](#supported-platforms)

> **[Architecture & Design Document](design.md)** — class diagrams, state machines, thread lifecycle, intrusive controller internals, and stack management details.

AtomicX is a general-purpose **cooperative thread library** for embedded applications (single-core or confined within another RTOS). It lets you partition your application into multiple controlled execution contexts using cooperative threads — without requiring an operating system, hardware timers, or dynamic memory (unless you opt in).

---

## Key Features

- **Zero stack displacement** — threads run on the real C stack and only back up the minimum necessary bytes on context switch
- **Two stack modes** — fixed-size (user-provided buffer, zero heap) or self-managed (auto-resizing via `malloc`)
- **Portable** — uses only `setjmp`/`longjmp` and `memcpy`; no assembly, no platform-specific code in the core
- **Rich IPC** — Wait/Notify signaling, thread-safe queues, semaphores, read-write mutexes, data pipes (Send/Receive), and broadcast messaging
- **RAII wrappers** — `smartMutex` and `smartSemaphore` for automatic resource release
- **Tiny footprint** — single `.hpp` + `.cpp`, suitable for MCUs with as little as 512 bytes of RAM (e.g., ATtiny85)
- **Dynamic nice** — optional kernel-managed scheduling that auto-tunes thread timing for best performance

---

## Table of Contents

- [Getting Started](#getting-started)
- [Quick Example](#quick-example)
- [How It Works](#how-it-works)
- [API Reference](#api-reference)
  - [Thread Lifecycle](#thread-lifecycle)
  - [Synchronization](#synchronization)
  - [IPC: Wait/Notify](#ipc-waitnotify)
  - [IPC: Queues](#ipc-queues)
  - [IPC: Send/Receive Data Pipes](#ipc-sendreceive-data-pipes)
  - [Broadcasting](#broadcasting)
- [Platform Porting](#platform-porting)
- [Examples](#examples)
- [Architecture & Design](#architecture--design)
- [Supported Platforms](#supported-platforms)
- [Changelog](#changelog)
- [License](#license)

---

## Getting Started

### Requirements

- C++11 or later
- `setjmp.h` support (available on virtually all C/C++ compilers)

### Installation

**Arduino**: Copy the `atomicx/` folder into your Arduino libraries directory, or use the Arduino IDE Library Manager.

**PlatformIO**: Add the library to your `lib/` directory.

**PC / Linux / macOS**: Include `atomicx.hpp` and compile `atomicx.cpp` alongside your project:

```bash
g++ -std=c++11 -I atomicx/ atomicx/atomicx.cpp main.cpp -o myapp
```

### Minimal Setup

1. Include the header
2. Implement two platform functions (`Atomicx_GetTick` and `Atomicx_SleepTick`)
3. Subclass `thread::atomicx`
4. Call `atomicx::Start()`

---

## Quick Example

```cpp
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "atomicx.hpp"

using namespace thread;

// --- Platform functions (user must implement) ---

atomicx_time Atomicx_GetTick(void) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (atomicx_time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void Atomicx_SleepTick(atomicx_time nSleep) {
    usleep((useconds_t)nSleep * 1000);
}

// --- Thread with fixed stack ---

class Blinker : public atomicx {
public:
    Blinker() : atomicx(stack) { SetNice(500); }

    void run() noexcept override {
        int count = 0;
        while (Yield()) {
            std::cout << "Blink " << ++count << std::endl;
        }
    }

    void StackOverflowHandler() noexcept override {
        std::cerr << "Stack overflow in Blinker!" << std::endl;
    }

    const char* GetName() override { return "Blinker"; }

private:
    uint8_t stack[512] = "";
};

// --- Thread with self-managed (auto) stack ---

class Counter : public atomicx {
public:
    Counter() : atomicx(128, 64) { SetNice(1000); }

    void run() noexcept override {
        int n = 0;
        while (Yield()) {
            std::cout << "Count " << ++n << std::endl;
        }
    }

    void StackOverflowHandler() noexcept override {
        std::cerr << "Stack overflow in Counter!" << std::endl;
    }

    const char* GetName() override { return "Counter"; }
};

int main() {
    Blinker b;
    Counter c;
    atomicx::Start();  // blocks here, running all threads cooperatively
}
```

---

## How It Works

AtomicX implements **stackful cooperative coroutines**:

1. **Construction** — When you instantiate a thread object, it automatically registers itself into a global intrusive doubly-linked list. No manual registration needed.
2. **`Start()`** — Enters the kernel loop. The scheduler picks the next thread and either calls `run()` (first time) or restores its context.
3. **`Yield()`** — The running thread saves its stack segment via `memcpy`, saves its CPU context via `setjmp`, and jumps back to the scheduler via `longjmp`.
4. **Resume** — The scheduler restores the stack segment and jumps into the thread's saved context. Execution continues right after `Yield()`.
5. **Destruction** — When the thread object is destroyed, it automatically removes itself from the scheduler's list.

```
Thread A          Scheduler          Thread B
   │                  │                  │
   │── Yield() ──────>│                  │
   │  [save stack]    │                  │
   │  [setjmp+longjmp]│                  │
   │                  │── resume ────────>│
   │                  │  [restore stack] │
   │                  │  [longjmp]       │
   │                  │                  │── runs...
   │                  │<── Yield() ──────│
   │<── resume ───────│                  │
   │── runs...        │                  │
```

> **No preemption.** Threads must call `Yield()` (or `Wait()`, or any blocking IPC call) to give control back to the scheduler. This makes all code between yields **atomic** with respect to other AtomicX threads.

For comprehensive architecture details, see [`design.md`](design.md).

---

## API Reference

### Thread Lifecycle

#### Creating a Thread

Subclass `atomicx` and implement the required virtual methods:

```cpp
class MyThread : public thread::atomicx {
public:
    // Fixed stack: provide a buffer
    MyThread() : atomicx(stack) { SetNice(100); }

    // OR self-managed stack: initial size + increase pace
    // MyThread() : atomicx(256, 32) { SetNice(100); }

    void run() noexcept override {
        // Your thread logic. Call Yield() periodically.
        while (Yield()) {
            // do work
        }
    }

    void StackOverflowHandler() noexcept override {
        // Called when stack exceeds buffer (and auto-resize fails)
    }

    // Optional overrides:
    const char* GetName() override { return "MyThread"; }
    void finish() noexcept override { /* cleanup after run() returns */ }

private:
    uint8_t stack[512] = "";
};
```

#### Key Methods

| Method | Description |
|--------|-------------|
| `atomicx::Start()` | **Static.** Enters the kernel loop — blocks until all threads finish or deadlock |
| `Yield(nSleep)` | Context switch. Default sleep = thread's nice value. Pass `0` for immediate return |
| `YieldNow()` | High-priority yield — this thread gets picked up before normal sleepers |
| `SetNice(ms)` | Set the default sleep interval between yields (in tick units) |
| `SetDynamicNice(true)` | Let the kernel auto-tune nice based on actual execution time |
| `Stop()` / `Resume()` | Suspend / resume the thread |
| `Restart()` | Calls `finish()` and re-enters `run()` from the beginning |
| `Detach()` | Calls `finish()`, removes thread from scheduler permanently |
| `GetID()` | Returns the thread's unique ID (its memory address) |
| `GetName()` | Returns the thread name (override to customize) |
| `GetStackSize()` | Allocated stack buffer size |
| `GetUsedStackSize()` | Actual stack usage from last context switch |
| `IsStackSelfManaged()` | `true` if using auto-stack mode |
| `GetStatus()` / `GetSubStatus()` | Current thread state (see state machine in [design.md](design.md)) |
| `GetCurrentTick()` | Returns the current tick via `Atomicx_GetTick()` |
| `GetLastUserExecTime()` | How long the thread ran during its last time slice |
| `GetThreadCount()` | Number of active threads in the system |
| `IsKernelRunning()` | `true` if `Start()` is currently executing |

#### Iterating All Threads

```cpp
for (auto& th : *atomicx::GetCurrent()) {
    std::cout << th.GetName() << " stack: " << th.GetUsedStackSize()
              << "/" << th.GetStackSize() << std::endl;
}
```

### Synchronization

#### Semaphore

```cpp
atomicx::semaphore sem(3);  // max 3 concurrent acquisitions

// In thread:
if (sem.acquire(1000)) {   // wait up to 1000 ticks
    // critical section
    sem.release();
}

// RAII version:
atomicx::smartSemaphore ss(sem);
if (ss.acquire()) {
    // auto-released when ss goes out of scope
}
```

| Method | Description |
|--------|-------------|
| `semaphore(maxShared)` | Create with max concurrent locks |
| `acquire(timeout)` | Acquire a slot (0 = wait forever) |
| `release()` | Release one slot |
| `GetCount()` | Current acquired count |
| `GetWaitCount()` | Threads waiting to acquire |

#### Mutex (Read-Write Lock)

```cpp
atomicx::mutex mtx;

// Exclusive lock:
if (mtx.Lock(1000)) {      // timeout optional
    // only this thread has access
    mtx.Unlock();
}

// Shared lock (multiple readers):
if (mtx.SharedLock()) {
    // read-only access, other shared locks allowed
    mtx.SharedUnlock();
}

// RAII version:
atomicx::smartMutex sm(mtx);
if (sm.Lock()) {
    // auto-unlocked when sm is destroyed
}
```

### IPC: Wait/Notify

Any variable's address can be used as a synchronization point. The `tag` parameter adds a channel/meaning layer.

```cpp
int mySignal;  // the variable itself is just an anchor — its value doesn't matter

// Thread A (consumer): blocks until notified
size_t message;
if (Wait(message, mySignal, /*tag=*/1, /*timeout=*/5000)) {
    // received notification with message
}

// Thread B (producer): wakes up Thread A
size_t payload = 42;
Notify(payload, mySignal, /*tag=*/1);
```

| Method | Description |
|--------|-------------|
| `Wait(msg, ref, tag, timeout)` | Block until notified. Returns message via `msg` |
| `Wait(ref, tag, timeout)` | Block until notified (no message) |
| `WaitAny(msg, ref, tag, timeout)` | Wait for any tag on `ref`. Returns the actual tag |
| `Notify(ref, tag)` | Wake one waiting thread + yield |
| `Notify(msg, ref, tag)` | Wake one + send message + yield |
| `SafeNotify(ref, tag)` | Wake one thread, **no** yield (use in ISR-like contexts) |
| `SyncNotify(msg, ref, tag, timeout)` | Wait until a waiter exists, then notify |
| `LookForWaitings(ref, tag, timeout)` | Block until someone is waiting on ref+tag |
| `HasWaitings(ref, tag)` | Count of threads waiting on ref+tag |
| `IsWaiting(ref, tag)` | `true` if at least one waiter exists |

### IPC: Queues

Thread-safe, blocking queue built on Wait/Notify:

```cpp
atomicx::queue<int> q(10);  // capacity of 10

// Producer thread:
q.PushBack(42);       // blocks if full
q.PushFront(99);      // push to front

// Consumer thread:
int val = q.Pop();    // blocks if empty

q.GetSize();          // current item count
q.IsFull();           // true if at capacity
```

### IPC: Send/Receive Data Pipes

Transfer arbitrary binary data between threads. Built on top of SyncNotify/WaitAny:

```cpp
struct SensorData { float temp; float humidity; };

int channel;  // any variable as reference anchor

// Sender thread:
SensorData data = {23.5f, 65.0f};
uint16_t sent = Send(channel, (uint8_t*)&data, sizeof(data), Timeout(5000));

// Receiver thread:
SensorData buf;
uint16_t received = Receive(channel, (uint8_t*)&buf, sizeof(buf), Timeout(5000));
```

### Broadcasting

Send messages to all threads that have opted in:

```cpp
// In your thread class:
class MyThread : public atomicx {
    MyThread() : atomicx(stack) {
        SetReceiveBroadcast(true);  // opt in
    }

    void BroadcastHandler(const size_t& ref, const Message& msg) override {
        // handle broadcast: ref, msg.message, msg.tag
    }
};

// From any thread:
BroadcastMessage(SIGNAL_TYPE, {payload, tag});
```

---

## Platform Porting

To run AtomicX on any platform, implement these two `extern "C"` functions:

```cpp
// Return the current time in your chosen tick unit (ms, us, etc.)
atomicx_time Atomicx_GetTick(void);

// Sleep/idle for nSleep ticks — opportunity for power saving
void Atomicx_SleepTick(atomicx_time nSleep);
```

### Arduino Example

```cpp
atomicx_time Atomicx_GetTick(void) {
    return (atomicx_time)millis();
}

void Atomicx_SleepTick(atomicx_time nSleep) {
    delay(nSleep);
}
```

### POSIX (Linux / macOS) Example

```cpp
atomicx_time Atomicx_GetTick(void) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (atomicx_time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void Atomicx_SleepTick(atomicx_time nSleep) {
    usleep((useconds_t)nSleep * 1000);
}
```

### ESP32 with Power Saving

```cpp
atomicx_time Atomicx_GetTick(void) {
    return (atomicx_time)millis();
}

void Atomicx_SleepTick(atomicx_time nSleep) {
    esp_sleep_enable_timer_wakeup(nSleep * 1000);  // light sleep
    esp_light_sleep_start();
}
```

> The `Atomicx_SleepTick` function is called by the scheduler when no thread is ready to run. Use it to reduce power consumption on battery-powered devices.

---

## Examples

### PC

| Example | Description |
|---------|-------------|
| [`examples/pc/simple`](examples/pc/simple) | Basic threads with fixed and self-managed stacks |
| [`examples/pc/semaphore`](examples/pc/semaphore) | Semaphore usage with Send/Receive data pipes |

### Arduino

| Example | Description |
|---------|-------------|
| [`examples/Arduino/simple`](examples/Arduino/simple) | Minimal thread example |
| [`examples/Arduino/semaphore`](examples/Arduino/semaphore) | Counting semaphore demo |
| [`examples/Arduino/sharedlock`](examples/Arduino/sharedlock) | Read-write mutex (shared lock) |
| [`examples/Arduino/send_receive`](examples/Arduino/send_receive) | Data pipe transfer between threads |
| [`examples/Arduino/watchdog`](examples/Arduino/watchdog) | Watchdog pattern using thread monitoring |
| [`examples/Arduino/DotMatrix`](examples/Arduino/DotMatrix) | Full project: LED matrix scroller with Serial/Telnet terminals, UDP trap, and logging (ESP8266) |
| [`examples/Arduino/ThermalCameraDemo`](examples/Arduino/ThermalCameraDemo) | Thermal camera display |
| [`examples/Arduino/avrAutoRobotController`](examples/Arduino/avrAutoRobotController) | Robot controller with IPC motor commands |

---

## Architecture & Design

For detailed architecture diagrams, class relationships, state machines, stack management internals, and the intrusive object controller design, see **[`design.md`](design.md)**.

### Key Design Decisions

- **Cooperative, not preemptive** — deterministic behavior, no race conditions between AtomicX threads, no need for critical sections
- **Intrusive linked list** — zero-allocation thread management; threads register/unregister themselves on construction/destruction
- **`setjmp`/`longjmp` context switch** — portable across all C compilers, no assembly required
- **Stack save/restore via `memcpy`** — threads use the real C stack during execution, only backing up the used portion on yield
- **All synchronization built on Wait/Notify** — semaphores, mutexes, and queues are layered on top of a single primitive, keeping the core small

---

## Supported Platforms

| Platform | Tested | Notes |
|----------|--------|-------|
| Arduino AVR (Uno, Mega, ATtiny85) | Yes | Fixed stack recommended |
| ESP8266 | Yes | Full featured, see DotMatrix example |
| ESP32 | Yes | Single-core cooperative context |
| STM32 | Yes | Via Arduino core or bare-metal |
| Linux / macOS (POSIX) | Yes | Great for development and testing |
| Any C++11 with `setjmp.h` | Should work | Implement the two platform functions |

---

## Changelog

### Version 1.3.0

- **Send/Receive data pipes** — transfer binary data between threads using `Send()` and `Receive()`, enabling client/server patterns inside embedded applications
- **WaitAny** — extends Wait/Notify to receive any tag, returning the actual tag by reference
- **Broadcasting** — replaced the old Broker with `BroadcastMessage()` + `BroadcastHandler()` for async thread-to-thread signaling
- **Mutex/Semaphore timeouts** — `Lock(timeout)` and `SharedLock(timeout)` now accept optional timeouts (fully backward compatible)
- **DotMatrix project** — full ESP8266 example with Serial/Telnet terminals, UDP trap, and a logging API

### Version 1.2.1

- **Dynamic Nice** — kernel auto-tunes thread timing via `SetDynamicNice(true)`
- **`YieldNow()`** — high-priority context switch for time-sensitive threads
- **`smartSemaphore`** and **`smartMutex`** — RAII wrappers for automatic resource release
- **Semaphores** — `atomicx::semaphore` with `acquire()`/`release()` and optional timeout
- **`Timeout` utility class** — `IsTimedout()`, `GetRemaining()`, `GetDurationSince()`
- **Renamed** `atomicx::lock` to `atomicx::mutex` for consistency
- **Stack increase pace** — configurable growth rate for self-managed stacks

### Version 1.2.0

- **Self-managed stack** — use `atomicx()` default constructor for automatic stack memory management
- New examples: Arduino/Simple, avrAutoRobotController

### Version 1.1.3

- Thermal Camera Demo
- Custom `subType` parameter for Wait/Notify (enables layered notification channels)

### Version 1.1.2

- Split `Notify` into `Notify` and `SyncNotify` to resolve compilation ambiguity on some boards

### Version 1.1.1

- `SyncNotify` — Notify waits for a matching `Wait` to be available before sending
- `avrRobotController` simulator with terminal interface

### Version 1.1.0

- `finish()` callback after `run()` returns (enables self-destroying eventual threads)
- `smartMutex` RAII compliance
- **Timed Wait/Notify** — real state-blocking with timeout, no spin locks
- `LookForWaitings` — block until a waiter appears for a given ref+tag
- Tag value `0` matches all tags (wildcard)

### Version 1.0.0

- Initial release
- Cooperative threading with `setjmp`/`longjmp` context switching
- Zero stack displacement — threads use the full C stack
- Wait/Notify IPC with message and tag
- Thread-safe queues
- Read-write mutex (Lock / SharedLock)
- Safe notification variants (no context switch) for use in interrupt-like contexts

---

## Important Notes

- **Stack memory is protected** — each thread's stack context is isolated. Do not pass stack pointers between threads. Use global variables, heap allocations, `smart_ptr`, queues, or `Send`/`Receive` instead.
- **No spin locks** — all blocking operations use real kernel-level state blocking via the scheduler. Waiting threads consume zero CPU.
- **Cooperative discipline** — you must call `Yield()` (or any blocking API) regularly. A thread that never yields will starve all others.

---

## License

[MIT License](LICENSE) - Copyright (c) 2022 Gustavo Campos
