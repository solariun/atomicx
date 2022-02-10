//
//  atomic.hpp
//  atomic
//
//  Created by GUSTAVO CAMPOS on 29/08/2021.
//

#ifndef atomic_hpp
#define atomic_hpp

#include <stdint.h>
#include <stdlib.h>
#include <setjmp.h>

/* Official version */
#define ATOMICX_VERSION "1.2.1"
#define ATOMIC_VERSION_LABEL "AtomicX v" ATOMICX_VERSION " built at " __TIMESTAMP__

using atomicx_time = uint32_t;

#define ATOMICX_TIME_MAX ((atomicx_time) ~0)

extern "C"
{
    extern void yield(void);
}

/**
 * @brief Implement the custom Tick acquisition
 *
 * @return atomicx_time
 */
extern atomicx_time Atomicx_GetTick(void);

/**
 * @brief Implement a custom sleep, usually based in the same GetTick granularity
 *
 * @param nSleep    How long custom tick to wait
 *
 * @note This function is particularly special, since it give freedom to tweak the
 *       processor power consuption if necessary
 */
extern void Atomicx_SleepTick(atomicx_time nSleep);

namespace thread
{
    class atomicx
    {
    public:

        /**
         * ------------------------------
         * STATE MACHINE TYPES
         * ------------------------------
         */
        enum class aTypes : uint8_t
        {
            start=1,
            running=5,
            now=6,
            stop=10,
            lock=50,
            wait=55,
            subscription=60,
            sleep=100,
            stackOverflow=255
        };

        enum class aSubTypes : uint8_t
        {
            error=10,
            ok,
            look,
            wait,
            timeout
        };

        enum class NotifyType : uint8_t
        {
            one = 0,
            all = 1
        };

        /**
         * @brief Timeout Check object
         */

        class Timeout
        {
            public:
                Timeout () = delete;

                /**
                 * @brief Construct a new Timeout object
                 *
                 * @param nTimoutValue  Timeout value to be calculated
                 *
                 * @note    To decrease the amount of memory, Timeout does not save
                 *          the start time.
                 *          Special use case: if nTimeoutValue == 0, IsTimedout is always false.
                 */
                Timeout (atomicx_time nTimoutValue);

                /**
                 * @brief Set a timeout from now
                 *
                 * @param nTimoutValue timeout in atomicx_time
                 */
                void Set(atomicx_time nTimoutValue);

                /**
                 * @brief Check wether it has timeout
                 *
                 * @return true if it timeout otherwise 0
                 */
                bool IsTimedout();

                /**
                 * @brief Get the remaining time till timeout
                 *
                 * @return atomicx_time Remaining time till timeout, otherwise 0;
                 */
                atomicx_time GetRemaining();

                /**
                 * @brief Get the Time Since specific point in time
                 *
                 * @param startTime     The specific point in time
                 *
                 * @return atomicx_time How long since the point in time
                 *
                 * @note    To decrease the amount of memory, Timeout does not save
                 *          the start time.
                 */
                atomicx_time GetDurationSince(atomicx_time startTime);

            private:
                atomicx_time m_timeoutValue = 0;
        };
        /**
         * ------------------------------
         * ITERATOR FOR THREAD LISTING
         * ------------------------------
         */
        class aiterator
        {
        public:
            aiterator() = delete;

            /**
             * @brief atomicx based constructor
             *
             * @param ptr atomicx pointer to iterate
             */
            aiterator(atomicx* ptr);

            /*
             * Access operator
             */
            atomicx& operator*() const;
            atomicx* operator->();

            /*
             * Movement operator
             */
            aiterator& operator++();

            /*
             * Binary operators
             */
            friend bool operator== (const aiterator& a, const aiterator& b){ return a.m_ptr == b.m_ptr;};
            friend bool operator!= (const aiterator& a, const aiterator& b){ return a.m_ptr != b.m_ptr;};

        private:
            atomicx* m_ptr;
        };

        /**
         * @brief Get the beggining of the list
         *
         * @return aiterator
         */
        aiterator begin(void);

        /**
         * @brief Get the end of the list
         *
         * @return aiterator
         */
        aiterator end(void);

        /**
         * ------------------------------
         * SUPLEMENTAR SMART_PTR IMPLEMENTATION
         * ------------------------------
         */
        template <typename T> class smart_ptr
        {
        public:

            /**
             * @brief smart pointer constructor
             *
             * @param p pointer type T to be managed
             */
            smart_ptr(T* p) : pRef (new reference {p, 1})
            { }

            /**
             * @brief smart pointer overload constructor
             *
             * @param sa    Smart pointer reference
             */
            smart_ptr(const smart_ptr<T>& sa)
            {
                pRef = sa.pRef;
                pRef->nRC++;
            }

            /**
             * @brief Smart pointer Assignment operator
             *
             * @param sa    Smart poiter reference
             *
             * @return smart_ptr<T>&  smart pointer this reference.
             */
            smart_ptr<T>& operator=(const smart_ptr<T>& sa)
            {
                if (pRef != nullptr && pRef->nRC > 0)
                {
                    pRef->nRC--;
                }

                pRef = sa.pRef;

                if (pRef != nullptr)
                {
                    pRef->nRC++;
                }

                return *this;
            }

            /**
             * @brief Smart pointer destructor
             */
            ~smart_ptr(void)
            {
                if (pRef != nullptr)
                {
                    if (--pRef->nRC == 0)
                    {
                        delete pRef->pReference;
                        delete pRef;
                    }
                    else
                    {
                        pRef->nRC--;
                    }
                }
            }

            /**
             * @brief Smart pointer access operator
             *
             * @return T*  Pointer for the managed object T
             */
            T* operator-> (void)
            {
                return pRef->pReference;
            }

            /**
             * @brief Smart pointer access operator
             *
             * @return T*  Reference for the managed object T
             */
            T& operator& (void)
            {
                return *pRef->pReference;
            }

            /**
             * @brief  Check if the referece still valid
             *
             * @return true if the reference still not null, otherwise false
             */
            bool IsValid(void)
            {
                return pRef == nullptr ? false : pRef->pReference == nullptr ? false : true;
            }

            /**
             * @brief Get the Ref Counter of the managed pointer
             *
             * @return size_t How much active references
             */
            size_t GetRefCounter(void)
            {
                if (pRef != nullptr)
                {
                    return pRef->nRC;
                }

                return 0;
            }

        private:

            smart_ptr(void) = delete;
            struct reference
            {
                T* pReference ;
                size_t nRC;
            };

            reference* pRef=nullptr;
        };

        /**
         * ------------------------------
         * QUEUE FOR IPC IMPLEMENTATION
         * ------------------------------
         */

        template<typename T>
        class queue
        {
        public:

            queue() = delete;

            /**
             * @brief Thread Safe Queue constructor
             *
             * @param nQSize Max number of objects to hold
             */
            queue(size_t nQSize):m_nQSize(nQSize), m_nItens{0}
            {}

            /**
             * @brief Push an object to the end of the queue, if the queue
             *        is full, it waits till there is a space.
             *
             * @param item  The object to be pushed into the queue
             *
             * @return true if it was able to push a object in the queue, false otherwise
             */
            bool PushBack(T item)
            {
                if (m_nItens >= m_nQSize)
                {
                    if (atomicx::GetCurrent() != nullptr)
                    {
                        atomicx::GetCurrent()->Wait(*this,1);
                    }
                    else
                    {
                        return false;
                    }
                }

                QItem* pQItem = new QItem(item);

                if (m_pQIStart == nullptr)
                {
                    m_pQIStart = m_pQIEnd = pQItem;
                }
                else
                {
                    m_pQIEnd->SetNext(*pQItem);
                    m_pQIEnd = pQItem;
                }

                m_nItens++;

                if (atomicx::GetCurrent() != nullptr)
                {
                    atomicx::GetCurrent()->Notify(*this,0);
                }

                return true;
            }


            /**
             * @brief Push an object to the beggining of the queue, if the queue
             *        is full, it waits till there is a space.
             *
             * @param item  The object to be pushed into the queue
             *
             * @return true if it was able to push a object in the queue, false otherwise
             */
            bool PushFront(T item)
            {
                if (m_nItens >= m_nQSize)
                {
                    if (atomicx::GetCurrent() != nullptr)
                    {
                        atomicx::GetCurrent()->Wait(*this,1);
                    }
                    else
                    {
                        return false;
                    }
                }

                QItem* pQItem = new QItem(item);

                if (m_pQIStart == nullptr)
                {
                    m_pQIStart = m_pQIEnd = pQItem;
                }
                else
                {
                    pQItem->SetNext(*m_pQIStart);
                    m_pQIStart = pQItem;
                }

                m_nItens++;

                if (atomicx::GetCurrent() != nullptr)
                {
                    atomicx::GetCurrent()->Notify    (*this,0);
                }

                return true;
            }

            /**
             * @brief Pop an Item from the beggining of queue. Is no object there is no
             *        object in the queue, it waits for it.
             *
             * @return T return the object stored.
             */
            T Pop()
            {
                if (m_nItens == 0)
                {
                    atomicx::GetCurrent()->Wait(*this,0);
                }

                T pItem = m_pQIStart->GetItem();

                QItem* p_tmpQItem = m_pQIStart;

                m_pQIStart = m_pQIStart->GetNext();

                delete p_tmpQItem;

                m_nItens--;

                if (atomicx::GetCurrent() != nullptr)
                {
                    atomicx::GetCurrent()->Notify(*this,1);
                }

                return pItem;
            }

            /**
             * @brief Get the number of the objects in the queue
             *
             * @return size_t Number of the objects in the queue
             */
            size_t GetSize()
            {
                return m_nItens;
            }

            /**
             * @brief Get the Max number of object in the queue can hold
             *
             * @return size_t   The max number of object
             */
            size_t GetMaxSize()
            {
                return m_nQSize;
            }

            /**
             * @brief Check if the queue is full
             *
             * @return true for yes, otherwise false
             */
            bool IsFull()
            {
                return m_nItens >= m_nQSize;
            }

        protected:

            /**
             * @brief Queue Item object
             */
            class QItem
            {
            public:
                QItem () = delete;

                /**
                 * @brief Queue Item constructor
                 *
                 * @param qItem Obj template type T
                 */
                QItem(T& qItem) : m_qItem(qItem), m_pNext(nullptr)
                {}

                /**
                 * @brief Get the current object in the QItem
                 *
                 * @return T& The template type T object
                 */
                T& GetItem()
                {
                    return m_qItem;
                }

            protected:
                friend class queue;

                /**
                 * @brief Set Next Item in the Queue list
                 *
                 * @param qItem QItem that holds a Queue element
                 */
                void SetNext (QItem& qItem)
                {
                    m_pNext = &qItem;
                }

                /**
                 * @brief Get the Next QItem object, if any
                 *
                 * @return QItem* A valid QItem pointer otherwise nullptr
                 */
                QItem* GetNext ()
                {
                    return m_pNext;
                }

            private:

                T m_qItem;
                QItem* m_pNext;
            };

        private:
            size_t m_nQSize;
            size_t m_nItens;

            QItem* m_pQIEnd = nullptr;
            QItem* m_pQIStart = nullptr;

        };

        /**
         * --------------------------------
         * SEMAPHORES IMPLEMENTATION
         * --------------------------------
         */

        class semaphore
        {
            public:
                /**
                 * @brief Construct a new semaphore with MaxShared allowed
                 *
                 * @param nMaxShred     Max shared lock
                 */
                semaphore(size_t nMaxShared);

                /**
                 * @brief Acquire a shared lock context, if already on max shared allowed, wait till one is release or timeout
                 *
                 * @param nTimeout  default = 0 (indefinitely), How long to wait of accquiring
                 *
                 * @return true if it acquired the context, otherwise timeout returns false
                 */
                bool acquire(atomicx_time nTimeout = 0);

                /**
                 * @brief Releases one shared lock
                 */
                void release();

                /**
                 * @brief Get How many shared locks at a given moment
                 *
                 * @return size_t   Number of shared locks
                 */
                size_t GetCount();

                /**
                 * @brief Get how many waiting threads for accquiring context
                 *
                 * @return size_t   Number of waiting threads
                 */
                size_t GetWaitCount();

                /**
                 * @brief Get the Max Acquired Number
                 *
                 * @return size_t   The max acquired context number
                 */
                size_t GetMaxAcquired();

                /**
                 * @brief Get the maximun accquired context possible
                 *
                 * @return size_t
                 */
                static size_t GetMax ();

            private:
                size_t m_counter=0;
                size_t m_maxShared;
        };

        class smartSemaphore
        {
            public:
                /**
                 * @brief Acquire and managed the semaphore
                 *
                 * @param sem   base semaphore
                 */
                smartSemaphore (atomicx::semaphore& sem);
                smartSemaphore () = delete;
                /**
                 * @brief Destroy the smart Semaphore while releasing it
                 */
                ~smartSemaphore();

                /**
                 * @brief Acquire a shared lock context, if already on max shared allowed, wait till one is release or timeout
                 *
                 * @param nTimeout  default = 0 (indefinitely), How long to wait of accquiring
                 *
                 * @return true if it acquired the context, otherwise timeout returns false
                 */
                bool acquire(atomicx_time nTimeout = 0);

                /**
                 * @brief Releases one shared lock
                 */
                void release();

                /**
                 * @brief Get How many shared locks at a given moment
                 *
                 * @return size_t   Number of shared locks
                 */
                size_t GetCount();

                /**
                 * @brief Get how many waiting threads for accquiring context
                 *
                 * @return size_t   Number of waiting threads
                 */
                size_t GetWaitCount();

                /**
                 * @brief Get the Max Acquired Number
                 *
                 * @return size_t   The max acquired context number
                 */
                size_t GetMaxAcquired();

                /**
                 * @brief Get the maximun accquired context possible
                 *
                 * @return size_t
                 */
                static size_t GetMax ();

                /**
                 * @brief  Report if the smartSemaphore has acquired a shared context
                 *
                 * @return true if it has successfully acquired a shared context otherwise false
                 */
                bool IsAcquired ();

            private:
            semaphore& m_sem;
            bool bAcquired = false;
        };

        /**
         * ------------------------------
         * SMART LOCK IMPLEMENTATION
         * ------------------------------
         */

        /* The stamart mutex implementation */
        class mutex
        {
        public:
            /**
             * @brief Exclusive/binary lock the smart lock
             *
             * @note Once Lock() methos is called, if any thread held a shared lock,
             *       the Lock will wait for it to finish in order to acquire the exclusive
             *       lock, and all other threads that needs to a shared lock will wait till
             *       Lock is accquired and released.
             */
            bool Lock(atomicx_time ttimeout=0);

            /**
             * @brief Release the exclusive lock
             */
            void Unlock();

            /**
             * @brief Shared Lock for the smart Lock
             *
             * @note Shared lock can only be accquired if no Exclusive lock is waiting or already accquired a exclusive lock,
             *       In contrast, if at least one thread holds a shared lock, any exclusive lock can only be accquired once it
             *       is released.
             */
            bool SharedLock(atomicx_time ttimeout=0);

            /**
             * @brief Release the current shared lock
             */
            void SharedUnlock();

            /**
             * @brief Check how many shared locks are accquired
             *
             * @return size_t   Number of threads holding shared locks
             */
            size_t IsShared();

            /**
             * @brief Check if a exclusive lock has been already accquired
             *
             * @return true if yes, otherwise false
             */
            bool IsLocked();

        protected:
        private:
            size_t nSharedLockCount=0;
            bool bExclusiveLock=false;
        };

        /**
         * @brief RII compliance lock/shared lock to auto unlock on destruction
         *
         */
        class smartMutex
        {
            public:
                smartMutex() = delete;

                /**
                 * @brief Construct a new Smart Lock object based a existing lock
                 *
                 * @param lockObj the existing lock object
                 */
                smartMutex (mutex& lockObj);

                /**
                 * @brief Destroy and release the smart lock taken
                 */
                ~smartMutex();

                /**
                 * @brief Accquire a SharedLock
                 *
                 * @return true if accquired, false if another accquisition was already done
                 */
                bool SharedLock(atomicx_time ttimeout=0);

                /**
                 * @brief Accquire a exclusive Lock
                 *
                 * @return true if accquired, false if another accquisition was already done
                 */
                bool Lock(atomicx_time ttimeout=0);

                /**
                 * @brief Check how many shared locks are accquired
                 *
                 * @return size_t   Number of threads holding shared locks
                 */
                size_t IsShared();

                /**
                 * @brief Check if a exclusive lock has been already accquired
                 *
                 * @return true if yes, otherwise false
                 */
                bool IsLocked();

            private:

            mutex& m_lock;
            uint8_t m_lockType = '\0';
        };

        /**
         * PUBLIC OBJECT METHOS
         */

        /**
         * @brief virtual destructor of the atomicx
         */
        virtual ~atomicx(void);

        /**
         * @brief Get the Current thread in execution
         *
         * @return atomicx* thread
         */
        static atomicx* GetCurrent();


        /**
         * @brief Get the Thread pointer from a object
         * 
         * @param threadId  The thread ID reference
         * 
         * @return atomicx* nullprt if not found otherwise the pointer to the thread
         */
        static atomicx* GetThread(size_t threadId);

        /**
         * @brief Once it is call the process blocks execution and start all threads
         *
         * @return false if it was destried by dead lock (all threads locked)
         */
        static bool Start(void);

        /**
         * @brief Get the current thread ID
         *
         * @return size_t  Thread ID number
         */
        size_t GetID(void);

        /**
         * @brief Get the Max Stack Size for the thread
         *
         * @return size_t size in bytes
         */
        size_t GetStackSize(void);

        /**
         * @brief Get the Nice the current thread
         *
         * @return  atomicx_time the number representing the nice and based on the ported tick
         *          granularity.
         */
        atomicx_time GetNice(void);

        /**
         * @brief Get the Used Stack Size for the thread since the last context change cycle
         *
         * @return size_t size in bytes
         */
        size_t GetUsedStackSize(void);

        /**
         * @brief Get the Current Tick using the ported tick granularity function
         *
         * @return atomicx_time based on the ported tick granularity
         */
        atomicx_time GetCurrentTick(void);

        /**
         * @brief Get the Name object
         *
         * @return const char* name in plain c string
         *
         * @name If GetName was not overloaded by the derived thread implementation
         *       a standard name will be returned.
         */
        virtual const char* GetName(void);

        /**
         * @brief Get next moment in ported tick granularity the thread will be due to return
         *
         * @return atomicx_time based on the ported tick granularity
         */
        atomicx_time GetTargetTime(void);

        /**
         * @brief Get the current thread status
         *
         * @return int use atomicx::aTypes
         */
        int GetStatus(void);

        /**
         * @brief Get the current thread sub status
         *
         * @return int use atomicx::aTypes
         */
        int GetSubStatus(void);

        /**
         * @brief Get the Reference Lock last used to lock the thread
         *
         * @return size_t the lock_id (used my wait)
         */
        size_t GetReferenceLock(void);

        /**
         * @brief Get the last tag message posted
         *
         * @return size_t atomicx::message::tag value
         */
        size_t GetTagLock(void);

        /**
         * @brief Set the Nice of the thread
         *
         * @param nice in atomicx_time reference based on the ported tick granularity
         */
        void SetNice (atomicx_time nice);

        /**
         * @brief Construct a new atomicx thread
         *
         * @tparam T Stack memory page type
         * @tparam N Stack memory page size
         */
        template<typename T, size_t N> atomicx(T (&stack)[N]) : m_context{}, m_stackSize{N}, m_stack((volatile uint8_t*) stack)
        {
           SetDefaultParameters();

            AddThisThread();
        }

        /**
         * @brief Construct a new atomicx object and set initial auto stack and increase pace
         *
         * @param nStackSize            Initial Size of the stack
         * @param nStackIncreasePace    defalt=1, The increase pace on each resize
         */
        atomicx(size_t nStackSize=0, int nStackIncreasePace=1);

        /**
         * @brief The pure virtual function that runs the thread loop
         *
         * @note REQUIRED implementation and once it returns it will execute finish method
         */
        virtual void run(void) noexcept = 0;

        /**
         * @brief Handles the StackOverflow of the current thread
         *
         * @note REQUIRED
         */
        virtual void StackOverflowHandler(void) noexcept = 0;

        /**
         * @brief Called right after run returns, can be used to self-destroy the object and other maintenance actions
         *
         * @note if not implemented a default "empty" call is used instead
         */
        virtual void finish() noexcept
        {
            return;
        }

        /**
         * @brief Return if the current thread's stack memory is automatic
         */
        bool IsStackSelfManaged(void);

        /**
         * @brief Foce the context change explicitly
         *
         * @param nSleep  default is ATOMICX_TIME_MAX, otherwise it will override the nice and sleep for n custom tick granularity
         *
         * @return true if the context came back correctly, otherwise false
         */
        bool Yield(atomicx_time nSleep=ATOMICX_TIME_MAX);

        /**
         * @brief Get the Last Execution of User Code
         *
         * @return atomicx_time
         */
        atomicx_time GetLastUserExecTime();

        /**
         * @brief Get the Stack Increase Pace value
         */
        size_t GetStackIncreasePace(void);

         /**
         * @brief Trigger a high priority NOW, caution it will always execute before normal yield.
         */
        void YieldNow (void);

        /**
         * @brief Set the Dynamic Nice on and off
         *
         * @param status    True for on otherwsize off
         */
        void SetDynamicNice(bool status);

        /**
         * @brief Get Dynamic Nice status
         *
         * @return true if dynamic nice is on otherwise off
         */
        bool IsDynamicNiceOn();

        /**
         * @brief Return how many threads assigned
         * 
         * @return size_t number of assigned threads
         */
        size_t GetThreadCount();
        
        /**
         *  SPECIAL PRIVATE SECTION FOR HELPER METHODS USED BY PROCTED METHODS
         */
    private:

        /**
         * @brief Set the Default Parameters for constructors
         *
         */
        void SetDefaultParameters ();

        template<typename T> void SetWaitParammeters (T& refVar, size_t nTag, aSubTypes asubType = aSubTypes::wait)
        {
            m_TopicId = 0;
            m_pLockId = (uint8_t*)&refVar;
            m_aStatus = aTypes::wait;
            m_aSubStatus = asubType;

            m_lockMessage.tag = nTag;
            m_lockMessage.message = 0;
        }

        template<typename T> bool IsNotificationEligible (atomicx& thr, T& refVar, size_t nTag, aSubTypes subType)
        {
            if (thr.m_aSubStatus == subType &&
                thr.m_aStatus == aTypes::wait && 
                thr.m_pLockId == (void*) &refVar &&
                nTag == thr.m_lockMessage.tag)
            {
                return true;
            }

            return false;
        }
        
        /**
         * @brief Safely notify all Waits from a specific reference pointer along with a message without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  The size_t message to be sent
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification
         * @param notifyAll default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeNotifier(size_t& nMessage, T& refVar, size_t nTag, aSubTypes subType, NotifyType notifyAll=NotifyType::one)
        {
            size_t nRet = 0;

            for (auto& thr : *this)
            {
                if (IsNotificationEligible (thr, refVar, nTag, subType))
                {
                    thr.m_TopicId = 0;
                    thr.m_aStatus = aTypes::now;
                    thr.m_nTargetTime = 0;
                    thr.m_pLockId = nullptr;

                    thr.m_lockMessage.message = nMessage;
                    thr.m_lockMessage.tag = nTag;

                    nRet++;

                    if (notifyAll == NotifyType::one)
                    {
                        break;
                    }
                }
            }

            return nRet;
        }

        /**
         * @brief Safely notify all LookForWaitings from a specific reference pointer along with a message without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeNotifyLookWaitings(T& refVar, size_t nTag)
        {
            size_t message=0;

            return SafeNotifier(message, refVar, nTag, aSubTypes::look, NotifyType::all);
        }

    /**
     *  PROTECTED METHODS, THOSE WILL BE ONLY ACCESSIBLE BY THE THREAD ITSELF
     */
    protected:

        struct Message
        {
            size_t tag;
            size_t message;
        };

        /**
         * ------------------------------
         * MESSAGE BROADCAST IMPLEMENTATION
         * ------------------------------
         */

        /**
         * @brief Enable or Disable async broadcast 
         * 
         * @param bBroadcastStatus   if true, the thread will receive broadcast otherwise no
         */
        void SetReceiveBroadcast (bool bBroadcastStatus);

        /**
         * @brief Broadcast a message to all threads
         * 
         * @param messageReference Works as the signaling
         * @param message   Message structure with the message
         *                  message is the payload
         *                  tag is the meaning
         * 
         * @return size_t 
         */
        size_t BroadcastMessage (const size_t messageReference, const Message message);

        /**
         * @brief The default boradcast handler used to received the message
         * 
         * @param messageReference Works as the signaling
         * @param message   Message structure with the message
         *                  message is the payload
         *                  tag is the meaning
         */
        virtual void BroadcastHandler (const size_t& messageReference, const Message& message);

        /**
         * @brief calculate the Topic ID for a given topic text
         *
         * @param pszTopic    Topic Text in C string
         * @param nKeyLenght  Size, in bytes + zero terminated char
         *
         * @return uint32_t  The calculated topic ID
         */
        uint32_t GetTopicID (const char* pszTopic, size_t nKeyLenght);

        /**
         * ------------------------------
         * SMART WAIT/NOTIFY  IMPLEMENTATION
         * ------------------------------
         */

        /**
         * @brief Set the Async Wait Handler Enable or disable
         * 
         * @param awaitStatus  async wait handler status to be set
         */
        void SetAsyncWaitHandler(bool awaitStatus);

        /**
         * @brief Is thread is marked as asynWait, all notify will call this functions
         * 
         * @param refVar    The pointer for the reference used as notifier
         * @param nTag      The notification meaning
         * 
         * @return false if it is not subscribed, otherwise 0
         * 
         * @note if not implemented a default implementation will be used returning always zero
         */
        virtual bool AsyncWaitHander (size_t refVar, size_t nTag) noexcept;

        /**
         * @brief Sync with thread call for a wait (refVar,nTag)
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer
         * @param nTag      The notification meaning
         * @param waitFor   default=0, if 0 wait indefinitely, otherwise wait for custom tick granularity times
         * @param hasAtleast define how minimal Wait calls to report true
         *
         * @return true There is thread waiting for the given refVar/nTag
         */
        template<typename T> bool LookForWaitings(T& refVar, size_t nTag, size_t hasAtleast, atomicx_time waitFor)
        {
            Timeout timeout (waitFor);

            while ((waitFor == 0 || timeout.IsTimedout () == false) && IsWaiting(refVar, nTag, hasAtleast) == false)
            {
                SetWaitParammeters (refVar, nTag, aSubTypes::look);

                Yield(waitFor);

                m_lockMessage = {0,0};

                if (m_aSubStatus == aSubTypes::timeout)
                {
                    return false;
                }

                // Decrease the timeout time to slice the remaining time otherwise break it
                if (waitFor  == 0 || (waitFor = timeout.GetRemaining ()) == 0)
                {
                    break;
                }
            }

            return (timeout.IsTimedout ()) ? false : true;
        }

        /**
         * @brief Sync with thread call for a wait (refVar,nTag)
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer
         * @param nTag      The notification meaning
         * @param waitFor   default=0, if 0 wait indefinitely, otherwise wait for custom tick granularity times
         *
         * @return true There is thread waiting for the given refVar/nTag
         */
        template<typename T> bool LookForWaitings(T& refVar, size_t nTag, atomicx_time waitFor)
        {
            if (IsWaiting(refVar, nTag) == false)
            {
                SetWaitParammeters (refVar, nTag, aSubTypes::look);

                Yield(waitFor);

                m_lockMessage = {0,0};

                if (m_aSubStatus == aSubTypes::timeout)
                {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Check if there are waiting threads for a given reference pointer and tag value
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true
         *
         * @note This is a powerful tool since it create layers of waiting within the same reference pointer
         */
        template<typename T> bool IsWaiting(T& refVar, size_t nTag, size_t hasAtleast = 1, aSubTypes asubType = aSubTypes::wait)
        {
            hasAtleast = hasAtleast == 0 ? 1 : hasAtleast;

            if (HasWaitings (refVar, nTag, asubType)>=hasAtleast)
            {
                return true;
            }
            
            return false;
        }

        /**
         * @brief Report how much waiting threads for a given reference pointer and tag value are there
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true
         *
         * @note This is a powerful tool since it create layers of waiting within the same reference pointer
         */
        template<typename T> size_t HasWaitings(T& refVar, size_t nTag, aSubTypes asubType = aSubTypes::wait)
        {
            size_t nCounter = 0;

            for (auto& thr : *this)
            {
                if (IsNotificationEligible (thr, refVar, nTag, asubType))
                {
                    nCounter++;
                }
            }

            return nCounter;
        }

        /**
         * @brief Blocks/Waits a notification along with a message and tag from a specific reference pointer
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  the size_t message to be received
         * @param refVar    the reference pointer used as a notifier
         * @param nTag      the size_t tag that will give meaning to the the message, if nTag == 0 means wait all refVar regardless
         * @param waitFor   default==0 (undefinitly), How log to wait for a notification based on atomicx_time
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         * @return true if it was successfully received.
         */
        template<typename T> bool Wait(size_t& nMessage, T& refVar, size_t nTag, atomicx_time waitFor=0, aSubTypes asubType = aSubTypes::wait)
        {
            SafeNotifyLookWaitings(refVar, nTag);

            SetWaitParammeters (refVar, nTag, asubType);

            m_lockMessage.tag = nTag;

            Yield(waitFor);

            bool bRet = false;

            if (m_aSubStatus != aSubTypes::timeout)
            {
                nMessage = m_lockMessage.message;
                bRet = true;
            }

            m_lockMessage = {0,0};

            m_aSubStatus = aSubTypes::ok;

            return bRet;
        }

        /**
         * @brief Blocks/Waits a notification along with a tag from a specific reference pointer
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    the reference pointer used as a notifier
         * @param nTag      the size_t tag that will give meaning to the the message, if nTag == 0 means wait all refVar regardless
         * @param waitFor   default==0 (undefinitly), How log to wait for a notification based on atomicx_time
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true if it was successfully received.
         */
        template<typename T> bool Wait(T& refVar, size_t nTag, atomicx_time waitFor=0, aSubTypes asubType = aSubTypes::wait)
        {
            SafeNotifyLookWaitings(refVar, nTag);

            SetWaitParammeters (refVar, nTag, asubType);

            m_lockMessage.tag = nTag;

            Yield(waitFor);

            bool bRet = false;

            if (m_aSubStatus != aSubTypes::timeout)
            {
                bRet = true;
            }

            m_lockMessage = {0,0};
            m_aSubStatus = aSubTypes::ok;

            return bRet;
        }

        /**
         * @brief Safely notify all Waits from a specific reference pointer along with a message without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  The size_t message to be sent
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param notifyAll default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeNotify(size_t& nMessage, T& refVar, size_t nTag,  NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
            return SafeNotifier(nMessage, refVar, nTag, asubType, notifyAll);
        }

        /**
         * @brief Notify all Waits from a specific reference pointer along with a message and trigger context change if at least one wait thread got notified
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  The size_t message to be sent
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param notifyAll default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t Notify(size_t& nMessage, T& refVar, size_t nTag, NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
            size_t bRet = SafeNotify (nMessage, refVar, nTag, notifyAll, asubType);

            if (bRet) Yield(0);

            return bRet;
        }

        template<typename T> size_t Notify(size_t&& nMessage, T& refVar, size_t nTag, NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
            size_t bRet = SafeNotify (nMessage, refVar, nTag, notifyAll, asubType);

            if (bRet) Yield(0);

            return bRet;
        }

        /**
         * @brief SYNC Waits for at least one Wait call for a given reference pointer along with a message and trigger context change
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  The size_t message to be sent
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param waitForWaitings default=0 (waiting for Waiting calls) othersize wait for Wait commands compatible with the paramenters (Sync call).
         * @param notifyAll  default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SyncNotify(size_t& nMessage, T& refVar, size_t nTag, atomicx_time waitForWaitings=0, NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
            if (LookForWaitings (refVar, nTag, waitForWaitings) == false)
            {
                return 0;
            }

            size_t bRet = SafeNotify (nMessage, refVar, nTag, notifyAll, asubType);

            if (bRet) Yield(0);

            return bRet;
        }

        template<typename T> size_t SyncNotify(size_t&& nMessage, T& refVar, size_t nTag, atomicx_time waitForWaitings=0, NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
            if (LookForWaitings (refVar, nTag, waitForWaitings) == false)
            {
                return 0;
            }

            size_t bRet = SafeNotify (nMessage, refVar, nTag, notifyAll, asubType);

            if (bRet) Yield(0);

            return bRet;
        }

        /**
         * @brief Safely notify all Waits from a specific reference pointer without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param notifyAll default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeNotify(T& refVar, size_t nTag, NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
             size_t message=0;
             return SafeNotifier (message, refVar, nTag, asubType, notifyAll);
        }

        /**
         * @brief SYNC Waits for at least one Wait call for a given reference pointer and trigger context change
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param waitForWaitings default=0 (waiting for Waiting calls) othersize wait for Wait commands compatible with the paramenters (Sync call).
         * @param notifyAll      default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SyncNotify(T& refVar, size_t nTag, atomicx_time waitForWaitings=0, NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
            if (LookForWaitings (refVar, nTag, waitForWaitings) == false)
            {
                return 0;
            }

            size_t bRet = SafeNotify(refVar, nTag, notifyAll, asubType);

            if (bRet) Yield(0);

            return bRet;
        }

        /**
         * @brief Notify all Waits from a specific reference pointer and trigger context change if at least one wait thread got notified
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param notifyAll default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         * @param asubType  Type of the notification, only use it if you know what you are doing, it creates a different
         *                  type of wait/notify, deafault == aSubType::wait
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t Notify(T& refVar, size_t nTag, NotifyType notifyAll=NotifyType::one, aSubTypes asubType = aSubTypes::wait)
        {
            size_t bRet = SafeNotify(refVar, nTag, notifyAll, asubType);

            if (bRet) Yield(0);

            return bRet;
        }

        /**
         * @brief Set the Stack Increase Pace object
         *
         * @param nIncreasePace The new stack increase pace value
         */
        void SetStackIncreasePace(size_t nIncreasePace);

    private:

        /**
         * @brief Add the current thread to the global thread list
         */
        void AddThisThread();

        /**
         * @brief Remove the current thread to the global thread list
         */
        void RemoveThisThread();

        /**
         * @brief CRC16 used to compose a multi uint32_t for Topic ID
         *
         * @param pData     Data bytes to be used on calculation
         * @param nSize     The size of the Data bytes
         * @param nCRC      The previous CRC calculated or a specific starter (salt)
         *
         * @return uint16_t the CRC calculated.
         */
        uint16_t crc16(const uint8_t* pData, size_t nSize, uint16_t nCRC);

        /**
         * @brief The scheduler used by the kernel implementation
         *
         * @return true it at least one thread is not blocked, if all threads a blocked
         *          a false is returned telling to the kernel  to stop Start with false.
         */
        static bool SelectNextThread(void);

        /**
         * Thread related controll variable
         */

        atomicx* m_paNext = nullptr;
        atomicx* m_paPrev = nullptr;

        jmp_buf m_context;

        size_t m_stackSize=0;
        size_t m_stacUsedkSize=0;
        size_t m_stackIncreasePace=1;

        Message m_lockMessage = {0,0};

        atomicx_time m_nTargetTime=0;
        atomicx_time m_nice=0;
        atomicx_time m_LastUserExecTime=0;
        atomicx_time m_lastResumeUserTime=0;

        uint32_t m_TopicId=0;

        aTypes  m_aStatus = aTypes::start;
        aSubTypes m_aSubStatus = aSubTypes::ok;

        volatile uint8_t* m_stack;
        volatile uint8_t* m_pStaskStart=nullptr;
        volatile uint8_t* m_pStaskEnd=nullptr;

        uint8_t* m_pLockId=nullptr;

        struct
        {
            bool autoStack : 1;
            bool dynamicNice : 1;
            bool broadcast : 1;
        } m_flags = {0,0,0};
    };
}

#endif /* atomicx_hpp */
