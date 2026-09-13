/**
 * @file aiaverage.cpp
 * @brief Implementation of AIAverage
 *
 * Copyright (c) 2013, Aleric Inglewood.
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
 *   11/04/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */
#include "sys.h"
#include "aiaverage.h"
#include "llerror.h"
void AIAverage::cleanup(U64 clock_tick)
{
  if (LL_LIKELY(clock_tick > mCurrentClock))
  {
	++mCurrentBucket;
	mCurrentBucket %= mNrOfBuckets;
	mData[mCurrentBucket].time = clock_tick;
	U64 old_time = clock_tick - mNrOfBuckets;
	if (LL_UNLIKELY(mTail == mCurrentBucket) ||
		mData[mTail].time <= old_time)
	{
	  do
	  {
		mSum -= mData[mTail].sum;
		mN -= mData[mTail].n;
		mData[mTail].sum = 0;
		mData[mTail].n = 0;
		++mTail;
		if (LL_UNLIKELY(mTail == mNrOfBuckets))
		{
		  mTail = 0;
		}
	  }
	  while (mData[mTail].time <= old_time);
	}
	llassert(mData[mCurrentBucket].sum == 0 &&
	         mData[mCurrentBucket].n == 0);
  }
  mCurrentClock = clock_tick;
  return;
}
