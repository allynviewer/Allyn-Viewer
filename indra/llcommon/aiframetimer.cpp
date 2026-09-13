/**
 * @file aiframetimer.cpp
 *
 * Copyright (c) 2011, Aleric Inglewood.
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
 *   06/08/2011
 *   - Initial version, written by Aleric Inglewood @ SL
 */
#include "linden_common.h"
#include "aiframetimer.h"
static F64 const NEVER = 1e16;
F64 AIFrameTimer::sNextExpiration;
AIFrameTimer::timer_list_type AIFrameTimer::sTimerList;
LLGlobalMutex AIFrameTimer::sMutex;
void AIFrameTimer::create(F64 expiration, signal_type::slot_type const& slot)
{
	AIRunningFrameTimer new_timer(expiration, this);
	LLMutexLock lock(sMutex);
	llassert(mHandle.mRunningTimer == sTimerList.end());
	mHandle.init(sTimerList.insert(new_timer), slot);
	sNextExpiration = sTimerList.begin()->expiration();
}
void AIFrameTimer::cancel(void)
{
	mHandle.mMutex.lock();
	{
		LLMutexLock lock(sMutex);
		if (mHandle.mRunningTimer != sTimerList.end())
		{
			sTimerList.erase(mHandle.mRunningTimer);
			mHandle.mRunningTimer = sTimerList.end();
			sNextExpiration = sTimerList.empty() ? NEVER : sTimerList.begin()->expiration();
		}
	}
    mHandle.mMutex.unlock();
}
void AIFrameTimer::handleExpiration(F64 current_frame_time)
{
    sMutex.lock();
	for(;;)
	{
		if (sTimerList.empty())
		{
			sNextExpiration = NEVER;
			break;
		}
		timer_list_type::iterator running_timer = sTimerList.begin();
		sNextExpiration = running_timer->expiration();
		if (sNextExpiration > current_frame_time)
		{
			break;
		}
		Handle& handle(running_timer->getTimer()->mHandle);
		llassert_always(running_timer == handle.mRunningTimer);
		handle.mRunningTimer = sTimerList.end();
		if (handle.mMutex.try_lock())
		{
			sMutex.unlock();
			running_timer->do_callback();
			sMutex.lock();
			handle.mMutex.unlock();
		}
		sTimerList.erase(running_timer);
	}
    sMutex.unlock();
}
