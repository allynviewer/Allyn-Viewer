/** 
 * @file llthread.cpp
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
#if LL_GNUC
#pragma implementation "llthread.h"
#endif
#include "linden_common.h"
#include "llapr.h"
#include "apr_portable.h"
#include "llthread.h"
#include "lltimer.h"
#if LL_SOLARIS
#include <sched.h>
#endif
LLAtomicS32	LLThread::sCount = 0;
LLAtomicS32	LLThread::sRunning = 0;
LL_COMMON_API void assert_main_thread()
{
	if (!AIThreadID::in_main_thread_inline())
	{
		LL_ERRS() << "Illegal execution outside main thread." << LL_ENDL;
	}
}
void *APR_THREAD_FUNC LLThread::staticRun(apr_thread_t *apr_threadp, void *datap)
{
#ifdef CWDEBUG
  	debug::init_thread();
#endif
	LLThread *threadp = (LLThread *)datap;
	AIThreadID::set_current_thread_id();
	LLThreadLocalData::create(threadp);
	threadp->run();
	char const* volatile name = threadp->mName.c_str();
	--sRunning;
	threadp->terminated();
	LL_DEBUGS() << "LLThread::staticRun() Exiting: " << name << LL_ENDL;
	return NULL;
}
LLThread::LLThread(std::string const& name) :
	mPaused(false),
	mName(name),
	mAPRThreadp(NULL),
	mStatus(STOPPED),
	mThreadLocalData(NULL)
{
	sCount++;
	llassert(sCount <= 50);
	mRunCondition = new LLCondition;
}
LLThread::~LLThread()
{
	shutdown();
}
void LLThread::shutdown()
{
	if (mAPRThreadp)
	{
		if (!isStopped())
		{
			setQuitting();
			LL_INFOS() << "LLThread::shutdown() Killing thread " << mName << " Status: " << mStatus << LL_ENDL;
			S32 counter = 0;
			const S32 MAX_WAIT = 600;
			while (counter < MAX_WAIT)
			{
				if (isStopped())
				{
					break;
				}
				ms_sleep(100);
				yield();
				counter++;
			}
		}
		if (!isStopped())
		{
			LL_WARNS() << "LLThread::shutdown() exiting thread before clean exit!" << LL_ENDL;
			apr_thread_exit(mAPRThreadp, -1);
			return;
		}
		mAPRThreadp = NULL;
	}
	--sCount;
	delete mRunCondition;
	mRunCondition = 0;
}
void LLThread::start()
{
	llassert(isStopped());
	mStatus = RUNNING;
	sRunning++;
	apr_status_t status =
		apr_thread_create(&mAPRThreadp, NULL, staticRun, (void *)this, tldata().mRootPool());
	if(status == APR_SUCCESS)
	{
		apr_thread_detach(mAPRThreadp);
	}
	else
	{
		--sRunning;
		mStatus = STOPPED;
		LL_WARNS() << "failed to start thread " << mName << LL_ENDL;
		ll_apr_warn_status(status);
	}
}
void LLThread::pause()
{
	if (!mPaused)
	{
		mPaused = true;
	}
}
void LLThread::unpause()
{
	if (mPaused)
	{
		mPaused = false;
	}
	wake();
}
bool LLThread::runCondition(void)
{
	return true;
}
void LLThread::checkPause()
{
	mRunCondition->lock();
	while(shouldSleep())
	{
		mRunCondition->wait();
	}
 	mRunCondition->unlock();
}
void LLThread::setQuitting()
{
	mRunCondition->lock();
	if (mStatus == RUNNING)
	{
		mStatus = QUITTING;
	}
	mRunCondition->unlock();
	wake();
}
void LLThread::yield()
{
#if LL_SOLARIS
	sched_yield();
#else
	apr_thread_yield();
#endif
}
void LLThread::wake()
{
	mRunCondition->lock();
	if(!shouldSleep())
	{
		mRunCondition->signal();
	}
	mRunCondition->unlock();
}
void LLThread::wakeLocked()
{
	if(!shouldSleep())
	{
		mRunCondition->signal();
	}
}
apr_threadkey_t* LLThreadLocalData::sThreadLocalDataKey;
LLThreadLocalData::LLThreadLocalData(char const* name) : mCurlMultiHandle(NULL), mCurlErrorBuffer(NULL), mName(name)
{
}
LLThreadLocalData::~LLThreadLocalData()
{
  delete mCurlMultiHandle;
  delete [] mCurlErrorBuffer;
}
void LLThreadLocalData::init(void)
{
	if (sThreadLocalDataKey)
	{
		return;
	}
	AIThreadID::set_main_thread_id();
	AIThreadID::set_current_thread_id();
	apr_status_t status = apr_threadkey_private_create(&sThreadLocalDataKey, &LLThreadLocalData::destroy, LLAPRRootPool::get()());
	ll_apr_assert_status(status);
	LLThreadLocalData::create(NULL);
}
void LLThreadLocalData::destroy(void* thread_local_data)
{
	delete static_cast<LLThreadLocalData*>(thread_local_data);
}
void LLThreadLocalData::create(LLThread* threadp)
{
	LLThreadLocalData* new_tld = new LLThreadLocalData(threadp ? threadp->mName.c_str() : "main thread");
	if (threadp)
	{
		threadp->mThreadLocalData = new_tld;
	}
	apr_status_t status = apr_threadkey_private_set(new_tld, sThreadLocalDataKey);
	llassert_always(status == APR_SUCCESS);
}
LLThreadLocalData& LLThreadLocalData::tldata(void)
{
	if (!sThreadLocalDataKey)
	{
		LLThreadLocalData::init();
	}
	void* data;
	apr_status_t status = apr_threadkey_private_get(&data, sThreadLocalDataKey);
	llassert_always(status == APR_SUCCESS);
	return *static_cast<LLThreadLocalData*>(data);
}
#if defined(NEEDS_MUTEX_IMPL)
#if defined(USE_WIN32_THREAD)
LLMutexImpl::LLMutexImpl()
{
	InitializeCriticalSection(&mMutexImpl);
}
LLMutexImpl::~LLMutexImpl()
{
	DeleteCriticalSection(&mMutexImpl);
}
void LLMutexImpl::lock()
{
	EnterCriticalSection(&mMutexImpl);
}
void LLMutexImpl::unlock()
{
	LeaveCriticalSection(&mMutexImpl);
}
bool LLMutexImpl::try_lock()
{
	return !!TryEnterCriticalSection(&mMutexImpl);
}
LLConditionVariableImpl::LLConditionVariableImpl()
{
	InitializeConditionVariable(&mConditionVariableImpl);
}
LLConditionVariableImpl::~LLConditionVariableImpl()
{
}
void LLConditionVariableImpl::notify_one()
{
	WakeConditionVariable(&mConditionVariableImpl);
}
void LLConditionVariableImpl::notify_all()
{
	WakeAllConditionVariable(&mConditionVariableImpl);
}
void LLConditionVariableImpl::wait(LLMutex& lock)
{
	LLMutex::ImplAdoptMutex impl_adopted_mutex(lock);
	SleepConditionVariableCS(&mConditionVariableImpl, &lock.native_handle(), INFINITE);
}
#else
void APRExceptionThrower(apr_status_t status)
{
	if(status != APR_SUCCESS)
	{
		static char buf[256];
		throw std::logic_error(apr_strerror(status,buf,sizeof(buf)));
	}
}
LLMutexImpl::LLMutexImpl(native_pool_type& pool) : mPool(pool), mMutexImpl(NULL)
{
	APRExceptionThrower(apr_thread_mutex_create(&mMutexImpl, APR_THREAD_MUTEX_UNNESTED, mPool()));
}
LLMutexImpl::~LLMutexImpl()
{
	APRExceptionThrower(apr_thread_mutex_destroy(mMutexImpl));
	mMutexImpl = NULL;
}
void LLMutexImpl::lock()
{
	APRExceptionThrower(apr_thread_mutex_lock(mMutexImpl));
}
void LLMutexImpl::unlock()
{
	APRExceptionThrower(apr_thread_mutex_unlock(mMutexImpl));
}
bool LLMutexImpl::try_lock()
{
	apr_status_t status = apr_thread_mutex_trylock(mMutexImpl);
	if(APR_STATUS_IS_EBUSY(status))
		return false;
	APRExceptionThrower(status);
	return true;
}
LLConditionVariableImpl::LLConditionVariableImpl(native_pool_type& pool) : mPool(pool), mConditionVariableImpl(NULL)
{
	APRExceptionThrower(apr_thread_cond_create(&mConditionVariableImpl, mPool()));
}
LLConditionVariableImpl::~LLConditionVariableImpl()
{
	APRExceptionThrower(apr_thread_cond_destroy(mConditionVariableImpl));
}
void LLConditionVariableImpl::notify_one()
{
	APRExceptionThrower(apr_thread_cond_signal(mConditionVariableImpl));
}
void LLConditionVariableImpl::notify_all()
{
	APRExceptionThrower(apr_thread_cond_broadcast(mConditionVariableImpl));
}
void LLConditionVariableImpl::wait(LLMutex& lock)
{
	LLMutex::ImplAdoptMutex impl_adopted_mutex(lock);
	APRExceptionThrower(apr_thread_cond_wait(mConditionVariableImpl, lock.native_handle()));
}
#endif
#endif
LLTrace::BlockTimerStatHandle FT_WAIT_FOR_MUTEX("LLMutex::lock()");
void LLMutex::lock_main(LLTrace::BlockTimerStatHandle* timer)
{
	llassert(!isSelfLocked());
	LL_RECORD_BLOCK_TIME(timer ? *timer : FT_WAIT_FOR_MUTEX);
	LLMutexImpl::lock();
}
LLTrace::BlockTimerStatHandle FT_WAIT_FOR_CONDITION("LLCondition::wait()");
void LLCondition::wait_main()
{
	llassert(isSelfLocked());
	LL_RECORD_BLOCK_TIME(FT_WAIT_FOR_CONDITION);
	LLConditionVariableImpl::wait(*this);
	llassert(isSelfLocked());
}
LLTrace::BlockTimerStatHandle FT_WAIT_FOR_MUTEXLOCK("LLMutexLock::lock()");
void LLMutexLock::lock()
{
	if (mMutex)
	{
		mMutex->lock(&FT_WAIT_FOR_MUTEXLOCK);
	}
}