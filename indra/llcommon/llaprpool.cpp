/**
 * @file llaprpool.cpp
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
#include "linden_common.h"
#include "llerror.h"
#include "llaprpool.h"
#include "llatomic.h"
#include "llthread.h"
void LLAPRPool::create(LLAPRPool& parent)
{
	llassert(!mPool);
	mParent = &parent;
	if (!mParent)
	{
		mParent = &LLThreadLocalData::tldata().mRootPool;
	}
	llassert(mParent->mPool);
#if APR_HAS_THREADS
	mOwner.reset_inline();
#else
	mOwner = mParent->mOwner;
	llassert(mOwner.equals_current_thread_inline());
#endif
	apr_status_t const apr_pool_create_status = apr_pool_create(&mPool, mParent->mPool);
	llassert_always(apr_pool_create_status == APR_SUCCESS);
	llassert(mPool);
	apr_pool_cleanup_register(mPool, this, &s_plain_cleanup, &apr_pool_cleanup_null);
}
void LLAPRPool::destroy(void)
{
	if (mPool)
	{
#if !APR_HAS_THREADS
		llassert(!mParent || mParent->mOwner.equals_current_thread_inline());
#endif
		apr_pool_t* pool = mPool;
		mPool = NULL;
		apr_pool_cleanup_kill(pool, this, &s_plain_cleanup);
		apr_pool_destroy(pool);
	}
}
bool LLAPRPool::parent_is_being_destructed(void)
{
	return mParent && (!mParent->mPool || mParent->parent_is_being_destructed());
}
LLAPRInitialization::LLAPRInitialization(void)
{
	static bool apr_initialized = false;
	if (!apr_initialized)
	{
		apr_initialize();
	}
	apr_initialized = true;
}
bool LLAPRRootPool::sCountInitialized = false;
LLAtomicS32 LLAPRRootPool::sCount;
LLAPRRootPool::LLAPRRootPool(void) : LLAPRInitialization(), LLAPRPool(0)
{
	if (!sCountInitialized)
	{
#ifdef NEEDS_APR_ATOMICS
		apr_status_t status = apr_atomic_init(mPool);
		llassert_always(status == APR_SUCCESS);
#endif
		sCount = 1;
		sCountInitialized = true;
		LLThreadLocalData::init();
	}
	sCount++;
}
LLAPRRootPool::~LLAPRRootPool()
{
	if (!--sCount)
	{
		LL_INFOS("APR") << "Cleaning up APR" << LL_ENDL;
		static_cast<LLAPRRootPool*>(this)->destroy();
		apr_terminate();
	}
}
LLAPRRootPool& LLAPRRootPool::get(void)
{
  static LLAPRRootPool global_APRpool(0);
  return global_APRpool;
}
void LLVolatileAPRPool::clearVolatileAPRPool()
{
	llassert_always(mNumActiveRef > 0);
	if (--mNumActiveRef == 0)
	{
		if (isOld())
		{
			destroy();
			mNumTotalRef = 0 ;
		}
		else
		{
			clear();
		}
	}
	llassert(mNumTotalRef < (FULL_VOLATILE_APR_POOL << 2)) ;
}
