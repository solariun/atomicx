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

extern atomicx_time Atomicx_GetTick(void);
extern void Atomicx_SleepTick(atomicx_time nSleep);
extern void yield(void);

namespace thread
{
    /* Official version */
    static const int coreXVVersion[]={ 1, 0, 0 };
    static const char coreVersionString[] = "V1.0.0 built at " __TIMESTAMP__;

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
            aiterator(atomicx* ptr);

            atomicx& operator*() const;
            atomicx* operator->();
            aiterator& operator++();

            friend bool operator== (const aiterator& a, const aiterator& b){ return a.m_ptr == b.m_ptr;};
            friend bool operator!= (const aiterator& a, const aiterator& b){ return a.m_ptr != b.m_ptr;};

        private:
            atomicx* m_ptr;
        };
        
        /**
         * ------------------------------
         * SUPLEMENTAR SMART_PTR IMPLEMENTATION
         * ------------------------------
         */
        template <typename T> class smart_ptr
        {
        public:
            
            smart_ptr(T* p) : pRef (new reference {p, 1})
            { }
            
            smart_ptr(const smart_ptr<T>& sa)
            {
                pRef = sa.pRef;
                pRef->nRC++;
            }
            
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
            
            T* operator-> (void)
            {
                return pRef->pReference;
            }

            T& operator& (void)
            {
                return *pRef->pReference;
            }

            bool IsValid(void)
            {
                return pRef == nullptr ? false : pRef->pReference == nullptr ? false : true;
            }
            
            size_t GetRefCounter(void)
            {
                if (pRef != nullptr)
                {
                    return pRef->nRC;
                }
                
                return 0;
            }

        private:
            
            smart_ptr(void){};
            struct reference
            {
                T* pReference ;
                size_t nRC;
            };
            
            reference* pRef=nullptr;
        };

        aiterator begin(void);
        aiterator end(void);
                
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
            
            queue(size_t nQSize):m_nQSize(nQSize), m_nItens{0}
            {}
            
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
            
            size_t GetSize()
            {
                return m_nItens;
            }
            
        protected:
            
            class QItem
            {
            public:
                QItem () = delete;
                
                QItem(T& qItem) : m_qItem(qItem), m_pNext(nullptr)
                {}
                
                T& GetItem()
                {
                    return m_qItem;
                }
                
            protected:
                friend class queue;
                
                void SetNext (QItem& qItem)
                {
                    m_pNext = &qItem;
                }

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
            void Lock();
            void Unlock();
            void SharedLock();
            void SharedUnlock();
            size_t IsShared();
            bool IsLocked();
            
        protected:
        private:
            size_t nSharedLockCount=0;
            bool bExclusiveLock=false;
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

        template<typename T> bool IsWaiting(T& refVar, size_t nTag, bool bAll=false)
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
    };
}

#endif /* atomicx_hpp */
