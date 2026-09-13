/** 
 * @file llthrottle.h
 * @brief LLThrottle class used for network bandwidth control
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LL_LLTHROTTLE_H
#define LL_LLTHROTTLE_H
#include "lltimer.h"
const S32 MAX_THROTTLE_SIZE = 32;
class LLDataPacker;
class LLThrottle
{
public:
	LLThrottle(const F32 throttle = 1.f);
	~LLThrottle() { }
	void setRate(const F32 rate);
	BOOL checkOverflow(const F32 amount);
	BOOL throttleOverflow(const F32 amount);
	F32 getAvailable();
	F32 getRate() const				{ return mRate; }
private:
	F32 mLookaheadSecs;
	F32	mRate;
	F32	mAvailable;
	F64Seconds	mLastSendTime;
};
typedef enum e_throttle_categories
{
	TC_RESEND,
	TC_LAND,
	TC_WIND,
	TC_CLOUD,
	TC_TASK,
	TC_TEXTURE,
	TC_ASSET,
	TC_EOF
} EThrottleCats;
class LLThrottleGroup
{
public:
	LLThrottleGroup();
	~LLThrottleGroup() { }
	void	resetDynamicAdjust();
	BOOL	checkOverflow(S32 throttle_cat, F32 bits);
	BOOL	throttleOverflow(S32 throttle_cat, F32 bits);
	BOOL	dynamicAdjust();
	BOOL	setNominalBPS(F32* throttle_vec);
	S32		getAvailable(S32 throttle_cat);
	void packThrottle(LLDataPacker &dp) const;
	void unpackThrottle(LLDataPacker &dp);
public:
	F32		mThrottleTotal[TC_EOF];
protected:
	F32		mNominalBPS[TC_EOF];
	F32		mCurrentBPS[TC_EOF];
	F32		mBitsAvailable[TC_EOF];
	F32		mBitsSentThisPeriod[TC_EOF];
	F32		mBitsSentHistory[TC_EOF];
	F64Seconds	mLastSendTime[TC_EOF];
	F64Seconds	mDynamicAdjustTime;
};
#endif
