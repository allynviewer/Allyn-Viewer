/** 
 * @file llframetimer.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "linden_common.h"
#include "u64.h"
#include "llframetimer.h"
#include "aiframetimer.h"
#include "llaprpool.h"
static F64 const USEC_PER_SECOND = 1000000.0;
static F64 const USEC_TO_SEC_F64 = 0.000001;
static F64 const NEVER = 1e16;
U64 const LLFrameTimer::sStartTotalTime = totalTime();
U64 LLFrameTimer::sTotalTime = LLFrameTimer::sStartTotalTime;
F64 LLFrameTimer::sTotalSeconds =
		U64_to_F64(LLFrameTimer::sTotalTime) * USEC_TO_SEC_F64;
F64 LLFrameTimer::sFrameTime = 0.0;
U64 LLFrameTimer::sFrameCount = 0;
U64 LLFrameTimer::sPrevTotalTime = LLFrameTimer::sStartTotalTime;
U64 LLFrameTimer::sFrameDeltaTime = 0;
LLGlobalMutex LLFrameTimer::sGlobalMutex;
bool LLFrameTimer::sFirstFrameTimerCreated;
void LLFrameTimer::global_initialization(void)
{
	sFirstFrameTimerCreated = true;
	AIFrameTimer::sNextExpiration = NEVER;
}
void LLFrameTimer::updateFrameTime(void)
{
	llassert(is_main_thread());
	sTotalTime = totalTime();
	sTotalSeconds = U64_to_F64(sTotalTime) * USEC_TO_SEC_F64;
	F64 new_frame_time = U64_to_F64(sTotalTime - sStartTotalTime) * USEC_TO_SEC_F64;
	sGlobalMutex.lock();
	sFrameTime = new_frame_time;
	sGlobalMutex.unlock();
}
void LLFrameTimer::updateFrameTimeAndCount(void)
{
	updateFrameTime();
	sFrameDeltaTime = sTotalTime - sPrevTotalTime;
	sPrevTotalTime = sTotalTime;
	++sFrameCount;
	if (AIFrameTimer::sNextExpiration <= sFrameTime)
	{
		AIFrameTimer::handleExpiration(sFrameTime);
	}
}
void LLFrameTimer::pause(void)
{
	llassert(is_main_thread());
	if (!mPaused)
	{
		mStartTime = sFrameTime - mStartTime;
	}
	mPaused = true;
}
void LLFrameTimer::unpause(void)
{
	llassert(is_main_thread());
	if (mPaused)
	{
		mStartTime = sFrameTime - mStartTime;
	}
	mPaused = false;
}
void LLFrameTimer::setExpiryAt(F64 seconds_since_epoch)
{
	llassert(is_main_thread());
	llassert(!mPaused);
	mStartTime = sFrameTime;
	mExpiry = seconds_since_epoch - (USEC_TO_SEC_F64 * sStartTotalTime);
}
F64 LLFrameTimer::expiresAt(void) const
{
	F64 expires_at = U64_to_F64(sStartTotalTime) * USEC_TO_SEC_F64;
	expires_at += mExpiry;
	return expires_at;
}
bool LLFrameTimer::checkExpirationAndReset(F32 expiration)
{
	llassert(!mPaused);
  	F64 frame_time = getElapsedSeconds();
	if (frame_time >= mExpiry)
	{
		mStartTime = frame_time;
		mExpiry = mStartTime + expiration;
		return true;
	}
	return false;
}
F32 LLFrameTimer::getElapsedTimeAndResetF32(void)
{
  llassert(mRunning && !mPaused);
  F64 frame_time = getElapsedSeconds();
  F32 elapsed_time = (F32)(frame_time - mStartTime);
  mExpiry = mStartTime = frame_time;
  return elapsed_time;
}
F32 LLFrameTimer::getFrameDeltaTimeF32(void)
{
	llassert(is_main_thread());
	return (F32)(U64_to_F64(sFrameDeltaTime) * USEC_TO_SEC_F64);
}
F32 LLFrameTimer::getCurrentFrameTime(void)
{
	llassert(is_main_thread());
	U64 frame_time = totalTime() - sTotalTime;
	return (F32)(U64_to_F64(frame_time) * USEC_TO_SEC_F64);
}
F32 getCurrentFrameTime(void)
{
	return (F32)(LLFrameTimer::getCurrentFrameTime());
}
