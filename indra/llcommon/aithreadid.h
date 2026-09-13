/**
 * @file aithreadid.h
 * @brief Declaration of AIThreadID.
 *
 * Copyright (c) 2012, Aleric Inglewood.
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
 *   08/08/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AI_THREAD_ID
#define AI_THREAD_ID
#include <apr_portable.h>
#include <iosfwd>
#include "llpreprocessor.h"
class AIThreadID
{
private:
	apr_os_thread_t mID;
	static LL_COMMON_API apr_os_thread_t sMainThreadID;
	static LL_COMMON_API apr_os_thread_t const undefinedID;
#ifndef LL_DARWIN
	static ll_thread_local apr_os_thread_t lCurrentThread;
#endif
public:
	static LL_COMMON_API AIThreadID const sNone;
	enum undefined_thread_t { none };
public:
	AIThreadID(void) : mID(apr_os_thread_current()) { }
	explicit AIThreadID(undefined_thread_t) : mID(undefinedID) { }
	AIThreadID(AIThreadID const& id) : mID(id.mID) { }
	AIThreadID& operator=(AIThreadID const& id) { mID = id.mID; return *this; }
	bool is_main_thread(void) const { return apr_os_thread_equal(mID, sMainThreadID); }
	bool is_no_thread(void) const { return apr_os_thread_equal(mID, sNone.mID); }
	friend LL_COMMON_API bool operator==(AIThreadID const& id1, AIThreadID const& id2) { return apr_os_thread_equal(id1.mID, id2.mID); }
	friend LL_COMMON_API bool operator!=(AIThreadID const& id1, AIThreadID const& id2) { return !apr_os_thread_equal(id1.mID, id2.mID); }
	friend LL_COMMON_API std::ostream& operator<<(std::ostream& os, AIThreadID const& id);
	static void set_main_thread_id(void);
	static void set_current_thread_id(void);
#ifndef LL_DARWIN
	LL_COMMON_API void clear(void);
	LL_COMMON_API void reset(void);
	LL_COMMON_API bool equals_current_thread(void) const;
	LL_COMMON_API static bool in_main_thread(void);
	LL_COMMON_API static apr_os_thread_t getCurrentThread(void);
	void reset_inline(void) { mID = lCurrentThread; }
	bool equals_current_thread_inline(void) const { return apr_os_thread_equal(mID, lCurrentThread); }
	static bool in_main_thread_inline(void) { return apr_os_thread_equal(lCurrentThread, sMainThreadID); }
	static apr_os_thread_t getCurrentThread_inline(void) { return lCurrentThread; }
#else
	void clear(void) { mID = undefinedID; }
	void reset(void) { mID = apr_os_thread_current(); }
	void reset_inline(void) { mID = apr_os_thread_current(); }
	bool equals_current_thread(void) const { return apr_os_thread_equal(mID, apr_os_thread_current()); }
	bool equals_current_thread_inline(void) const { return apr_os_thread_equal(mID, apr_os_thread_current()); }
	static bool in_main_thread(void) { return apr_os_thread_equal(apr_os_thread_current(), sMainThreadID); }
	static bool in_main_thread_inline(void) { return apr_os_thread_equal(apr_os_thread_current(), sMainThreadID); }
	static apr_os_thread_t getCurrentThread(void) { return apr_os_thread_current(); }
	static apr_os_thread_t getCurrentThread_inline(void) { return apr_os_thread_current(); }
#endif
};
inline bool is_single_threaded(AIThreadID& thread_id)
{
  if (LL_UNLIKELY(thread_id.is_no_thread()))
  {
	thread_id.reset();
  }
  return thread_id.equals_current_thread();
}
inline bool is_main_thread(void)
{
  return AIThreadID::in_main_thread();
}
#endif
