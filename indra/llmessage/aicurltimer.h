/**
 * @file aicurltimer.h
 * @brief Implementation of AICurlTimer.
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
 *   29/06/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AICURLTIMER_H
#define AICURLTIMER_H
#include "llerror.h"
#include "stdtypes.h"
#include <boost/signals2.hpp>
#include <set>
class AICurlTimer
{
  protected:
	typedef boost::signals2::signal<void (void)> signal_type;
	typedef long deltams_type;
  private:
	struct Signal {
		signal_type mSignal;
	};
	class AIRunningCurlTimer {
	  private:
		U64 mExpire;
		AICurlTimer* mTimer;
		mutable Signal* mCallback;
	  public:
		AIRunningCurlTimer(deltams_type expiration, AICurlTimer* timer) : mExpire(AICurlTimer::sTime_1ms + expiration), mTimer(timer), mCallback(NULL) { }
		~AIRunningCurlTimer() { delete mCallback; }
		void init(signal_type::slot_type const& slot) const
			{
			  llassert(!mCallback);
			  mCallback = new Signal;
			  mCallback->mSignal.connect(slot);
			}
		friend bool operator<(AIRunningCurlTimer const& ft1, AIRunningCurlTimer const& ft2) { return ft1.mExpire < ft2.mExpire; }
		void do_callback(void) const { mCallback->mSignal(); }
		U64 expiration(void) const { return mExpire; }
		AICurlTimer* getTimer(void) const { return mTimer; }
#if LL_DEBUG
		AIRunningCurlTimer(AIRunningCurlTimer const& running_curl_timer) :
			mExpire(running_curl_timer.mExpire), mTimer(running_curl_timer.mTimer), mCallback(running_curl_timer.mCallback)
			{ llassert(!mCallback); }
#endif
	};
	typedef std::multiset<AIRunningCurlTimer> timer_list_type;
	static timer_list_type sTimerList;
	static U64 sNextExpiration;
  public:
	static F64 const sClockWidth_1ms;
	static U64 sTime_1ms;
	static deltams_type nextExpiration(void) { return static_cast<deltams_type>(sNextExpiration - sTime_1ms); }
	static bool expiresBefore(deltams_type timeout_ms) { return sNextExpiration < sTime_1ms + timeout_ms; }
  private:
	class Handle {
	  public:
		timer_list_type::iterator mRunningTimer;
		Handle(void) : mRunningTimer(sTimerList.end()) { }
		void init(timer_list_type::iterator const& running_timer, signal_type::slot_type const& slot)
			{
			  mRunningTimer = running_timer;
			  mRunningTimer->init(slot);
			}
	  private:
	  	Handle& operator=(Handle const&) { return *this; }
	};
	Handle mHandle;
  public:
	AICurlTimer(void) { }
	AICurlTimer(deltams_type expiration, signal_type::slot_type const& slot) { create(expiration, slot); }
	~AICurlTimer() { cancel(); }
	void create(deltams_type expiration, signal_type::slot_type const& slot);
	void cancel(void);
	bool isRunning(void) const { return mHandle.mRunningTimer != sTimerList.end(); }
  public:
	static void handleExpiration(void);
};
#endif
