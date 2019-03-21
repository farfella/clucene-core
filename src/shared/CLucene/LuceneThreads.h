/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#pragma once


namespace lucene {
    namespace util {
        struct CLuceneThreadIdCompare;

        class mutexGuard;

#define _LUCENE_THREADID_TYPE uint64_t
#define _LUCENE_THREAD_FUNC(name, argName) void __stdcall name(void* argName) //< use this macro to correctly define the thread start routine
#define _LUCENE_THREAD_FUNC_RETURN(val) CL_NS(util)::mutex_thread::_exitThread(val)

#define _LUCENE_ATOMIC_INC(theInteger) CL_NS(util)::mutex_thread::atomic_increment(theInteger)
#define _LUCENE_ATOMIC_DEC(theInteger) CL_NS(util)::mutex_thread::atomic_decrement(theInteger)

#ifdef _M_X64
    #define _LUCENE_ATOMIC_INT long long
#else
    #define _LUCENE_ATOMIC_INT long
#endif

#define _LUCENE_ATOMIC_INT_SET(x,v) x=v
#define _LUCENE_ATOMIC_INT_GET(x) x

        //typedef void (__stdcall luceneThreadStartRoutine)(void* lpThreadParameter );
        typedef void(__cdecl*   luceneThreadStartRoutine)(void*);
        class CLUCENE_SHARED_EXPORT mutex_thread
        {
        private:
            struct Internal;
            Internal* _internal;
        public:
            mutex_thread(const mutex_thread& clone);
            mutex_thread();
            ~mutex_thread();
            void lock();
            void unlock();
            static void _exitThread(int ret);
            static _LUCENE_THREADID_TYPE _GetCurrentThreadId();
            static _LUCENE_THREADID_TYPE CreateThread(luceneThreadStartRoutine func, void* arg);
            static void JoinThread(_LUCENE_THREADID_TYPE id);

            static int32_t atomic_increment(_LUCENE_ATOMIC_INT* theInteger);
            static int32_t atomic_decrement(_LUCENE_ATOMIC_INT* theInteger);
        };

        class CLUCENE_SHARED_EXPORT shared_condition
        {
        private:
            class Internal;
            Internal* _internal;
        public:
            shared_condition();
            ~shared_condition();
            void Wait(mutex_thread* shared_lock);
            void NotifyAll();
        };


#define _LUCENE_THREAD_CREATE(func, arg) CL_NS(util)::mutex_thread::CreateThread(func,arg)
#define _LUCENE_THREAD_JOIN(id) CL_NS(util)::mutex_thread::JoinThread(id)
#define _LUCENE_CURRTHREADID CL_NS(util)::mutex_thread::_GetCurrentThreadId()
#define _LUCENE_THREADMUTEX CL_NS(util)::mutex_thread
#define _LUCENE_THREADCOND CL_NS(util)::shared_condition


        /** @internal */
        class CLUCENE_SHARED_EXPORT mutexGuard
        {
        private:
            _LUCENE_THREADMUTEX * mrMutex;
            mutexGuard(const mutexGuard& clone);
        public:
            mutexGuard(_LUCENE_THREADMUTEX& rMutex);
            ~mutexGuard();
        };

#define SCOPED_LOCK_MUTEX(theMutex) 				CL_NS(util)::mutexGuard theMutexGuard(theMutex);
#define DEFINE_MUTEX(theMutex) 							_LUCENE_THREADMUTEX theMutex;
#define DEFINE_CONDITION(theCondition) 			_LUCENE_THREADCOND theCondition;
#define DEFINE_MUTABLE_MUTEX(theMutex)  		mutable _LUCENE_THREADMUTEX theMutex;
#define STATIC_DEFINE_MUTEX(theMutex) 			static _LUCENE_THREADMUTEX theMutex;

#define CONDITION_WAIT(theMutex, theCondition)	theCondition.Wait(&theMutex);
#define CONDITION_NOTIFYALL(theCondition)				theCondition.NotifyAll();


    }
}
