/**
 * @file aithreadsafe.h
 * @brief Implementation of AIThreadSafe, AIReadAccessConst, AIReadAccess and AIWriteAccess.
 *
 * Copyright (c) 2010 - 2013, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   31/03/2010
 *   Initial version, written by Aleric Inglewood @ SL
 *
 *   14/03/2012
 *   Added AIThreadSafeSingleThread and friends.
 *   Added AIAccessConst (and derived AIAccess from it) to allow read
 *   access to a const AIThreadSafeSimple.
 *
 *   26/01/2013
 *   Added support for LLCondition to AIThreadSafeSimple.
 */
#ifndef AITHREADSAFE_H
#define AITHREADSAFE_H
#include <new>
#include "llthread.h"
#include "llerror.h"
template<typename T, typename RWLOCK> struct AIReadAccessConst;
template<typename T, typename RWLOCK> struct AIReadAccess;
template<typename T, typename RWLOCK> struct AIWriteAccess;
template<typename T, typename MUTEX> struct AIAccessConst;
template<typename T, typename MUTEX> struct AIAccess;
template<typename T> struct AISTAccessConst;
template<typename T> struct AISTAccess;
template<typename T>
struct AIThreadSafeBitsPOD
{
	long mMemory[(sizeof(T) + sizeof(long) - 1) / sizeof(long)];
};
template<typename T>
class AIThreadSafeBits : private AIThreadSafeBitsPOD<T>
{
public:
	~AIThreadSafeBits() { ptr()->~T(); }
	void* memory() const { return const_cast<long*>(&AIThreadSafeBitsPOD<T>::mMemory[0]); }
	template<typename T2>
	  static AIThreadSafeBits<T2>* wrapper_cast(T2* ptr)
	      { return static_cast<AIThreadSafeBits<T2>*>(reinterpret_cast<AIThreadSafeBitsPOD<T2>*>(reinterpret_cast<char*>(ptr) - offsetof(AIThreadSafeBitsPOD<T2>, mMemory[0]))); }
	template<typename T2>
	  static AIThreadSafeBits<T2> const* wrapper_cast(T2 const* ptr)
	      { return static_cast<AIThreadSafeBits<T2> const*>(reinterpret_cast<AIThreadSafeBitsPOD<T2> const*>(reinterpret_cast<char const*>(ptr) - offsetof(AIThreadSafeBitsPOD<T2>, mMemory[0]))); }
protected:
	T const* ptr() const { return reinterpret_cast<T const*>(AIThreadSafeBitsPOD<T>::mMemory); }
	T* ptr() { return reinterpret_cast<T*>(AIThreadSafeBitsPOD<T>::mMemory); }
};
template<typename T, typename RWLOCK = AIRWLock>
class AIThreadSafe : public AIThreadSafeBits<T>
{
protected:
	friend struct AIReadAccessConst<T, RWLOCK>;
	friend struct AIReadAccess<T, RWLOCK>;
	friend struct AIWriteAccess<T, RWLOCK>;
	RWLOCK mRWLock;
	AIThreadSafe(void) { }
	MUTEX_POOL(AIThreadSafe(LLAPRPool& parent) : mRWLock(parent){ })
public:
	AIThreadSafe(T* object)
	{
#if !LL_WINDOWS
		llassert(object == AIThreadSafeBits<T>::ptr());
#endif
	}
#if LL_DEBUG
	~AIThreadSafe() { llassert(!mRWLock.isLocked()); }
#endif
};
#define AITHREADSAFE(type, var, paramlist) AIThreadSafe<type> var(new (var.memory()) type paramlist)
template<typename T, typename RWLOCK = AIRWLock>
class AIThreadSafeDC : public AIThreadSafe<T, RWLOCK>
{
public:
	AIThreadSafeDC(void) { new (AIThreadSafe<T, RWLOCK>::ptr()) T; }
	template<typename T2> AIThreadSafeDC(T2 const& val) { new (AIThreadSafe<T, RWLOCK>::ptr()) T(val); }
};
template<typename T, typename RWLOCK = AIRWLock>
struct AIReadAccessConst
{
	enum state_type
	{
		readlocked,
		read2writelocked,
		writelocked,
		write2writelocked
	};
	AIReadAccessConst(AIThreadSafe<T, RWLOCK> const& wrapper, bool high_priority = false)
		: mWrapper(const_cast<AIThreadSafe<T, RWLOCK>&>(wrapper)),
		  mState(readlocked)
		{
			mWrapper.mRWLock.rdlock(high_priority);
		}
	~AIReadAccessConst()
		{
			if (mState == readlocked)
				mWrapper.mRWLock.rdunlock();
			else if (mState == writelocked)
				mWrapper.mRWLock.wrunlock();
			else if (mState == read2writelocked)
				mWrapper.mRWLock.wr2rdlock();
		}
	T const* operator->() const { return mWrapper.ptr(); }
	T const& operator*() const { return *mWrapper.ptr(); }
protected:
	AIReadAccessConst(AIThreadSafe<T, RWLOCK>& wrapper, state_type state)
		: mWrapper(wrapper), mState(state)
		{ }
	AIThreadSafe<T, RWLOCK>& mWrapper;
	state_type const mState;
private:
	AIReadAccessConst(AIReadAccessConst const&);
};
template<typename T, typename RWLOCK = AIRWLock>
struct AIReadAccess : public AIReadAccessConst<T, RWLOCK>
{
	typedef typename AIReadAccessConst<T, RWLOCK>::state_type state_type;
	using AIReadAccessConst<T, RWLOCK>::readlocked;
	AIReadAccess(AIThreadSafe<T, RWLOCK>& wrapper, bool high_priority = false) : AIReadAccessConst<T, RWLOCK>(wrapper, readlocked) { this->mWrapper.mRWLock.rdlock(high_priority); }
protected:
	AIReadAccess(AIThreadSafe<T, RWLOCK>& wrapper, state_type state) : AIReadAccessConst<T, RWLOCK>(wrapper, state) { }
	friend struct AIWriteAccess<T, RWLOCK>;
};
template<typename T, typename RWLOCK = AIRWLock>
struct AIWriteAccess : public AIReadAccess<T, RWLOCK>
{
	using AIReadAccessConst<T, RWLOCK>::readlocked;
	using AIReadAccessConst<T, RWLOCK>::read2writelocked;
	using AIReadAccessConst<T, RWLOCK>::writelocked;
	using AIReadAccessConst<T, RWLOCK>::write2writelocked;
	AIWriteAccess(AIThreadSafe<T, RWLOCK>& wrapper) : AIReadAccess<T, RWLOCK>(wrapper, writelocked) { this->mWrapper.mRWLock.wrlock();}
	explicit AIWriteAccess(AIReadAccess<T, RWLOCK>& access)
		: AIReadAccess<T, RWLOCK>(access.mWrapper, (access.mState == readlocked) ? read2writelocked : write2writelocked)
		{
			if (this->mState == read2writelocked)
			{
				this->mWrapper.mRWLock.rd2wrlock();
			}
		}
	T* operator->() const { return this->mWrapper.ptr(); }
	T& operator*() const { return *this->mWrapper.ptr(); }
};
template<typename T, typename MUTEX = LLMutex>
class AIThreadSafeSimple : public AIThreadSafeBits<T>
{
protected:
	friend struct AIAccessConst<T, MUTEX>;
	friend struct AIAccess<T, MUTEX>;
	MUTEX mMutex;
	friend struct AIRegisteredStateMachinesList;
	AIThreadSafeSimple(void) { }
	MUTEX_POOL(AIThreadSafeSimple(LLAPRPool& parent) : mMutex(parent) { })
public:
	AIThreadSafeSimple(T* object) { llassert(object == AIThreadSafeBits<T>::ptr()); }
#if LL_DEBUG
	~AIThreadSafeSimple() { llassert(!mMutex.isLocked()); }
#endif
};
#define AITHREADSAFESIMPLE(type, var, paramlist) AIThreadSafeSimple<type> var(new (var.memory()) type paramlist)
#define AITHREADSAFESIMPLECONDITION(type, var, paramlist) AIThreadSafeSimple<type, LLCondition> var(new (var.memory()) type paramlist)
template<typename T, typename MUTEX = LLMutex>
class AIThreadSafeSimpleDC : public AIThreadSafeSimple<T, MUTEX>
{
public:
	AIThreadSafeSimpleDC(void) { new (AIThreadSafeSimple<T, MUTEX>::ptr()) T; }
	template<typename T2> AIThreadSafeSimpleDC(T2 const& val) { new (AIThreadSafeSimple<T, MUTEX>::ptr()) T(val); }
protected:
	AIThreadSafeSimpleDC(LLAPRRootPool& parent) : AIThreadSafeSimple<T, MUTEX>(MUTEX_POOL(parent)) { new (AIThreadSafeSimple<T, MUTEX>::ptr()) T; }
};
class AIThreadSafeSimpleDCRootPool_pbase
{
protected:
	LLAPRRootPool mRootPool;
private:
	template<typename T> friend class AIThreadSafeSimpleDCRootPool;
	AIThreadSafeSimpleDCRootPool_pbase(void) { }
};
template<typename T>
class AIThreadSafeSimpleDCRootPool : private AIThreadSafeSimpleDCRootPool_pbase, public AIThreadSafeSimpleDC<T>
{
public:
	AIThreadSafeSimpleDCRootPool(void) :
		AIThreadSafeSimpleDCRootPool_pbase(),
		AIThreadSafeSimpleDC<T>(MUTEX_POOL(mRootPool)) { }
};
template<typename T, typename MUTEX = LLMutex>
struct AIAccessConst
{
	AIAccessConst(AIThreadSafeSimple<T, MUTEX> const& wrapper) : mWrapper(const_cast<AIThreadSafeSimple<T, MUTEX>&>(wrapper))
	{
	  this->mWrapper.mMutex.lock();
	}
	T const* operator->() const { return this->mWrapper.ptr(); }
	T const& operator*() const { return *this->mWrapper.ptr(); }
	~AIAccessConst()
	{
	  this->mWrapper.mMutex.unlock();
	}
	void wait() { this->mWrapper.mMutex.wait(); }
	void signal() { this->mWrapper.mMutex.signal(); }
protected:
	AIThreadSafeSimple<T, MUTEX>& mWrapper;
private:
	AIAccessConst(AIAccessConst const&);
};
template<typename T, typename MUTEX = LLMutex>
struct AIAccess : public AIAccessConst<T, MUTEX>
{
	AIAccess(AIThreadSafeSimple<T, MUTEX>& wrapper) : AIAccessConst<T, MUTEX>(wrapper) { }
	T* operator->() const { return this->mWrapper.ptr(); }
	T& operator*() const { return *this->mWrapper.ptr(); }
};
template<typename T>
class AIThreadSafeSingleThread : public AIThreadSafeBits<T>
{
protected:
	friend struct AISTAccessConst<T>;
	friend struct AISTAccess<T>;
	AIThreadSafeSingleThread(void)
#ifdef LL_DEBUG
	  : mAccessed(false)
#endif
	{ }
#ifdef LL_DEBUG
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
#endif
public:
	AIThreadSafeSingleThread(T* object)
#ifdef LL_DEBUG
	  : mAccessed(false)
#endif
	{
	  llassert(object == AIThreadSafeBits<T>::ptr());
	}
private:
	AIThreadSafeSingleThread(AIThreadSafeSingleThread const&);
	void operator=(AIThreadSafeSingleThread const&);
};
template<typename T>
class AIThreadSafeSingleThreadDC : public AIThreadSafeSingleThread<T>
{
public:
	AIThreadSafeSingleThreadDC(void) { new (AIThreadSafeSingleThread<T>::ptr()) T; }
	template<typename T2> AIThreadSafeSingleThreadDC(T2 const& val) { new (AIThreadSafeSingleThread<T>::ptr()) T(val); }
	AIThreadSafeSingleThreadDC& operator=(T const& val) { AIThreadSafeSingleThread<T>::accessed(); *AIThreadSafeSingleThread<T>::ptr() = val; return *this; }
	friend std::ostream& operator<<(std::ostream& os, AIThreadSafeSingleThreadDC const& wrapped_val) { wrapped_val.accessed(); return os << *wrapped_val.ptr(); }
	operator T&(void) { AIThreadSafeSingleThread<T>::accessed(); return *AIThreadSafeSingleThread<T>::ptr(); }
	operator T const&(void) const { AIThreadSafeSingleThread<T>::accessed(); return *AIThreadSafeSingleThread<T>::ptr(); }
};
#define AITHREADSAFESINGLETHREAD(type, var, paramlist) AIThreadSafeSingleThread<type> var(new (var.memory()) type paramlist)
template<typename T>
struct AISTAccessConst
{
	AISTAccessConst(AIThreadSafeSingleThread<T> const& wrapper) : mWrapper(const_cast<AIThreadSafeSingleThread<T>&>(wrapper))
	{
#if LL_DEBUG
	  wrapper.accessed();
#endif
	}
	T const* operator->() const { return this->mWrapper.ptr(); }
	T const& operator*() const { return *this->mWrapper.ptr(); }
protected:
	AIThreadSafeSingleThread<T>& mWrapper;
private:
	AISTAccessConst(AISTAccessConst const&);
};
template<typename T>
struct AISTAccess : public AISTAccessConst<T>
{
	AISTAccess(AIThreadSafeSingleThread<T>& wrapper) : AISTAccessConst<T>(wrapper)
	{
#if LL_DEBUG
	  wrapper.accessed();
#endif
	}
	T* operator->() const { return this->mWrapper.ptr(); }
	T& operator*() const { return *this->mWrapper.ptr(); }
};
#endif
