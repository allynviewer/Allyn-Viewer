/**
 * @file aiframetimer.h
 * @brief Implementation of AIFrameTimer.
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
 *   05/08/2011
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AIFRAMETIMER_H
#define AIFRAMETIMER_H
#include "llframetimer.h"
#include "llthread.h"
#include <boost/signals2.hpp>
#include <set>
class LL_COMMON_API AIFrameTimer
{
  protected:
	typedef boost::signals2::signal<void (void)> signal_type;
  private:
	struct Signal {
		signal_type mSignal;
	};
	class AIRunningFrameTimer {
	  private:
		F64 mExpire;
		AIFrameTimer* mTimer;
		mutable Signal* mCallback;
	  public:
		AIRunningFrameTimer(F64 expiration, AIFrameTimer* timer) : mExpire(LLFrameTimer::getElapsedSeconds() + expiration), mTimer(timer), mCallback(NULL) { }
		~AIRunningFrameTimer() { delete mCallback; }
		void init(signal_type::slot_type const& slot) const
			{
			  llassert(!mCallback);
			  mCallback = new Signal;
			  mCallback->mSignal.connect(slot);
			}
		friend bool operator<(AIRunningFrameTimer const& ft1, AIRunningFrameTimer const& ft2) { return ft1.mExpire < ft2.mExpire; }
		void do_callback(void) const { mCallback->mSignal(); }
		F64 expiration(void) const { return mExpire; }
		AIFrameTimer* getTimer(void) const { return mTimer; }
#if LL_DEBUG
		AIRunningFrameTimer(AIRunningFrameTimer const& running_frame_timer) :
			mExpire(running_frame_timer.mExpire), mTimer(running_frame_timer.mTimer), mCallback(running_frame_timer.mCallback)
			{ llassert(!mCallback); }
#endif
	};
	typedef std::multiset<AIRunningFrameTimer> timer_list_type;
	static LLGlobalMutex sMutex;
	static timer_list_type sTimerList;
	static F64 sNextExpiration;
	friend class LLFrameTimer;
	class Handle {
	  public:
		timer_list_type::iterator mRunningTimer;
		LLMutex mMutex;
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
	AIFrameTimer(void) { }
	AIFrameTimer(F64 expiration, signal_type::slot_type const& slot) { create(expiration, slot); }
	~AIFrameTimer() { cancel(); }
	void create(F64 expiration, signal_type::slot_type const& slot);
	void cancel(void);
	bool isRunning(void) const { bool running; sMutex.lock(); running = mHandle.mRunningTimer != sTimerList.end(); sMutex.unlock(); return running; }
  protected:
	static void handleExpiration(F64 current_frame_time);
};
#endif
