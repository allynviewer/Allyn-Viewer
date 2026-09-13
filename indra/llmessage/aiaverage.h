/**
 * @file aiaverage.h
 * @brief Definition of class AIAverage
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
#ifndef AIAVERAGE_H
#define AIAVERAGE_H
#include "llpreprocessor.h"
#include "stdtypes.h"
#include "llthread.h"
#include <cstddef>
#include <vector>
class AIAverage {
  private:
	struct Data {
	  U32 sum;
	  U32 n;
	  U64 time;
	};
	U64 mCurrentClock;
	int mTail;
	int mCurrentBucket;
	size_t mSum;
	U32 mN;
	int const mNrOfBuckets;
	std::vector<Data> mData;
	mutable LLMutex mLock;
  public:
	AIAverage(int number_of_buckets) : mCurrentClock(~(U64)0), mTail(0), mCurrentBucket(0), mSum(0), mN(0), mNrOfBuckets(number_of_buckets), mData(number_of_buckets)
	{
	  std::memset(&*mData.begin(), 0, number_of_buckets * sizeof(Data));
	}
	size_t addData(U32 n, U64 clock_tick)
	{
	  DoutEntering(dc::curl, "AIAverage::addData(" << n << ", " << clock_tick << ")");
	  mLock.lock();
	  if (LL_UNLIKELY(clock_tick != mCurrentClock))
	  {
		cleanup(clock_tick);
	  }
	  mSum += n;
	  mN += 1;
	  mData[mCurrentBucket].sum += n;
	  mData[mCurrentBucket].n += 1;
	  size_t sum = mSum;
	  mLock.unlock();
	  Dout(dc::curl, "Current sum: " << sum << ", average: " << (sum / mN));
	  return sum;
	}
	size_t truncateData(U64 clock_tick)
	{
	  mLock.lock();
	  if (clock_tick != mCurrentClock)
	  {
		cleanup(clock_tick);
	  }
	  size_t sum = mSum;
	  mLock.unlock();
	  return sum;
	}
	double getAverage(double avg_no_data) const
	{
	  mLock.lock();
	  double avg = mSum;
	  llassert(mN != 0 || mSum == 0);
	  if (LL_UNLIKELY(mN == 0))
		avg = avg_no_data;
	  else
		avg /= mN;
	  mLock.unlock();
	  return avg;
	}
  private:
	void cleanup(U64 clock_tick);
};
#endif
