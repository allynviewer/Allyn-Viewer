/** 
 * @file llframetimer.h
 * @brief A lightweight timer that measures seconds and is only
 * updated once per frame.
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
#ifndef LL_LLFRAMETIMER_H
#define LL_LLFRAMETIMER_H
#include "lltimer.h"
#include "timing.h"
#include "llthread.h"
class LL_COMMON_API LLFrameTimer
{
public:
	LLFrameTimer(void) : mExpiry(0), mRunning(true), mPaused(false) { if (!sFirstFrameTimerCreated) global_initialization(); setAge(0.0); }
	void copy(LLFrameTimer const& timer) { mStartTime = timer.mStartTime; mExpiry = timer.mExpiry; mRunning = timer.mRunning; mPaused = timer.mPaused; }
	static F64SecondsImplicit getElapsedSeconds(void)
	{
		sGlobalMutex.lock();
		F64 res = sFrameTime;
		sGlobalMutex.unlock();
		return res;
	}
	static U64 getTotalTime(void)
	{
		llassert(is_main_thread());
		U64 res = sTotalTime;
		llassert(res);
		return res;
	}
	static F64 getTotalSeconds(void)
	{
		llassert(is_main_thread());
		F64 res = sTotalSeconds;
		return res;
	}
	static U64 getFrameCount(void)
	{
		llassert(is_main_thread());
		U64 res = sFrameCount;
		return res;
	}
	static void updateFrameTime(void);
	static void updateFrameTimeAndCount(void);
	static F32 getFrameDeltaTimeF32(void);
	static F32 getCurrentFrameTime(void);
	void reset(F32 expiration = 0.f)
	{
		llassert(!mPaused);
		mStartTime = getElapsedSeconds();
		mExpiry = mStartTime + expiration;
	}
	void start(F32 expiration = 0.f)
	{
		reset(expiration);
		mRunning = true;
	}
	void stop(void)
	{
		llassert(!mPaused);
		mRunning = false;
	}
	void resetWithExpiry(F32 expiration)			{ reset(); setTimerExpirySec(expiration); }
	void pause();
	void unpause();
	void setTimerExpirySec(F32 expiration)			{ llassert(!mPaused); mExpiry = mStartTime + expiration; }
	void setExpiryAt(F64 seconds_since_epoch);
	bool checkExpirationAndReset(F32 expiration);
	F32 getElapsedTimeAndResetF32(void);
	void setAge(const F64 age)						{ llassert(!mPaused); mStartTime = getElapsedSeconds() - age; }
	bool hasExpired() const							{ return getElapsedSeconds() >= mExpiry; }
	F32  getElapsedTimeF32() const					{ llassert(mRunning); return mPaused ? (F32)mStartTime : (F32)(getElapsedSeconds() - mStartTime); }
	bool getStarted() const							{ return mRunning; }
	F64 getStartTime() const						{ llassert(!mPaused); return mStartTime; }
	F64 expiresAt() const;
public:
	static void global_initialization(void);
protected:
	static U64 const sStartTotalTime;
	static LLGlobalMutex sGlobalMutex;
	static F64 sFrameTime;
	static U64 sFrameDeltaTime;
	static U64 sTotalTime;
	static U64 sPrevTotalTime;
	static F64 sTotalSeconds;
	static U64 sFrameCount;
	static bool sFirstFrameTimerCreated;
	F64 mStartTime;
	F64 mExpiry;
	bool mRunning;
	bool mPaused;
};
extern F32 getCurrentFrameTime();
#endif
