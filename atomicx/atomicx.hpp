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
        enum class atypes : uint8_t
        {
            start=1,
            running=5,
            now=6,
            stop=10,
            lock=50,
            subscription=60,
            sleep=100,
            stackOverflow=255
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
             * @brief atomicx based contructor
             *
             * @param ptr
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

        class SmartLock
        {
            public:
                SmartLock() = delete;
                SmartLock (lock& lockObj) : m_lock(lockObj)
                {}

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

                size_t IsShared()
                {
                    return m_lock.IsShared();
                }

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

        virtual ~atomicx(void);

        static atomicx* GetCurrent();

        static bool Start(void);

        size_t GetID(void);
        size_t GetStackSize(void);
        atomicx_time GetNice(void);
        size_t GetUsedStackSize(void);
        atomicx_time GetCurrentTick();
        virtual const char* GetName(void);
        atomicx_time GetTargetTime(void);
        int GetStatus(void);
        size_t GetReferenceLock(void);
        size_t GetTagLock(void);

        void SetNice (atomicx_time nice);

        template<typename T, size_t N>
        atomicx(T (&stack)[N]) : m_context{}, m_stack((volatile uint8_t*) stack)
        {
            m_stackSize = N  ;

            AddThisThread();
        }

        virtual void run(void) noexcept = 0;

        virtual void StackOverflowHandler(void) = 0;

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

        uint32_t GetTopicID (const char* pszTopic, size_t nKeyLenght);

        /**
         * ------------------------------
         * SMART WAIT/NOTIFY  IMPLEMENTATION
         * ------------------------------
         */

        template<typename T> bool Wait(size_t& nMessage, T& refVar, size_t nTag)
        {
            m_TopicId = 0;
            m_pLockId = (uint8_t*)&refVar;
            m_aStatus = atypes::lock;

            m_lockMessage.tag = nTag;

            Yield();

            nMessage = m_lockMessage.message;

            m_lockMessage = {0,0};

            return true;
        }

        template<typename T> bool Wait(T& refVar, size_t nTag)
        {
            m_TopicId = 0;
            m_pLockId = (uint8_t*)&refVar;
            m_aStatus = atypes::lock;

            m_lockMessage.tag = nTag;

            Yield();

            m_lockMessage = {0,0};

            return true;
        }

        template<typename T> bool SafeNotify(size_t& nMessage, T& refVar, uint32_t nTag, bool bAll=false)
        {
            bool bRet = false;

            for (auto& thr : *this)
            {
                if (thr.m_pLockId == (void*) &refVar && nTag == thr.m_lockMessage.tag)
                {
                    thr.m_TopicId = 0;
                    thr.m_aStatus = atypes::now;
                    thr.m_nTargetTime = 0;
                    thr.m_pLockId = nullptr;

                    thr.m_lockMessage.message = nMessage;

                    bRet = true;

                    if (bAll == false)
                    {
                        break;
                    }
                }
            }

            return bRet;
        }

        template<typename T> bool Notify(size_t& nMessage, T& refVar, uint32_t nTag, bool bAll=false)
        {
            bool bRet = SafeNotify (nMessage, refVar, nTag, bAll);

            if (bRet) Yield(0);

            return bRet;
        }

        template<typename T> bool SafeNotify(T& refVar, size_t nTag, bool bAll=false)
        {
            bool bRet = false;

            for (auto& thr : *this)
            {
                if (thr.m_pLockId == (void*) &refVar && nTag == thr.m_lockMessage.tag)
                {
                    thr.m_TopicId = 0;
                    thr.m_aStatus = atypes::now;
                    thr.m_nTargetTime = 0;
                    thr.m_pLockId = nullptr;

                    thr.m_lockMessage.message = 0;

                    bRet = true;

                    if (bAll == false)
                    {
                        break;
                    }
                }
            }

            return bRet;
        }

        template<typename T> bool Notify(T& refVar, size_t nTag, bool bAll=false)
        {
            bool bRet = SafeNotify(refVar, nTag, bAll);

            if (bRet) Yield(0);

            return bRet;
        }

        template<typename T> bool IsWaiting(T& refVar, size_t nTag)
        {
            for (auto& thr : *this)
            {
                if (thr.m_pLockId == (void*) &refVar && thr.m_lockMessage.tag == nTag)
                {
                    return true;
                }
            }

            return false;
        }

        /**
         * ------------------------------
         * SMART BROKER IMPLEMENTATION
         * ------------------------------
         */

        bool WaitBrokerMessage (const char* pszKey, size_t nKeyLenght, Message& message);
        bool WaitBrokerMessage (const char* pszKey, size_t nKeyLenght);

        bool Publish (const char* pszKey, size_t nKeyLenght, const Message message);
        bool SafePublish (const char* pszKey, size_t nKeyLenght, const Message message);

        bool Publish (const char* pszKey, size_t nKeyLenght);
        bool SafePublish (const char* pszKey, size_t nKeyLenght);

        bool HasSubscriptions (const char* pszTopic, size_t nKeyLenght);
        bool HasSubscriptions (uint32_t nKeyID);

        virtual bool BrokerHandler(const char* pszKey, size_t nKeyLenght, Message& message)
        {
            (void) pszKey; (void) nKeyLenght; (void) message;
            return false;
        }

        virtual bool IsSubscribed (const char* pszKey, size_t nKeyLenght)
        {
            (void) pszKey; (void) nKeyLenght;

            return false;
        }

        bool Yield(atomicx_time nSleep=0);

    private:

        void AddThisThread();
        void RemoveThisThread();

        uint16_t crc16(const uint8_t* pData, size_t nSize, uint16_t nCRC);

        static bool SelectNextThread(void);

        atomicx* m_paNext = nullptr;
        atomicx* m_paPrev = nullptr;

        atypes  m_aStatus = atypes::start;

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
