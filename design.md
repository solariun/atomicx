# AtomicX v1.3.0 - Design Document

## 1. Overview

AtomicX is a **cooperative multitasking library** designed for embedded systems and microcontrollers. Instead of relying on OS-level threads or hardware interrupts for preemption, it implements **stackful coroutines** using `setjmp`/`longjmp` for context switching and `memcpy` for stack save/restore. This makes it portable to bare-metal environments with no OS support.

All components live within the `thread` namespace and the central class is `thread::atomicx`.

---

## 2. Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         USER APPLICATION                                │
│                                                                         │
│   class MyThread : public thread::atomicx {                            │
│       void run() noexcept override { /* user logic, Yield() */ }       │
│       void StackOverflowHandler() noexcept override { /* ... */ }      │
│   };                                                                    │
│                                                                         │
│   main() {                                                              │
│       MyThread t1, t2, t3;      // auto-registered in global list      │
│       atomicx::Start();          // enters kernel loop                  │
│   }                                                                     │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │  inherits
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                       thread::atomicx (Core)                            │
│                                                                         │
│  ┌─────────────┐  ┌────────────────┐  ┌──────────────────────────────┐ │
│  │  Scheduler   │  │ Context Switch │  │  Intrusive Linked List       │ │
│  │              │  │                │  │                              │ │
│  │ Start()      │  │ setjmp/longjmp │  │ ms_paFirst ──► th1 ──► th2  │ │
│  │ SelectNext() │  │ memcpy stack   │  │ ms_paLast  ──► thN          │ │
│  │ Yield()      │  │ save/restore   │  │ ms_pCurrent──► (running)    │ │
│  └─────────────┘  └────────────────┘  └──────────────────────────────┘ │
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │              IPC / Synchronization Primitives                    │   │
│  │                                                                 │   │
│  │  Wait / Notify / SyncNotify    semaphore    mutex               │   │
│  │  SafeNotify / WaitAny          smartSemaphore  smartMutex       │   │
│  │  LookForWaitings               queue<T>     Send / Receive      │   │
│  │  BroadcastMessage                                               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  ┌───────────────────────────────────┐  ┌──────────────────────────┐   │
│  │  Utilities                        │  │  Stack Management        │   │
│  │  Timeout, smart_ptr<T>, crc16     │  │  Fixed / Auto (malloc)   │   │
│  │  iterator<T>                      │  │  Auto-resize on overflow │   │
│  └───────────────────────────────────┘  └──────────────────────────┘   │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │ calls
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     Platform Abstraction Layer (PAL)                     │
│                                                                         │
│   extern atomicx_time Atomicx_GetTick(void);   // user must implement  │
│   extern void Atomicx_SleepTick(atomicx_time);  // user must implement │
│                                                                         │
│   e.g. millis() on Arduino, gettimeofday() on POSIX                    │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Class Diagram

```
thread namespace
═══════════════════════════════════════════════════════════════════════════

  ┌──────────────────────┐       ┌──────────────────────────┐
  │  iterator<T>         │       │  LinkItem<T>             │
  │──────────────────────│       │──────────────────────────│
  │ - m_ptr: T*          │       │ - next: LinkItem<T>*     │
  │──────────────────────│       │ - prev: LinkItem<T>*     │
  │ + operator*(): T&    │       │──────────────────────────│
  │ + operator->(): T*   │       │ + operator()(): T&       │
  │ + operator++()       │       │ + operator++(): T*       │
  │ + operator==/!=      │       └──────────────────────────┘
  └──────────────────────┘                   △
                                             │ extends
  ┌──────────────────────┐                   │
  │  LinkList<T>         │       (used by user-defined objects to
  │──────────────────────│        become part of an intrusive list)
  │ - current: LinkItem* │
  │ - first: LinkItem*   │
  │ - last: LinkItem*    │
  │──────────────────────│
  │ + AttachBack()       │
  │ + Detach()           │
  │ + begin()/end()      │
  └──────────────────────┘


  ┌═══════════════════════════════════════════════════════════════════┐
  │                         atomicx  (abstract)                       │
  │═══════════════════════════════════════════════════════════════════│
  │  «static» ms_paFirst, ms_paLast: atomicx*                        │
  │  «static» ms_pCurrent: atomicx*                                   │
  │  «static» ms_joinContext: jmp_buf                                 │
  │  «static» ms_running: bool                                        │
  │───────────────────────────────────────────────────────────────────│
  │  m_paNext, m_paPrev: atomicx*          (intrusive linked list)    │
  │  m_context: jmp_buf                    (saved coroutine context)  │
  │  m_stack: volatile uint8_t*            (stack save buffer)        │
  │  m_stackSize, m_stacUsedkSize: size_t                             │
  │  m_pStaskStart, m_pStaskEnd: volatile uint8_t*                    │
  │  m_aStatus: aTypes                     (thread state machine)     │
  │  m_aSubStatus: aSubTypes                                          │
  │  m_nice, m_nTargetTime: atomicx_time                              │
  │  m_pLockId: uint8_t*                   (Wait/Notify ref pointer)  │
  │  m_lockMessage: Message                                           │
  │  m_flags: { autoStack, dynamicNice, broadcast, attached }         │
  │═══════════════════════════════════════════════════════════════════│
  │  «static» Start(): bool               ← kernel entry point       │
  │  «static» SelectNextThread(): bool     ← scheduler                │
  │  «static» GetCurrent(): atomicx*                                  │
  │  Yield(nSleep): bool                   ← context switch           │
  │  YieldNow(): void                                                 │
  │  Wait(refVar, nTag, ...): bool         ← block until notified     │
  │  Notify(refVar, nTag, ...): size_t     ← wake waiting threads     │
  │  SafeNotify(...): size_t               ← notify without yield     │
  │  SyncNotify(...): size_t               ← wait-then-notify         │
  │  Send()/Receive(): uint16_t            ← data pipe transfer       │
  │  BroadcastMessage(): size_t                                       │
  │  Stop()/Resume()/Restart()/Detach()                               │
  │───────────────────────────────────────────────────────────────────│
  │  «pure virtual» run(): void            ← user thread body         │
  │  «pure virtual» StackOverflowHandler(): void                      │
  │  «virtual» finish(): void              ← cleanup hook             │
  │  «virtual» GetName(): const char*                                 │
  │  «virtual» BroadcastHandler(): void                               │
  └══════════════════════════╤════════════════════════════════════════┘
                             │
          ┌──────────────────┼───────────────────────┐
          │                  │                       │
  ┌───────┴───────┐  ┌──────┴────────┐  ┌──────────┴────────────┐
  │  Timeout      │  │  semaphore    │  │  mutex                 │
  │───────────────│  │───────────────│  │────────────────────────│
  │ m_timeoutValue│  │ m_counter     │  │ nSharedLockCount       │
  │───────────────│  │ m_maxShared   │  │ bExclusiveLock         │
  │ Set()         │  │───────────────│  │────────────────────────│
  │ IsTimedout()  │  │ acquire()     │  │ Lock() / Unlock()      │
  │ GetRemaining()│  │ release()     │  │ SharedLock()           │
  └───────────────┘  │ GetCount()    │  │ SharedUnlock()         │
                     └──────┬────────┘  └───────────┬────────────┘
                            │                       │
                    ┌───────┴─────────┐    ┌────────┴──────────┐
                    │ smartSemaphore  │    │ smartMutex (RAII)  │
                    │ (RAII wrapper)  │    │────────────────────│
                    │─────────────────│    │ m_lock: mutex&     │
                    │ m_sem: sem&     │    │ m_lockType: char   │
                    │ bAcquired: bool │    │────────────────────│
                    │─────────────────│    │ Lock()             │
                    │ acquire()       │    │ SharedLock()       │
                    │ release()       │    │ ~smartMutex()      │
                    │ ~smartSemaphore │    └────────────────────┘
                    └─────────────────┘

  ┌────────────────────────────┐   ┌──────────────────────────────┐
  │  smart_ptr<T>              │   │  queue<T>                    │
  │────────────────────────────│   │──────────────────────────────│
  │ pRef: reference*           │   │ m_nQSize: size_t             │
  │  └ reference {T*, nRC}     │   │ m_nItens: size_t             │
  │────────────────────────────│   │ m_pQIStart, m_pQIEnd: QItem* │
  │ operator->(): T*           │   │──────────────────────────────│
  │ operator&(): T&            │   │ PushBack(item): bool         │
  │ IsValid(): bool            │   │ PushFront(item): bool        │
  │ GetRefCounter(): size_t    │   │ Pop(): T                     │
  │ ~smart_ptr() ref-counted   │   │ GetSize(), IsFull()          │
  └────────────────────────────┘   │  ┌─────────────────────┐     │
                                   │  │ QItem (inner)       │     │
                                   │  │ m_qItem: T          │     │
                                   │  │ m_pNext: QItem*     │     │
                                   │  └─────────────────────┘     │
                                   └──────────────────────────────┘
```

---

## 4. State Machine: Thread Status (`aTypes`)

```
                         ┌─────────────────┐
                         │   CONSTRUCTION   │
                         │  AddThisThread() │
                         └────────┬────────┘
                                  │ m_aStatus = start
                                  ▼
                     ┌────────────────────────┐
         ┌──────────│        start (1)        │◄──────── Restart()
         │          └────────────┬────────────┘
         │                       │ Start() calls run()
         │                       ▼
         │          ┌────────────────────────┐
         │     ┌───►│      running (5)       │◄──────┐
         │     │    └──┬──────┬──────┬───────┘       │
         │     │       │      │      │               │
         │     │  Yield()  Wait()  Stop()            │
         │     │       │      │      │               │
         │     │       ▼      │      ▼               │
         │     │  ┌────────┐  │  ┌──────────┐        │
         │     │  │sleep   │  │  │ stop (10)│        │
         │     │  │ (100)  │  │  └────┬─────┘        │
         │     │  └───┬────┘  │       │ Resume()     │
         │     │      │       │       └──────────────┘
         │     │      │       ▼
         │     │      │  ┌────────────┐      Notify()
         │     │      │  │ wait (55)  │─────────────►now
         │     │      │  └──────┬─────┘              │
         │     │      │         │ timeout             │
         │     │      │         ▼                     │
         │     │      │  ┌────────────┐               │
         │     │      │  │  timeout   │               │
         │     │      │  │(subStatus) │               │
         │     │      │  └─────┬──────┘               │
         │     │      │        │                      │
         │     │      └────────┤                      │
         │     │               ▼                      │
         │     │    ┌──────────────────┐              │
         │     └────│    now (6)       │◄─────────────┘
         │          │ (high priority)  │
         │          └──────────────────┘
         │
         │  run() returns
         ▼
  ┌──────────────────┐         ┌───────────────────────┐
  │  finish() called │────────►│  start (restart loop) │
  └──────────────────┘         └───────────────────────┘
         │
         │ Detach() / ~atomicx()
         ▼
  ┌──────────────────┐
  │ RemoveThisThread │
  │  (unlinked)      │
  └──────────────────┘

  Special:
  ┌────────────────────────┐
  │  stackOverflow (255)   │ ── triggered when stack exceeds buffer
  │  → StackOverflowHandler│      and autoStack resize fails
  │  → abort()             │
  └────────────────────────┘
```

**Sub-statuses (`aSubTypes`):**
| Value     | Meaning                                         |
|-----------|--------------------------------------------------|
| `none`    | No sub-status                                    |
| `ok`      | Normal operation                                 |
| `error`   | Error condition                                  |
| `look`    | Waiting in `LookForWaitings` (sync-notify phase) |
| `wait`    | Waiting in `Wait()` for a notification           |
| `timeout` | The wait timed out                               |

---

## 5. Intrusive Object Controller (Thread Linked List)

AtomicX uses an **intrusive doubly-linked list** to manage all thread instances. "Intrusive" means the list pointers (`m_paNext`, `m_paPrev`) are embedded directly inside each `atomicx` object — no external container or heap allocation is needed.

### 5.1 Structure

```
  Static globals (in atomicx.cpp):
  ┌──────────────────────────────┐
  │ ms_paFirst ─────────────┐    │
  │ ms_paLast  ──────────┐  │    │
  │ ms_pCurrent ──┐      │  │    │
  └───────────────┼──────┼──┼────┘
                  │      │  │
                  ▼      │  ▼
              ┌──────┐   │ ┌──────┐      ┌──────┐      ┌──────┐
  nullptr ◄───│ prev │   │ │ prev │◄─────│ prev │◄─────│ prev │
              │ th_A │   │ │ th_B │      │ th_C │      │ th_D │
              │ next │───┼►│ next │─────►│ next │─────►│ next │──► nullptr
              └──────┘   │ └──────┘      └──────┘      └──────┘
                         │                                △
                         └────────────────────────────────┘
                            ms_paLast points to th_D
```

### 5.2 Registration: `AddThisThread()`

Called automatically from every `atomicx` constructor. Appends `this` to the tail of the global list:

```cpp
void atomicx::AddThisThread() {
    if (ms_paFirst == nullptr) {      // list is empty
        ms_paFirst = this;
        ms_paLast = ms_paFirst;
    } else {                          // append at end
        this->m_paPrev = ms_paLast;
        ms_paLast->m_paNext = this;
        ms_paLast = this;
    }
}
```

**Effect:** The moment you instantiate a thread object (stack or heap), it is live in the scheduler's list — no manual `register()` call needed.

### 5.3 Unregistration: `RemoveThisThread()`

Called from `DestroyThread()` which is called by `~atomicx()` and `Detach()`. Handles four cases:

| Case | Condition | Action |
|------|-----------|--------|
| Only node | `next == null && prev == null` | Clear `ms_paFirst`, `ms_pCurrent` |
| Head node | `prev == null` | Advance `ms_paFirst` to next |
| Tail node | `next == null` | Retract `ms_paLast` to prev |
| Middle node | both non-null | Splice prev↔next together |

### 5.4 Iteration

The global list is iterable via `begin()`/`end()` returning `iterator<atomicx>`, enabling range-for:

```cpp
for (auto& th : *atomicx::GetCurrent()) {
    // th is each thread in the system
}
```

The `operator++` on `atomicx` returns `m_paNext`, allowing the iterator to walk the linked list.

### 5.5 Why Intrusive?

| Property | Intrusive List | `std::list` / external container |
|----------|---------------|----------------------------------|
| Heap allocation per node | **None** | One per node |
| Cache locality | Object + links co-located | Indirection via pointer |
| Suitable for bare-metal MCU | **Yes** | Often unavailable |
| Object knows its own position | **Yes** (can self-remove) | No |
| Thread auto-registers on construction | **Yes** | Manual insertion needed |

This design is critical for the embedded target: zero additional allocation, deterministic behavior, and automatic lifecycle management.

---

## 6. Thread Lifecycle — Detailed Flow

### 6.1 Construction Phase

```
  User code:                         Library internals:
  ─────────                          ──────────────────
  MyThread t1(nice, "name");    ──►  atomicx(stack)  or  atomicx(size, pace)
                                         │
                                         ├─ SetDefaultInitializations()
                                         │    ├─ m_flags.attached = true
                                         │    └─ AddThisThread()  ◄── linked into global list
                                         │
                                         ├─ (if auto-stack) m_flags.autoStack = true
                                         └─ m_aStatus = start
```

### 6.2 Kernel Entry: `Start()`

```
  atomicx::Start()
  │
  ├─ ms_running = true
  ├─ ms_pCurrent = ms_paFirst
  │
  └─ while (ms_running && SelectNextThread())     ◄── MAIN KERNEL LOOP
       │
       ├─ setjmp(ms_joinContext) ──► saves scheduler context (return point)
       │     │
       │     ├─ [first time, returns 0]
       │     │    │
       │     │    ├─ if status == start:
       │     │    │    ├─ mark m_pStaskStart (stack anchor)
       │     │    │    ├─ m_aStatus = running
       │     │    │    ├─ ──► ms_pCurrent->run()     ◄── USER CODE RUNS
       │     │    │    │      (run returns when thread ends)
       │     │    │    ├─ m_aStatus = start (allows restart)
       │     │    │    └─ finish()
       │     │    │
       │     │    └─ else (status == sleep/now/running):
       │     │         └─ longjmp(ms_pCurrent->m_context, 1)
       │     │              ──► jumps INTO the thread (resumes after Yield's setjmp)
       │     │
       │     └─ [returns 1 from longjmp] ◄── thread called Yield → longjmp(ms_joinContext,1)
       │          └─ loop continues → SelectNextThread() → next iteration
       │
       └─ ms_running = false (all threads blocked = deadlock)
```

### 6.3 Context Switch: `Yield()` (the core mechanism)

This is the heart of AtomicX. Every cooperative switch goes through `Yield()`:

```
  Thread code calls Yield(nSleep)
  │
  ├─ 1. Record execution time
  │     m_LastUserExecTime = now - m_lastResumeUserTime
  │
  ├─ 2. Set sleep/target time based on status
  │     ├─ running → sleep, target = now + nice (or explicit sleep)
  │     ├─ wait   → target = now + timeout (or 0 = indefinite)
  │     └─ other  → target = max
  │
  ├─ 3. Measure stack usage
  │     nStackEnd on current stack ──► m_stacUsedkSize = start - end + 1
  │
  ├─ 4. Check stack overflow
  │     ├─ if usedSize > stackSize:
  │     │    ├─ autoStack? → realloc (free old, malloc larger)
  │     │    └─ !autoStack? → StackOverflowHandler() → abort()
  │     └─ ok → continue
  │
  ├─ 5. SAVE stack to buffer
  │     memcpy(m_stack, stackEnd, usedSize)      ◄── snapshot of live stack
  │
  ├─ 6. Save thread context & jump to scheduler
  │     if (setjmp(m_context) == 0)               ◄── save "resume here" point
  │         longjmp(ms_joinContext, 1)             ──► BACK TO SCHEDULER
  │     else:
  │         ──► RESUMED BY SCHEDULER (longjmp into m_context)
  │
  └─ 7. RESTORE stack from buffer
        memcpy(stackEnd, m_stack, usedSize)       ◄── restore stack snapshot
        m_aStatus = running
        m_lastResumeUserTime = now
        return true  ──► execution continues in thread after Yield() call
```

**Visual timeline of a context switch:**

```
  ┌─ Thread A ─┐    ┌─ Scheduler ─┐    ┌─ Thread B ─┐
  │            │    │              │    │             │
  │  run() {   │    │              │    │  run() {    │
  │    ...     │    │              │    │    ...      │
  │    Yield()─┼───►│              │    │             │
  │   [save    │    │ SelectNext() │    │             │
  │    stack]  │    │  → picks B   │    │             │
  │   [setjmp] │    │              │    │             │
  │   [longjmp]┼───►│              │    │             │
  │            │    │ [longjmp B]──┼───►│             │
  │            │    │              │    │ [restore    │
  │            │    │              │    │  stack]     │
  │            │    │              │    │   ...       │
  │            │    │              │    │   Yield()───┼──►│ ...
  │            │    │              │    │             │
```

### 6.4 Scheduler: `SelectNextThread()`

The scheduler traverses the full thread list and picks the best candidate:

```
Priority rules (highest to lowest):
  1. status == start  → brand new thread, run immediately
  2. status == now    → YieldNow() or just-notified thread
  3. status == sleep with earliest m_nTargetTime
  4. status == wait with m_nTargetTime > 0 (timed wait, earliest wins)

Skipped:
  - status == stop   → suspended threads
  - status == wait with m_nTargetTime == 0 → indefinite wait (needs Notify)

If only indefinite waits remain → deadlock → Start() returns false
```

After selecting, if the chosen thread is sleeping/timed-wait, the scheduler calls `Atomicx_SleepTick(delta)` to idle the CPU until the target time.

**Dynamic nice:** When enabled (`SetDynamicNice(true)`), the scheduler auto-adjusts `m_nice` as a running average of actual execution times, achieving natural load balancing.

### 6.5 Destruction Phase

```
  ~atomicx() or Detach()
  │
  └─ DestroyThread()
       ├─ if (m_flags.attached):
       │    ├─ RemoveThisThread()    ◄── unlink from global list
       │    ├─ if autoStack && m_stack:
       │    │    └─ free(m_stack)     ◄── release stack buffer
       │    └─ m_flags.attached = false
       └─ done
```

---

## 7. Wait/Notify Mechanism

The Wait/Notify system is a **reference-pointer-based signaling mechanism**. Any variable's address can serve as a synchronization point.

### 7.1 How It Works

```
  Thread A: Wait(refVar, tag)          Thread B: Notify(refVar, tag)
  │                                     │
  ├─ m_pLockId = &refVar               ├─ scan all threads
  ├─ m_aStatus = wait                  ├─ find threads where:
  ├─ m_lockMessage.tag = tag           │    m_aStatus == wait
  ├─ Yield(timeout)                    │    m_pLockId == &refVar
  │   ... blocked ...                  │    tag matches
  │                                    ├─ set matched thread:
  │   ◄── notified ───────────────────┤    m_aStatus = now
  │                                    │    m_lockMessage = message
  ├─ check m_aSubStatus               ├─ Yield(0)  (trigger switch)
  │   timeout? → return false          │
  │   ok? → read m_lockMessage         │
  └─ return true                       └─
```

### 7.2 Notification Variants

| Method | Behavior |
|--------|----------|
| `Wait(refVar, tag)` | Block until notified on refVar+tag |
| `WaitAny(refVar, &tag)` | Block until any tag on refVar, returns the tag |
| `Notify(refVar, tag)` | Wake one/all waiters + yield |
| `SafeNotify(refVar, tag)` | Wake one/all waiters, NO yield |
| `SyncNotify(refVar, tag)` | Wait for waiters to exist, then notify |
| `LookForWaitings(refVar, tag)` | Block until someone is waiting on refVar+tag |

---

## 8. Synchronization Primitives — Relationship Diagram

```
                    ┌──────────────────────────────────┐
                    │       atomicx (core engine)       │
                    │                                    │
                    │   Wait()  Notify()  Yield()       │
                    └──────────┬───────────────────────┘
                               │ uses internally
              ┌────────────────┼────────────────────┐
              ▼                ▼                     ▼
       ┌─────────────┐  ┌──────────┐         ┌──────────┐
       │  semaphore   │  │  mutex   │         │ queue<T> │
       │─────────────│  │──────────│         │──────────│
       │ acquire() ──┼─►│Wait(this)│         │ PushBack │──►Wait/Notify
       │ release() ──┼─►│Notify()  │         │ Pop()    │──►Wait/Notify
       └──────┬──────┘  └────┬─────┘         └──────────┘
              │              │
         RAII wraps      RAII wraps
              │              │
              ▼              ▼
       ┌──────────────┐ ┌────────────┐
       │smartSemaphore│ │ smartMutex │
       │ ~destructor  │ │ ~destructor│
       │  auto-release│ │  auto-     │
       └──────────────┘ │  unlock    │
                        └────────────┘
```

All synchronization primitives are built **on top of** `Wait()`/`Notify()`. They don't use OS-level primitives — they leverage the cooperative scheduler to block and wake threads.

**mutex** supports two modes:
- **Exclusive Lock**: Only one thread holds it; others block on `Wait(bExclusiveLock, 1)`
- **Shared Lock**: Multiple readers allowed; exclusive lock waits for all shared locks to release via `Wait(nSharedLockCount, 2)`

---

## 9. Stack Management

AtomicX supports two stack modes:

### 9.1 Fixed Stack (user-provided buffer)

```cpp
class MyThread : public atomicx {
    uint8_t stack[512];  // embedded in the object
public:
    MyThread() : atomicx(stack) {}  // template deduces size
};
```

The buffer is used directly. If the thread's actual stack usage exceeds it → `StackOverflowHandler()` → `abort()`.

### 9.2 Auto-Stack (malloc-managed)

```cpp
class MyThread : public atomicx {
public:
    MyThread() : atomicx(256, 64) {}  // initial=256, increase_pace=64
};
```

```
  First Yield:
  ┌────────────────┐     used = 200 bytes
  │ malloc(256)    │ ◄── fits, stack saved normally
  └────────────────┘

  Later Yield:
  ┌────────────────┐     used = 300 bytes > 256
  │ free(old)      │
  │ malloc(300+64) │ ◄── resized to usedSize + pace = 364
  └────────────────┘

  Even later:                used = 400 > 364
  │ free(old)      │
  │ malloc(400+64) │ ◄── 464
```

The stack buffer is **not** the actual execution stack. It's a **save area**: on each `Yield()`, the live stack segment is `memcpy`'d into this buffer, and on resume it's `memcpy`'d back. The real execution happens on the C call stack.

---

## 10. Data Transfer: Send/Receive Pipe

A protocol built on top of `SyncNotify`/`WaitAny` for streaming binary data between threads:

```
  Sender                                Receiver
  ──────                                ────────
  SyncNotify(size, ref, START)    ──►   WaitAny() → sees START
                                        state = transfering

  loop:                                 loop:
    LookForWaitings(ref, XFER)          WaitAny() → sees XFER
    SyncNotify(chunk, ref, XFER)  ──►     memcpy chunk into buffer

  LookForWaitings(ref, XFER)
  SyncNotify(total, ref, END)     ──►   WaitAny() → sees END → break
```

Chunks are packed into `size_t` messages: byte 0 = length, remaining bytes = payload.

---

## 11. Platform Abstraction

Users must implement two `extern` functions:

| Function | Purpose | Example (POSIX) |
|----------|---------|-----------------|
| `Atomicx_GetTick()` | Return current time in tick units | `gettimeofday()` → ms |
| `Atomicx_SleepTick(n)` | Sleep for `n` ticks (power saving) | `usleep(n * 1000)` |

These decouple the library from any specific timer hardware, making it portable across Arduino, ESP32, STM32, Linux, etc.

---

## 12. Summary of Design Patterns Used

| Pattern | Where | Purpose |
|---------|-------|---------|
| **Intrusive Linked List** | Thread management | Zero-allocation thread registry |
| **Cooperative Scheduling** | `Yield()`/`Start()` | Deterministic, no preemption |
| **Template Method** | `run()`, `finish()`, `StackOverflowHandler()` | User extends behavior |
| **RAII** | `smartMutex`, `smartSemaphore` | Auto-release on scope exit |
| **Reference Counting** | `smart_ptr<T>` | Lightweight shared ownership |
| **Observer/Signal** | `Wait()`/`Notify()` via refVar | Decoupled IPC on any variable |
| **State Machine** | `aTypes`/`aSubTypes` | Thread lifecycle management |
| **Strategy** | `Atomicx_GetTick`/`SleepTick` | Platform-specific behavior injection |
