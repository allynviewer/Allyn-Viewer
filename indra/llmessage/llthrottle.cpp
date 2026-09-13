/** 
 * @file llthrottle.cpp
 * @brief LLThrottle class used for network bandwidth control.
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
#include "linden_common.h"
#include "llthrottle.h"
#include "llmath.h"
#include "lldatapacker.h"
#include "message.h"
LLThrottle::LLThrottle(const F32 rate)
{
	mRate = rate;
	mAvailable = 0.f;
	mLookaheadSecs = 0.25f;
	mLastSendTime = LLMessageSystem::getMessageTimeSeconds(TRUE);
}
void LLThrottle::setRate(const F32 rate)
{
	mAvailable = getAvailable();
	mLastSendTime = LLMessageSystem::getMessageTimeSeconds();
	mRate = rate;
}
F32 LLThrottle::getAvailable()
{
	F32Seconds elapsed_time = LLMessageSystem::getMessageTimeSeconds() - mLastSendTime;
	return mAvailable + (mRate * elapsed_time.value());
}
BOOL LLThrottle::checkOverflow(const F32 amount)
{
	BOOL retval = TRUE;
	F32 lookahead_amount = mRate * mLookaheadSecs;
	F32Seconds elapsed_time =  LLMessageSystem::getMessageTimeSeconds() - mLastSendTime;
	F32 amount_available = mAvailable + (mRate * elapsed_time.value());
	if ((amount_available >= lookahead_amount) || (amount_available > amount))
	{
		retval = FALSE;
	}
	return retval;
}
BOOL LLThrottle::throttleOverflow(const F32 amount)
{
	F32Seconds elapsed_time;
	F32 lookahead_amount;
	BOOL retval = TRUE;
	lookahead_amount = mRate * mLookaheadSecs;
	F64Seconds mt_sec = LLMessageSystem::getMessageTimeSeconds();
	elapsed_time = mt_sec - mLastSendTime;
	mLastSendTime = mt_sec;
	mAvailable += mRate * elapsed_time.value();
	if (mAvailable >= lookahead_amount)
	{
		mAvailable = lookahead_amount;
		retval = FALSE;
	}
	else if (mAvailable > amount)
	{
		retval = FALSE;
	}
	mAvailable -= amount;
	return retval;
}
const F32 THROTTLE_LOOKAHEAD_TIME = 1.f;
F32 gThrottleMaximumBPS[TC_EOF] =
{
	150000.f,
	170000.f,
	34000.f,
	34000.f,
	446000.f,
	446000.f,
	220000.f,
};
F32 gThrottleDefaultBPS[TC_EOF] =
{
	100000.f,
	4000.f,
	4000.f,
	4000.f,
	4000.f,
	4000.f,
	100000.f,
};
F32 gThrottleMinimumBPS[TC_EOF] =
{
	10000.f,
	10000.f,
	 4000.f,
	 4000.f,
	20000.f,
	10000.f,
	10000.f,
};
const char* THROTTLE_NAMES[TC_EOF] =
{
	"Resend ",
	"Land   ",
	"Wind   ",
	"Cloud  ",
	"Task   ",
	"Texture",
	"Asset  "
};
LLThrottleGroup::LLThrottleGroup()
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		mThrottleTotal[i]	= gThrottleDefaultBPS[i];
		mNominalBPS[i]		= gThrottleDefaultBPS[i];
	}
	resetDynamicAdjust();
}
void LLThrottleGroup::packThrottle(LLDataPacker &dp) const
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		dp.packF32(mThrottleTotal[i], "Throttle");
	}
}
void LLThrottleGroup::unpackThrottle(LLDataPacker &dp)
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		F32 temp_throttle;
		dp.unpackF32(temp_throttle, "Throttle");
		temp_throttle = llclamp(temp_throttle, 0.f, 2250000.f);
		mThrottleTotal[i] = temp_throttle;
		if(mThrottleTotal[i] > gThrottleMaximumBPS[i])
		{
			mThrottleTotal[i] = gThrottleMaximumBPS[i];
		}
	}
}
void LLThrottleGroup::resetDynamicAdjust()
{
	F64Seconds mt_sec = LLMessageSystem::getMessageTimeSeconds();
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		mCurrentBPS[i]		= mNominalBPS[i];
		mBitsAvailable[i]	= mNominalBPS[i] * THROTTLE_LOOKAHEAD_TIME;
		mLastSendTime[i] = mt_sec;
		mBitsSentThisPeriod[i] = 0;
		mBitsSentHistory[i] = 0;
	}
	mDynamicAdjustTime = mt_sec;
}
BOOL LLThrottleGroup::setNominalBPS(F32* throttle_vec)
{
	BOOL changed = FALSE;
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		if (mNominalBPS[i] != throttle_vec[i])
		{
			changed = TRUE;
			mNominalBPS[i] = throttle_vec[i];
		}
	}
	if (changed)
	{
		resetDynamicAdjust();
	}
	return changed;
}
S32		LLThrottleGroup::getAvailable(S32 throttle_cat)
{
	S32 retval = 0;
	F32 category_bps = mCurrentBPS[throttle_cat];
	F32 lookahead_bits = category_bps * THROTTLE_LOOKAHEAD_TIME;
	F32Seconds elapsed_time = LLMessageSystem::getMessageTimeSeconds() - mLastSendTime[throttle_cat];
	F32 bits_available = mBitsAvailable[throttle_cat] + (category_bps * elapsed_time.value());
	if (bits_available >= lookahead_bits)
	{
		retval = (S32) gThrottleMaximumBPS[throttle_cat];
	}
	else
	{
		retval = (S32) bits_available;
	}
	return retval;
}
BOOL LLThrottleGroup::checkOverflow(S32 throttle_cat, F32 bits)
{
	BOOL retval = TRUE;
	F32 category_bps = mCurrentBPS[throttle_cat];
	F32 lookahead_bits = category_bps * THROTTLE_LOOKAHEAD_TIME;
	F32Seconds elapsed_time = LLMessageSystem::getMessageTimeSeconds() - mLastSendTime[throttle_cat];
	F32 bits_available = mBitsAvailable[throttle_cat] + (category_bps * elapsed_time.value());
	if (bits_available >= lookahead_bits)
	{
		mBitsAvailable[throttle_cat] = lookahead_bits;
		retval = FALSE;
	}
	else if ( bits_available > bits )
	{
		retval = FALSE;
	}
	return retval;
}
BOOL LLThrottleGroup::throttleOverflow(S32 throttle_cat, F32 bits)
{
	F32Seconds elapsed_time;
	F32 category_bps;
	F32 lookahead_bits;
	BOOL retval = TRUE;
	category_bps = mCurrentBPS[throttle_cat];
	lookahead_bits = category_bps * THROTTLE_LOOKAHEAD_TIME;
	F64Seconds mt_sec = LLMessageSystem::getMessageTimeSeconds();
	elapsed_time = mt_sec - mLastSendTime[throttle_cat];
	mLastSendTime[throttle_cat] = mt_sec;
	mBitsAvailable[throttle_cat] += category_bps * elapsed_time.value();
	if (mBitsAvailable[throttle_cat] >= lookahead_bits)
	{
		mBitsAvailable[throttle_cat] = lookahead_bits;
		retval = FALSE;
	}
	else if ( mBitsAvailable[throttle_cat] > bits )
	{
		retval = FALSE;
	}
	mBitsAvailable[throttle_cat] -= bits;
	mBitsSentThisPeriod[throttle_cat] += bits;
	return retval;
}
BOOL LLThrottleGroup::dynamicAdjust()
{
	const F32Seconds DYNAMIC_ADJUST_TIME(1.0f);
	const F32 CURRENT_PERIOD_WEIGHT = .25f;
	const F32 BUSY_PERCENT = 0.75f;
	const F32 IDLE_PERCENT = 0.70f;
	const F32 TRANSFER_PERCENT = 0.90f;
	const F32 RECOVER_PERCENT = 0.25f;
	S32 i;
	F64Seconds mt_sec = LLMessageSystem::getMessageTimeSeconds();
	if ((mt_sec - mDynamicAdjustTime) < DYNAMIC_ADJUST_TIME)
	{
		return FALSE;
	}
	mDynamicAdjustTime = mt_sec;
	S32 total = 0;
	for (i = 0; i < TC_EOF; i++)
	{
		if (mBitsSentHistory[i] == 0)
		{
			mBitsSentHistory[i] = mBitsSentThisPeriod[i];
		}
		else
		{
			mBitsSentHistory[i] = (1.f - CURRENT_PERIOD_WEIGHT) * mBitsSentHistory[i]
				+ CURRENT_PERIOD_WEIGHT * mBitsSentThisPeriod[i];
		}
		mBitsSentThisPeriod[i] = 0;
		total += ll_pos_round(mBitsSentHistory[i]);
	}
	BOOL channels_busy = FALSE;
	F32  busy_nominal_sum = 0;
	BOOL channel_busy[TC_EOF];
	BOOL channel_idle[TC_EOF];
	BOOL channel_over_nominal[TC_EOF];
	for (i = 0; i < TC_EOF; i++)
	{
		if (mBitsSentHistory[i] >= BUSY_PERCENT * DYNAMIC_ADJUST_TIME.value() * mCurrentBPS[i])
		{
			channels_busy = TRUE;
			busy_nominal_sum += mNominalBPS[i];
			channel_busy[i] = TRUE;
		}
		else
		{
			channel_busy[i] = FALSE;
		}
		if ((mBitsSentHistory[i] < IDLE_PERCENT * DYNAMIC_ADJUST_TIME.value() * mCurrentBPS[i]) &&
			(mBitsAvailable[i] > 0))
		{
			channel_idle[i] = TRUE;
		}
		else
		{
			channel_idle[i] = FALSE;
		}
		if (mCurrentBPS[i] > mNominalBPS[i])
		{
			channel_over_nominal[i] = TRUE;
		}
		else
		{
			channel_over_nominal[i] = FALSE;
		}
	}
	if (channels_busy)
	{
		F32 used_bps;
		F32 avail_bps;
		F32 transfer_bps;
		F32 pool_bps = 0;
		for (i = 0; i < TC_EOF; i++)
		{
			if (channel_idle[i] || channel_over_nominal[i] )
			{
				used_bps = mBitsSentHistory[i] / DYNAMIC_ADJUST_TIME.value();
				if (used_bps < gThrottleMinimumBPS[i])
				{
					used_bps = gThrottleMinimumBPS[i];
				}
				if (channel_over_nominal[i])
				{
					F32 unused_current = mCurrentBPS[i] - used_bps;
					avail_bps = llmax(mCurrentBPS[i] - mNominalBPS[i], unused_current);
				}
				else
				{
					avail_bps = mCurrentBPS[i] - used_bps;
				}
				if (avail_bps < 0)
				{
					continue;
				}
				transfer_bps = avail_bps * TRANSFER_PERCENT;
				mCurrentBPS[i] -= transfer_bps;
				pool_bps += transfer_bps;
			}
		}
		F32 unused_bps = 0.f;
		for (i = 0; i < TC_EOF; i++)
		{
			if (channel_busy[i])
			{
				F32 add_amount = pool_bps * (mNominalBPS[i] / busy_nominal_sum);
				mCurrentBPS[i] += add_amount;
				const F32 MAX_BPS = 4 * mNominalBPS[i];
				if (mCurrentBPS[i] > MAX_BPS)
				{
					F32 overage = mCurrentBPS[i] - MAX_BPS;
					mCurrentBPS[i] -= overage;
					unused_bps += overage;
				}
				if (mCurrentBPS[i] < gThrottleMinimumBPS[i])
				{
					mCurrentBPS[i] = gThrottleMinimumBPS[i];
				}
			}
		}
		if (unused_bps > 0.f)
		{
			mCurrentBPS[TC_TASK] += unused_bps;
		}
	}
	else
	{
		F32 starved_nominal_sum = 0;
		F32 avail_bps = 0;
		F32 transfer_bps = 0;
		F32 pool_bps = 0;
		for (i = 0; i < TC_EOF; i++)
		{
			if (mCurrentBPS[i] > mNominalBPS[i])
			{
				avail_bps = (mCurrentBPS[i] - mNominalBPS[i]);
				transfer_bps = avail_bps * RECOVER_PERCENT;
				mCurrentBPS[i] -= transfer_bps;
				pool_bps += transfer_bps;
			}
		}
		for (i = 0; i < TC_EOF; i++)
		{
			if (mCurrentBPS[i] < mNominalBPS[i])
			{
				starved_nominal_sum += mNominalBPS[i];
			}
		}
		for (i = 0; i < TC_EOF; i++)
		{
			if (mCurrentBPS[i] < mNominalBPS[i])
			{
				mCurrentBPS[i] += pool_bps * (mNominalBPS[i] / starved_nominal_sum);
			}
		}
	}
	return TRUE;
}
