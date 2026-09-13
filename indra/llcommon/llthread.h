/** 
 * @file llthread.h
 * @brief Base classes for thread, mutex and condition handling.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#ifndef LL_LLTHREAD_H
#define LL_LLTHREAD_H
#if !defined(_MSC_VER) || _MSC_VER >= 1700
#define USE_BOOST_MUTEX 1
#endif
#define IS_LLCOMMON_INLINE (!LL_COMMON_LINK_SHARED || defined(llcommon_EXPORTS))
#if LL_GNUC
#endif
#include "llapp.h"
#include "llapr.h"
#include "llaprpool.h"
#include "llatomic.h"
#include "llmemory.h"
#include "aithreadid.h"
class LLThread;
class LLMutex;
class LLCondition;
class LL_COMMON_API LLThreadLocalDataMember
{
public:
	virtual ~LLThreadLocalDataMember() { };
};
class LL_COMMON_API LLThreadLocalData
{
private:
	static apr_threadkey_t* sThreadLocalDataKey;
public:
	LLAPRRootPool mRootPool;
	LLVolatileAPRPool mVolatileAPRPool;
	LLThreadLocalDataMember* mCurlMultiHandle;
	char* mCurlErrorBuffer;
	std::string mName;
	static void init(void);
	static void destroy(void* thread_local_data);
	static void create(LLThread* pthread);
	static LLThreadLocalData& tldata(void);
private:
	LLThreadLocalData(char const* name);
	~LLThreadLocalData();
};
LL_COMMON_API void assert_main_thread();
class LL_COMMON_API LLThread
{
private:
	static U32 sIDIter;
	static LLAtomicS32	sCount;
	static LLAtomicS32	sRunning;
public:
	typedef enum e_thread_status
	{
		STOPPED = 0,
		RUNNING = 1,
		QUITTING= 2
	} EThreadStatus;
	LLThread(std::string const& name);
	virtual ~LLThread();
	virtual void shutdown();
	bool isQuitting() const { return (QUITTING == mStatus); }
	bool isStopped() const { return (STOPPED == mStatus); }
	static S32 getCount() { return sCount; }
	static S32 getRunning() { return sRunning; }
	static void yield();
public:
	void pause();
	void unpause();
	bool isPaused() { return isStopped() || mPaused; }
	void wake();
	void wakeLocked();
	void checkPause();
	void start(void);
	void setQuitting();
	static LLThreadLocalData& tldata(void) { return LLThreadLocalData::tldata(); }
private:
	bool				mPaused;
	static void *APR_THREAD_FUNC staticRun(apr_thread_t *apr_threadp, void *datap);
protected:
	std::string			mName;
	LLCondition*		mRunCondition;
	apr_thread_t		*mAPRThreadp;
	volatile EThreadStatus		mStatus;
	friend void LLThreadLocalData::create(LLThread* threadp);
	LLThreadLocalData*  mThreadLocalData;
	virtual void run(void) = 0;
	virtual void terminated(void) { mStatus = STOPPED; }
	virtual bool runCondition(void);
	inline void lockData();
	inline void unlockData();
	bool shouldSleep(void) { return (mStatus == RUNNING) && (isPaused() || (!runCondition())); }
};
#ifdef SHOW_ASSERT
#define ASSERT_SINGLE_THREAD do { static AIThreadID first_thread_id; llassert(first_thread_id.equals_current_thread()); } while(0)
#else
#define ASSERT_SINGLE_THREAD do { } while(0)
#endif
#define MUTEX_POOL(arg)
#define NEEDS_MUTEX_IMPL do_not_define_manually_thanks
#undef NEEDS_MUTEX_IMPL
#define NEEDS_MUTEX_RECURSION do_not_define_manually_thanks
#undef NEEDS_MUTEX_RECURSION
#if USE_BOOST_MUTEX && (BOOST_VERSION >= 103400)
#define BOOST_SYSTEM_NO_DEPRECATED
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
typedef boost::recursive_mutex LLMutexImpl;
typedef boost::condition_variable_any LLConditionVariableImpl;
#elif defined(USE_STD_MUTEX)
#include <mutex>
typedef std::recursive_mutex LLMutexImpl;
typedef std::condition_variable_any LLConditionVariableImpl;
#elif defined(USE_WIN32_MUTEX)
typedef CRITICAL_SECTION impl_mutex_handle_type;
typedef CONDITION_VARIABLE impl_cond_handle_type;
#define NEEDS_MUTEX_IMPL
#define NEEDS_MUTEX_RECURSION
#else
#include "apr_thread_cond.h"
#include "apr_thread_mutex.h"
typedef LLAPRPool native_pool_type;
typedef apr_thread_mutex_t* impl_mutex_handle_type;
typedef apr_thread_cond_t* impl_cond_handle_type;
#undef MUTEX_POOL
#undef DEFAULT_POOL
#define MUTEX_POOL(arg) arg
#define NEEDS_MUTEX_IMPL
#define NEEDS_MUTEX_RECURSION
#endif
#include "llfasttimer.h"
#ifdef NEEDS_MUTEX_IMPL
class LL_COMMON_API LLMutexImpl : private boost::noncopyable
{
	friend class LLMutex;
	friend class LLCondition;
	friend class LLConditionVariableImpl;
	typedef impl_mutex_handle_type native_handle_type;
	LLMutexImpl(MUTEX_POOL(native_pool_type& pool));
	virtual ~LLMutexImpl();
	void lock();
	void unlock();
	bool try_lock();
	native_handle_type& native_handle()	{ return mMutexImpl; }
private:
	native_handle_type mMutexImpl;
	MUTEX_POOL(native_pool_type mPool);
};
#endif
class LL_COMMON_API LLMutex : public LLMutexImpl
{
#ifdef NEEDS_MUTEX_IMPL
	friend class LLConditionVariableImpl;
#endif
public:
	LLMutex(MUTEX_POOL(native_pool_type& pool = LLThread::tldata().mRootPool)) : LLMutexImpl(MUTEX_POOL(pool)),
#ifdef NEEDS_MUTEX_RECURSION
		mLockDepth(0),
#endif
		mLockingThread(AIThreadID::sNone)
	{}
	~LLMutex()
	{}
	void lock(LLTrace::BlockTimerStatHandle* timer = NULL)
	{
		if (inc_lock_if_recursive())
			return;
#if IS_LLCOMMON_INLINE
		if (AIThreadID::in_main_thread_inline() && LLApp::isRunning())
#else
		if (AIThreadID::in_main_thread() && LLApp::isRunning())
#endif
		{
			if (!LLMutexImpl::try_lock())
			{
				lock_main(timer);
			}
		}
		else
		{
			LLMutexImpl::lock();
		}
#if IS_LLCOMMON_INLINE
		mLockingThread.reset_inline();
#else
		mLockingThread.reset();
#endif
	}
	void unlock()
	{
#ifdef NEEDS_MUTEX_RECURSION
		if (mLockDepth > 0)
		{
			--mLockDepth;
			return;
		}
#endif
		mLockingThread = AIThreadID::sNone;
		LLMutexImpl::unlock();
	}
	bool try_lock()
	{
		if (inc_lock_if_recursive())
			return true;
		if (!LLMutexImpl::try_lock())
			return false;
#if IS_LLCOMMON_INLINE
		mLockingThread.reset_inline();
#else
		mLockingThread.reset();
#endif
		return true;
	}
	bool isLocked()
	{
		if (isSelfLocked())
			return false;
		if (LLMutexImpl::try_lock())
		{
			LLMutexImpl::unlock();
			return false;
		}
		return true;
	}
	bool isSelfLocked() const
	{
#if IS_LLCOMMON_INLINE
		return mLockingThread.equals_current_thread_inline();
#else
		return mLockingThread.equals_current_thread();
#endif
	}
#ifdef NEEDS_MUTEX_IMPL
	friend class ImplAdoptMutex;
	class ImplAdoptMutex
	{
		friend class LLConditionVariableImpl;
		ImplAdoptMutex(LLMutex& mutex) : mMutex(mutex),
#ifdef NEEDS_MUTEX_RECURSION
			mLockDepth(mutex.mLockDepth),
#endif
			mLockingThread(mutex.mLockingThread)
		{
			mMutex.mLockingThread = AIThreadID::sNone;
#ifdef NEEDS_MUTEX_RECURSION
			mMutex.mLockDepth = 0;
#endif
		}
		~ImplAdoptMutex()
		{
			mMutex.mLockingThread = mLockingThread;
#ifdef NEEDS_MUTEX_RECURSION
			mMutex.mLockDepth = mLockDepth;
#endif
		}
		LLMutex& mMutex;
		AIThreadID mLockingThread;
#ifdef NEEDS_MUTEX_RECURSION
		S32 mLockDepth;
#endif
	};
#endif
private:
	void lock_main(LLTrace::BlockTimerStatHandle* timer);
	bool inc_lock_if_recursive()
	{
#ifdef NEEDS_MUTEX_RECURSION
		if (isSelfLocked())
		{
			mLockDepth++;
			return true;
		}
#endif
		return false;
	}
	mutable AIThreadID	mLockingThread;
#ifdef NEEDS_MUTEX_RECURSION
	LLAtomicS32 mLockDepth;
#endif
};
class LLGlobalMutex : public LLMutex
{
public:
	LLGlobalMutex() : LLMutex(MUTEX_POOL(LLAPRRootPool::get())), mbInitalized(true)
	{}
	bool isInitalized() const
	{
		return mbInitalized;
	}
private:
	bool mbInitalized;
};
#ifdef NEEDS_MUTEX_IMPL
class LL_COMMON_API LLConditionVariableImpl : private boost::noncopyable
{
	friend class LLCondition;
	typedef impl_cond_handle_type native_handle_type;
	LLConditionVariableImpl(MUTEX_POOL(native_pool_type& pool));
	virtual ~LLConditionVariableImpl();
	void notify_one();
	void notify_all();
	void wait(LLMutex& lock);
	native_handle_type& native_handle()	{ return mConditionVariableImpl; }
	native_handle_type mConditionVariableImpl;
	MUTEX_POOL(native_pool_type mPool);
};
#endif
typedef LLMutex LLMutexRootPool;
class LLCondition : public LLConditionVariableImpl, public LLMutex
{
public:
	LLCondition(MUTEX_POOL(native_pool_type& pool = LLThread::tldata().mRootPool)) :
		LLMutex(MUTEX_POOL(pool)),
		LLConditionVariableImpl(MUTEX_POOL(pool))
	{}
	~LLCondition()
	{}
	void wait()
	{
#if IS_LLCOMMON_INLINE
		if (AIThreadID::in_main_thread_inline())
#else
		if (AIThreadID::in_main_thread())
#endif
			wait_main();
		else LLConditionVariableImpl::wait(*this);
	}
	void signal()		{ LLConditionVariableImpl::notify_one(); }
	void broadcast()	{ LLConditionVariableImpl::notify_all(); }
private:
	LL_COMMON_API void wait_main();
};
class LLMutexLock
{
public:
	LLMutexLock(LLMutex* mutex)
	{
		mMutex = mutex;
		lock();
	}
	LLMutexLock(LLMutex& mutex)
	{
		mMutex = &mutex;
		lock();
	}
	~LLMutexLock()
	{
		if (mMutex) mMutex->unlock();
	}
private:
	LL_COMMON_API void lock();
	LLMutex* mMutex;
};
class AIRWLock
{
public:
	AIRWLock(LLAPRPool& parent = LLThread::tldata().mRootPool) :
		mWriterWaitingMutex(MUTEX_POOL(parent)), mNoHoldersCondition(MUTEX_POOL(parent)), mHoldersCount(0), mWriterIsWaiting(false) { }
private:
	LLMutex mWriterWaitingMutex;
	LLCondition mNoHoldersCondition;
	int mHoldersCount;
    bool volatile mWriterIsWaiting;
public:
	void rdlock(bool high_priority = false)
	{
		if (mWriterIsWaiting && !high_priority)
		{
			mWriterWaitingMutex.lock();
			mWriterWaitingMutex.unlock();
		}
		mNoHoldersCondition.lock();
		while (mHoldersCount == -1)
		{
		  	mNoHoldersCondition.wait();
		}
		++mHoldersCount;
		mNoHoldersCondition.unlock();
	}
	void rdunlock(void)
	{
		mNoHoldersCondition.lock();
		if (--mHoldersCount == 0)
		{
			mNoHoldersCondition.signal();
		}
		mNoHoldersCondition.unlock();
	}
	void wrlock(void)
	{
		mWriterWaitingMutex.lock();
		mWriterIsWaiting = true;
		mNoHoldersCondition.lock();
		while (mHoldersCount != 0)
		{
		  	mNoHoldersCondition.wait();
		}
		mWriterIsWaiting = false;
		mWriterWaitingMutex.unlock();
		mHoldersCount = -1;
		mNoHoldersCondition.unlock();
	}
	void wrunlock(void)
	{
		mNoHoldersCondition.lock();
		mHoldersCount = 0;
		mNoHoldersCondition.signal();
		mNoHoldersCondition.unlock();
	}
	void rd2wrlock(void)
	{
		mNoHoldersCondition.lock();
		if (--mHoldersCount > 0)
		{
			mWriterWaitingMutex.lock();
			mWriterIsWaiting = true;
			while (mHoldersCount != 0)
			{
				mNoHoldersCondition.wait();
			}
			mWriterIsWaiting = false;
			mWriterWaitingMutex.unlock();
		}
		mHoldersCount = -1;
		mNoHoldersCondition.unlock();
	}
	void wr2rdlock(void)
	{
		mNoHoldersCondition.lock();
		mHoldersCount = 1;
		mNoHoldersCondition.signal();
		mNoHoldersCondition.unlock();
	}
#if LL_DEBUG
	bool isLocked(void)
	{
		mNoHoldersCondition.lock();
		bool res = mHoldersCount;
		mNoHoldersCondition.unlock();
		return res;
	}
#endif
};
#if LL_DEBUG
class AINRLock
{
private:
	int read_locked;
	int write_locked;
	mutable bool mAccessed;
	mutable AIThreadID mTheadID;
	void accessed(void) const
	{
		if (!mAccessed)
		{
			mAccessed = true;
			mTheadID.reset();
		}
		else
		{
			llassert_always(mTheadID.equals_current_thread());
		}
	}
public:
	AINRLock(void) : read_locked(false), write_locked(false), mAccessed(false) { }
	bool isLocked() const { return read_locked || write_locked; }
	void rdlock(bool high_priority = false) { accessed(); ++read_locked; }
	void rdunlock() { --read_locked; }
	void wrlock() { llassert(!isLocked()); accessed(); ++write_locked; }
	void wrunlock() { --write_locked; }
	void wr2rdlock() { llassert(false); }
	void rd2wrlock() { llassert(false); }
};
#endif
void LLThread::lockData()
{
	mRunCondition->lock();
}
void LLThread::unlockData()
{
	mRunCondition->unlock();
}
class LLThreadSafeRefCount
{
private:
	LLThreadSafeRefCount(const LLThreadSafeRefCount&);
	LLThreadSafeRefCount&operator=(const LLThreadSafeRefCount&);
protected:
	virtual ~LLThreadSafeRefCount()
	{
		if (mRef != 0)
		{
			LL_ERRS() << "deleting non-zero reference" << LL_ENDL;
		}
	}
public:
	LLThreadSafeRefCount() : mRef(0)
	{}
	void ref()
	{
		mRef++;
	}
	void unref()
	{
		llassert(mRef > 0);
		if (--mRef == 0) delete this;
	}
	S32 getNumRefs() const
	{
		return mRef;
	}
private:
	LLAtomicS32	mRef;
};
class LLResponder : public LLThreadSafeRefCount
{
protected:
	virtual ~LLResponder() {}
public:
	virtual void completed(bool success) = 0;
};
#endif
