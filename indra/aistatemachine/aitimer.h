/**
 * @file aitimer.h
 * @brief Generate a timer event
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
 *   07/02/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AITIMER_H
#define AITIMER_H
#include "aistatemachine.h"
#include "aiframetimer.h"
class AITimer : public AIStateMachine {
  protected:
	typedef AIStateMachine direct_base_type;
	enum timer_state_type {
	  AITimer_start = direct_base_type::max_state,
	  AITimer_expired
	};
  public:
	static state_type const max_state = AITimer_expired + 1;
  private:
	AIFrameTimer mFrameTimer;
	F64 mInterval;
  public:
	AITimer(CWD_ONLY(bool debug = false)) :
#ifdef CWDEBUG
		AIStateMachine(debug),
#endif
		mInterval(0) { DoutEntering(dc::statemachine(mSMDebug), "AITimer(void) [" << (void*)this << "]"); }
	void setInterval(F64 interval) { mInterval = interval; }
	F64 getInterval(void) const { return mInterval; }
	const char* getName() const { return "AITimer"; }
  protected:
	~AITimer() { DoutEntering(dc::statemachine(mSMDebug), "~AITimer() [" << (void*)this << "]"); mFrameTimer.cancel(); }
	void initialize_impl(void);
	void multiplex_impl(state_type run_state);
	void abort_impl(void);
	char const* state_str_impl(state_type run_state) const;
  private:
	void expired(void);
};
#endif
