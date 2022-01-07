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

using atomicx_time = uint32_t;

//Using defines to avoid unnecessary RAM usage
#define  atomicx_notify_one false
#define  atomicx_notify_all true


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

/**
 * @brief Implement a yield
 *
 * @note this function exists for compatibility with other OSs
 */
extern void yield(void);


namespace thread
{
    /* Official version */
    #define COREX_VERSION "1.0.0"
    static const char coreVersionString[] = "V" COREX_VERSION " built at " __TIMESTAMP__;

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
             * @brief  Check if the referece still valis
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
         * ------------------------------
         * SMART LOCK IMPLEMENTATION
         * ------------------------------
         */

        /* The stamart mutex implementation */
        class lock
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
            void Lock();

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
            void SharedLock();

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
        class SmartLock
        {
            public:
                SmartLock() = delete;

                /**
                 * @brief Construct a new Smart Lock object based a existing lock
                 *
                 * @param lockObj the existing lock object
                 */
                SmartLock (lock& lockObj) : m_lock(lockObj)
                {}

                /**
                 * @brief Destroy and release the smart lock taken
                 */
                ~SmartLock()
                {
                    switch (m_lockType)
                    {
                        case 'L':
                            m_lock.Unlock();
                            break;
                        case 'S':
                            m_lock.SharedUnlock();
                            break;
                    }
                }

                /**
                 * @brief Accquire a SharedLock
                 *
                 * @return true if accquired, false if another accquisition was already done
                 */
                bool SharedLock()
                {
                    bool bRet = false;

                    if (m_lockType == '\0')
                    {
                        m_lock.SharedLock();
                        m_lockType = 'S';
                        bRet = true;
                    }

                    return bRet;
                }

                /**
                 * @brief Accquire a exclusive Lock
                 *
                 * @return true if accquired, false if another accquisition was already done
                 */
                bool Lock()
                {
                    bool bRet = false;

                    if (m_lockType == '\0')
                    {
                        m_lock.Lock();
                        m_lockType = 'L';
                        bRet = true;
                    }

                    return bRet;
                }

                /**
                 * @brief Check how many shared locks are accquired
                 *
                 * @return size_t   Number of threads holding shared locks
                 */
                size_t IsShared()
                {
                    return m_lock.IsShared();
                }

                /**
                 * @brief Check if a exclusive lock has been already accquired
                 *
                 * @return true if yes, otherwise false
                 */
                bool IsLocked()
                {
                    return m_lock.IsLocked();
                }

            private:

            lock& m_lock;
            uint8_t m_lockType = '\0';
        };

        /**
         * PUBLIC OBJECT METHOS
         */


        atomicx() = delete;

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
        atomicx_time GetCurrentTick();

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
        template<typename T, size_t N>
        atomicx(T (&stack)[N]) : m_context{}, m_stack((volatile uint8_t*) stack)
        {
            m_stackSize = N  ;

            AddThisThread();
        }

        /**
         * @brief The pure virtual function that runs the thread loop
         *
         * @note REQUIRED implementation and once it returns it will execute finish method
         */
        virtual void run(void) noexcept = 0;

        /**
         * @brief Handles the StackOverflow of the current thread
         *
         * @note HIGHLY ADVISED TO BE IMPLEMENTED, if not implemented a default "empty" call is used instead
         */
        virtual void StackOverflowHandler(void) noexcept
        {
            return;
        };

        /**
         * @brief Called right after run returns, can be used to self-destroy the object and other maintenance actions
         *
         * @note if not implemented a default "empty" call is used instead
         */
        virtual void finish() noexcept
        {
            return;
        }

    protected:

        struct Message
        {
            size_t tag;
            size_t message;
        };

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
         * @brief Check if there are waiting threads for a given reference pointer and tag value
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification,  if nTag == 0  mean all bTag for the refVar
         *
         * @return true
         *
         * @note This is a powerful tool since it create layers of waiting within the same reference pointer
         */
        template<typename T> bool IsWaiting(T& refVar, size_t nTag=0)
        {
            for (auto& thr : *this)
            {
                if (thr.m_aStatus == aTypes::wait && thr.m_pLockId == (void*) &refVar && (nTag == 0 || thr.m_lockMessage.tag == nTag))
                {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Safely notify all Waits from a specific reference pointer along with a message without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  The size_t message to be sent
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param bAll      default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeNotifier(size_t& nMessage, T& refVar, size_t nTag, aSubTypes subType, bool bAll=false)
        {
            size_t nRet = 0;

            for (auto& thr : *this)
            {
                if (thr.m_aSubStatus == subType && thr.m_aStatus == aTypes::wait && thr.m_pLockId == (void*) &refVar && (nTag == thr.m_lockMessage.tag || thr.m_lockMessage.tag == 0 || nTag == 0))
                {
                    thr.m_TopicId = 0;
                    thr.m_aStatus = aTypes::now;
                    thr.m_nTargetTime = 0;
                    thr.m_pLockId = nullptr;

                    thr.m_lockMessage.message = nMessage;
                    thr.m_lockMessage.tag = nTag;

                    nRet++;

                    if (bAll == false)
                    {
                        break;
                    }
                }
            }

            return nRet;
        }

        /**
         * @brief Sync with thread call for a wait (refVar,nTag)
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer
         * @param nTag      The notification meaning, if nTag == 0 means wait all refVar regardless
         * @param waitFor   default=0, if 0 wait indefinitely, otherwise wait for custom tick granularity times
         *
         * @return true There is thread waiting for the given refVar/nTag
         */
        template<typename T> bool LookForWaitings(T& refVar, size_t nTag=0, atomicx_time waitFor=0)
        {
            if (IsWaiting(refVar, nTag) == false)
            {
                m_TopicId = 0;
                m_pLockId = (uint8_t*)&refVar;
                m_aStatus = aTypes::wait;
                m_aSubStatus = aSubTypes::look;

                m_lockMessage.tag = nTag;

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
         * @brief Safely notify all LookForWaitings from a specific reference pointer along with a message without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means wait all refVar regardless
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeLookNotifiers(T& refVar, size_t nTag)
        {
            size_t message=0;

            return SafeNotifier(message, refVar, nTag, aSubTypes::look);
        }

        /**
         * @brief Blocks/Waits a notification along with a message and tag from a specific reference pointer
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  the size_t message to be received
         * @param refVar    the reference pointer used as a notifier
         * @param nTag      the size_t tag that will give meaning to the the message, if nTag == 0 means wait all refVar regardless
         *
         * @return true if it was successfully received.
         */
        template<typename T> bool Wait(size_t& nMessage, T& refVar, size_t nTag=0, atomicx_time waitFor=0)
        {
            SafeLookNotifiers(refVar, nTag);

            m_TopicId = 0;
            m_pLockId = (uint8_t*)&refVar;
            m_aStatus = aTypes::wait;
            m_aSubStatus = aSubTypes::wait;

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
         *
         * @return true if it was successfully received.
         */
        template<typename T> bool Wait(T& refVar, size_t nTag=0, atomicx_time waitFor=0)
        {
            SafeLookNotifiers(refVar, nTag);

            m_TopicId = 0;
            m_pLockId = (uint8_t*)&refVar;
            m_aStatus = aTypes::wait;
            m_aSubStatus = aSubTypes::wait;

            m_lockMessage.tag = nTag;

            Yield(waitFor);

            bool bRet = false;

            if (m_aSubStatus != aSubTypes::timeout)
            {
                bRet = true;
            }

            m_lockMessage = {0,0};
            m_aSubStatus = aSubTypes::ok;

            return true;
        }

        /**
         * @brief Safely notify all Waits from a specific reference pointer along with a message without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  The size_t message to be sent
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param bAll      default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeNotify(size_t& nMessage, T& refVar, size_t nTag=0,  bool bAll=false)
        {
            return SafeNotifier(nMessage, refVar, nTag, aSubTypes::wait, bAll);
        }

        /**
         * @brief Notify all Waits from a specific reference pointer along with a message and trigger context change if at least one wait thread got notified
         *
         * @tparam T        Type of the reference pointer
         * @param nMessage  The size_t message to be sent
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param bAll      default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t Notify(size_t& nMessage, T& refVar, size_t nTag=0, bool bAll=false)
        {
            size_t bRet = SafeNotify (nMessage, refVar, nTag, bAll);

            if (bRet) Yield(0);

            return bRet;
        }

        /**
         * @brief Safely notify all Waits from a specific reference pointer without triggering context change
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param bAll      default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t SafeNotify(T& refVar, size_t nTag=0, bool bAll=false)
        {
             size_t message=0;
             return SafeNotifier (message, refVar, nTag, aSubTypes::wait, bAll);
        }

        /**
         * @brief Notify all Waits from a specific reference pointer and trigger context change if at least one wait thread got notified
         *
         * @tparam T        Type of the reference pointer
         * @param refVar    The reference pointer used a a notifier
         * @param nTag      The size_t tag that will give meaning to the notification, if nTag == 0 means notify all refVar regardless
         * @param bAll      default = false, and only the fist available refVar Waiting thread will be notified, if true all available
         *                  refVar waiting thread will be notified.
         *
         * @return true     if at least one got notified, otherwise false.
         */
        template<typename T> size_t Notify(T& refVar, size_t nTag=0, bool bAll=false)
        {
            size_t bRet = SafeNotify(refVar, nTag, bAll);

            if (bRet) Yield(0);

            return bRet;
        }

        /**
         * ------------------------------
         * SMART BROKER IMPLEMENTATION
         * ------------------------------
         */

        /**
         * @brief Block and wait for message from a specific topic string
         *
         * @param pszKey        The Topic string
         * @param nKeyLenght    The size of the topic string in bytes
         * @param message       the atomicx::message structure with message and tag
         *
         * @return true if it was successfully received, otherwise false
         */
        bool WaitBrokerMessage (const char* pszKey, size_t nKeyLenght, Message& message);

        /**
         * @brief Block and wait for a notification from a specific topic string
         *
         * @param pszKey        The Topic string
         * @param nKeyLenght    The size of the topic string in bytes
         *
         * @return true if it was successfully received, otherwise false
         */
        bool WaitBrokerMessage (const char* pszKey, size_t nKeyLenght);

        /**
         * @brief Publish a message for a specific topic string and trigger a context change if any delivered
         *
         * @param pszKey        The Topic string
         * @param nKeyLenght    The size of the topic string in bytes
         * @param message       the atomicx::message structure with message and tag
         *
         * @return true if at least one thread has received a message
         */
        bool Publish (const char* pszKey, size_t nKeyLenght, const Message message);

        /**
         * @brief Safely Publish a message for a specific topic string DO NOT trigger a context change if any delivered
         *
         * @param pszKey        The Topic string
         * @param nKeyLenght    The size of the topic string in bytes
         * @param message       the atomicx::message structure with message and tag
         *
         * @return true if at least one thread has received a message
         *
         * @note Ideal for been used with interrupt request
         */
        bool SafePublish (const char* pszKey, size_t nKeyLenght, const Message message);

        /**
         * @brief Publish a notification for a specific topic string and trigger a context change if any delivered
         *
         * @param pszKey        The Topic string
         * @param nKeyLenght    The size of the topic string in bytes
         *
         * @return true if at least one thread has received a message
         */
        bool Publish (const char* pszKey, size_t nKeyLenght);

        /**
         * @brief Safely Publish a notification for a specific topic string DO NOT trigger a context change if any delivered
         *
         * @param pszKey        The Topic string
         * @param nKeyLenght    The size of the topic string in bytes
         *
         * @return true if at least one thread has received a message
         *
         * @note Ideal for been used with interrupt request
         */
        bool SafePublish (const char* pszKey, size_t nKeyLenght);

        /**
         * @brief Check if there is subscryption for a specific Topic String
         *
         * @param pszTopic      The Topic string in C string
         * @param nKeyLenght    The Topic C string length in bytes
         *
         * @return true if any substriction is found, otherwise false
         */
        bool HasSubscriptions (const char* pszTopic, size_t nKeyLenght);

        /**
         * @brief Check if there is subscryption for a specific Topic ID
         *
         * @param nKeyID       The Topic ID uint32_t
         *
         * @return true if any substriction is found, otherwise false
         */
        bool HasSubscriptions (uint32_t nKeyID);

        /**
         * @brief Default broker handler for a subscribed message
         *
         * @param pszKey        The Topic C string
         * @param nKeyLenght    The Topic C string size in bytes
         * @param message       The atomicx::message payload received
         *
         * @return true signify it was correctly processed
         *
         * @note Can be overloaded by the derived by the derived thread implementation and specialized,
         *       otherwise a empty function will be called instead
         */
        virtual bool BrokerHandler(const char* pszKey, size_t nKeyLenght, Message& message)
        {
            (void) pszKey; (void) nKeyLenght; (void) message;
            return false;
        }

        /**
         * @brief Specialize and gives power to decide if a topic is subscrybed on not
         *
         * @param pszKey        The Topic C String
         * @param nKeyLenght    The Topic C String size in bytes
         *
         * @return true if the given topic was subscribed, otherwise false.
         */
        virtual bool IsSubscribed (const char* pszKey, size_t nKeyLenght)
        {
            (void) pszKey; (void) nKeyLenght;

            return false;
        }

        /**
         * @brief Foce the context change explicitly
         *
         * @param nSleep  default is 0, otherwise it will override the nice and sleep for n custom tick granularity
         *
         * @return true if the context came back correctly, otherwise false
         */
        bool Yield(atomicx_time nSleep=0);

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

        atomicx* m_paNext = nullptr;
        atomicx* m_paPrev = nullptr;

        aTypes  m_aStatus = aTypes::start;
        aSubTypes m_aSubStatus = aSubTypes::ok;

        jmp_buf m_context;

        size_t m_stackSize;

        volatile uint8_t* m_stack;

        volatile uint8_t* m_pStaskStart=nullptr;
        volatile uint8_t* m_pStaskEnd=nullptr;
        size_t m_stacUsedkSize=0;

        /* WAIT / BROKER -------- */
        uint8_t* m_pLockId=nullptr;
        uint32_t m_TopicId=0;
        Message m_lockMessage = {0,0};

        atomicx_time m_nTargetTime=0;
        atomicx_time m_nice=0;

        bool bDeleteOnExit = false;
    };
}

#endif /* atomicx_hpp */
