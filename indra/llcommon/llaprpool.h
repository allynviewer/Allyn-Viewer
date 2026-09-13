/**
 * @file llaprpool.h
 * @brief Implementation of LLAPRPool.
 *
 * Copyright (c) 2010, Aleric Inglewood.
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
 *   04/04/2010
 *   - Initial version, written by Aleric Inglewood @ SL
 *
 *   10/11/2010
 *   - Changed filename, class names and license to a more
 *     company-neutral format.
 *   - Added APR_HAS_THREADS #if's to allow creation and destruction
 *     of subpools by threads other than the parent pool owner.
 */
#ifndef LL_LLAPRPOOL_H
#define LL_LLAPRPOOL_H
#ifdef LL_WINDOWS
#pragma warning(push)
#pragma warning(disable:4996)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning(pop)
#endif
#include "apr_portable.h"
#include "apr_pools.h"
#include "llatomic.h"
#include "llerror.h"
#include "aithreadid.h"
extern void ll_init_apr();
class LL_COMMON_API LLAPRPool
{
protected:
	apr_pool_t* mPool;
	LLAPRPool* mParent;
	AIThreadID mOwner;
public:
	LLAPRPool(void) : mPool(NULL), mOwner(AIThreadID::none) { }
	LLAPRPool(LLAPRPool& parent) : mPool(NULL), mOwner(AIThreadID::none) { create(parent); }
	~LLAPRPool() { destroy(); }
protected:
	LLAPRPool(int) : mPool(NULL), mParent(NULL)
	{
		apr_status_t const apr_pool_create_status = apr_pool_create(&mPool, NULL);
		llassert_always(apr_pool_create_status == APR_SUCCESS);
		llassert(mPool);
		apr_pool_cleanup_register(mPool, this, &s_plain_cleanup, &apr_pool_cleanup_null);
	}
public:
	void create(LLAPRPool& parent = *static_cast<LLAPRPool*>(NULL));
	void destroy(void);
	typedef LLAPRPool* const LLAPRPool::* const bool_type;
	operator bool_type() const { return mPool ? &LLAPRPool::mParent : 0; }
	apr_pool_t* operator()(void) const
	{
		if (mParent)
		{
			llassert(mPool);
			llassert(mOwner.equals_current_thread());
		}
		return mPool;
	}
	void clear(void)
	{
		llassert(mPool);
		llassert(mOwner.equals_current_thread());
		apr_pool_clear(mPool);
	}
#if 0
	void* palloc(size_t size)
	{
		llassert(mPool);
		llassert(mOwner.equals_current_thread());
		return apr_palloc(mPool, size);
	}
	void* pcalloc(size_t size)
	{
		llassert(mPool);
		llassert(mOwner.equals_current_thread());
		return apr_pcalloc(mPool, size);
	}
#endif
private:
	bool parent_is_being_destructed(void);
	static apr_status_t s_plain_cleanup(void* userdata) { return static_cast<LLAPRPool*>(userdata)->plain_cleanup(); }
	apr_status_t plain_cleanup(void)
	{
		if (mPool &&
			parent_is_being_destructed())
		{
			mPool = NULL;
		}
		return APR_SUCCESS;
	}
};
class LLAPRInitialization
{
public:
	LLAPRInitialization(void);
};
class LL_COMMON_API LLAPRRootPool : public LLAPRInitialization, public LLAPRPool
{
private:
	friend class LLThreadLocalData;
	friend class AIThreadSafeSimpleDCRootPool_pbase;
#if !APR_HAS_THREADS
	friend class LLMutexRootPool;
#endif
	LLAPRRootPool(void);
	~LLAPRRootPool();
private:
	static bool sCountInitialized;
	static LLAtomicS32 sCount;
public:
	static LLAPRRootPool& get(void);
#if APR_POOL_DEBUG
	void grab_ownership(void)
	{
		apr_pool_owner_set(mPool);
	}
#endif
private:
	LLAPRRootPool(int) : LLAPRInitialization(), LLAPRPool(0) { }
};
class LL_COMMON_API LLVolatileAPRPool : protected LLAPRPool
{
public:
	LLVolatileAPRPool(void) : mNumActiveRef(0), mNumTotalRef(0) { }
	void clearVolatileAPRPool(void);
	bool isOld(void) const { return mNumTotalRef > FULL_VOLATILE_APR_POOL; }
	bool isUnused() const { return mNumActiveRef == 0; }
private:
	friend class LLScopedVolatileAPRPool;
	friend class LLAPRFile;
	apr_pool_t* getVolatileAPRPool(void)
	{
		if (!mPool) create();
		++mNumActiveRef;
		++mNumTotalRef;
		return LLAPRPool::operator()();
	}
private:
	S32 mNumActiveRef;
	S32 mNumTotalRef;
	static S32 const FULL_VOLATILE_APR_POOL = 1024;
};
#endif
