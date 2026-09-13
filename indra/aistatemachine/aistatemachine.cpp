/**
 * @file aistatemachine.cpp
 * @brief Implementation of AIStateMachine
 *
 * Copyright (c) 2010 - 2013, Aleric Inglewood.
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
 *   01/03/2010
 *   Initial version, written by Aleric Inglewood @ SL
 *
 *   28/02/2013
 *   Rewritten from scratch to fully support threading.
 */
#include "linden_common.h"
#include "aistatemachine.h"
#include "aicondition.h"
#include "lltimer.h"
#ifdef EXAMPLE_CODE
class HelloWorld : public AIStateMachine {
  protected:
	typedef AIStateMachine direct_base_type;
	enum hello_world_state_type {
	  HelloWorld_start = direct_base_type::max_state,
	  HelloWorld_done,
	};
  public:
	static state_type const max_state = HelloWorld_done + 1;
  public:
	HelloWorld();
  protected:
	~HelloWorld();
  protected:
	void initialize_impl(void);
	void multiplex_impl(state_type run_state);
	void abort_impl(void);
	void finish_impl(void);
	char const* state_str_impl(state_type run_state) const;
};
char const* HelloWorld::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(HelloWorld_start);
	AI_CASE_RETURN(HelloWorld_done);
  }
#if directly_derived_from_AIStateMachine
  llassert(false);
  return "UNKNOWN STATE";
#else
  llassert(run_state < direct_base_type::max_state);
  return direct_base_type::state_str_impl(run_state);
#endif
}
#endif
#ifdef EXAMPLE_CODE
  HelloWorld* hello_world = new HelloWorld;
  hello_world->init(...);
  hello_world->run(...);
#endif
void AIEngine::add(AIStateMachine* state_machine)
{
  Dout(dc::statemachine(state_machine->mSMDebug), "Adding state machine [" << (void*)state_machine << "] to " << mName);
  engine_state_type_wat engine_state_w(mEngineState);
  engine_state_w->list.push_back(QueueElement(state_machine));
  if (engine_state_w->waiting)
  {
	engine_state_w.signal();
  }
}
#if STATE_MACHINE_PROFILING
void print_statemachine_diagnostics(U64 total_clocks, AIStateMachine::StateTimerBase::TimeData& slowest_timer, AIEngine::queued_type::const_reference slowest_element)
{
	AIStateMachine const& slowest_state_machine = slowest_element.statemachine();
	F64 const tfactor = 1000 / calc_clock_frequency();
	std::ostringstream msg;
	U64 max_delta = slowest_timer.GetDuration();
	if (total_clocks > max_delta)
	{
		msg << "AIStateMachine::mainloop did run for " << (total_clocks * tfactor) << " ms. The slowest ";
	}
	else
	{
		msg << "AIStateMachine::mainloop: A ";
	}
	msg << "state machine " << "(" << slowest_state_machine.getName() << ") " << "ran for " << (max_delta * tfactor) << " ms";
	if (slowest_state_machine.getRuntime() > max_delta)
	{
		msg << " (" << (slowest_state_machine.getRuntime() * tfactor) << " ms in total now)";
	}
	msg << ".\n";
	AIStateMachine::StateTimerBase::DumpTimers(msg);
	LL_WARNS() << msg.str() << LL_ENDL;
}
#endif
void AIEngine::mainloop(void)
{
  queued_type::iterator queued_element, end;
  {
	engine_state_type_wat engine_state_w(mEngineState);
	end = engine_state_w->list.end();
	queued_element = engine_state_w->list.begin();
  }
  U64 total_clocks = 0;
#if STATE_MACHINE_PROFILING
  queued_type::value_type slowest_element(NULL);
  AIStateMachine::StateTimerRoot::TimeData slowest_timer;
#endif
  while (queued_element != end)
  {
	AIStateMachine& state_machine(queued_element->statemachine());
	AIStateMachine::StateTimerBase::TimeData time_data;
	if (!state_machine.sleep(get_clock_count()))
	{
		AIStateMachine::StateTimerRoot timer(state_machine.getName());
		state_machine.multiplex(AIStateMachine::normal_run);
		time_data = timer.GetTimerData();
	}
	if (U64 delta = time_data.GetDuration())
	{
		state_machine.add(delta);
		total_clocks += delta;
#if STATE_MACHINE_PROFILING
		if (delta > slowest_timer.GetDuration())
		{
			slowest_element = *queued_element;
			slowest_timer = time_data;
		}
#endif
	}
	bool active = state_machine.active(this);
	engine_state_type_wat engine_state_w(mEngineState);
	if (!active)
	{
	  Dout(dc::statemachine(state_machine.mSMDebug), "Erasing state machine [" << (void*)&state_machine << "] from " << mName);
	  engine_state_w->list.erase(queued_element++);
	}
	else
	{
	  ++queued_element;
	}
	if (total_clocks >= sMaxCount)
	{
#if STATE_MACHINE_PROFILING
		print_statemachine_diagnostics(total_clocks, slowest_timer, slowest_element);
#endif
	  Dout(dc::statemachine, "Sorting " << engine_state_w->list.size() << " state machines.");
	  engine_state_w->list.sort(QueueElementComp());
	  break;
	}
  }
}
void AIEngine::flush(void)
{
  engine_state_type_wat engine_state_w(mEngineState);
  DoutEntering(dc::statemachine, "AIEngine::flush [" << mName << "]: calling force_killed() on " << engine_state_w->list.size() << " state machines.");
  for (queued_type::iterator iter = engine_state_w->list.begin(); iter != engine_state_w->list.end(); ++iter)
  {
	iter->statemachine().force_killed();
  }
  engine_state_w->list.clear();
}
U64 AIEngine::sMaxCount;
void AIEngine::setMaxCount(F32 StateMachineMaxTime)
{
  llassert(AIThreadID::in_main_thread());
  Dout(dc::statemachine, "(Re)calculating AIStateMachine::sMaxCount");
  sMaxCount = calc_clock_frequency() * StateMachineMaxTime / 1000;
}
#ifdef CWDEBUG
char const* AIStateMachine::event_str(event_type event)
{
  switch(event)
  {
	AI_CASE_RETURN(initial_run);
	AI_CASE_RETURN(schedule_run);
	AI_CASE_RETURN(normal_run);
	AI_CASE_RETURN(insert_abort);
  }
  llassert(false);
  return "UNKNOWN EVENT";
}
#endif
void AIStateMachine::multiplex(event_type event)
{
  llassert(event == initial_run || getNumRefs() > 0);
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::multiplex(" << event_str(event) << ") [" << (void*)this << "]");
  base_state_type state;
  state_type run_state;
  {
	multiplex_state_type_rat state_r(mState);
	llassert(!mMultiplexMutex.isSelfLocked());
	if (!mMultiplexMutex.try_lock())
	{
	  Dout(dc::statemachine(mSMDebug), "Leaving because it is already being run [" << (void*)this << "]");
	  return;
	}
	if (event == schedule_run && !sub_state_type_rat(mSubState)->need_run)
	{
	  Dout(dc::statemachine(mSMDebug), "Leaving because it was already being run [" << (void*)this << "]");
	  return;
	}
	run_state = begin_loop((state = state_r->base_state));
  }
  bool keep_looping;
  bool destruct = false;
  do
  {
	if (event == normal_run)
	{
#ifdef CWDEBUG
	  if (state == bs_multiplex)
		Dout(dc::statemachine(mSMDebug), "Running state bs_multiplex / " << state_str_impl(run_state) << " [" << (void*)this << "]");
	  else
		Dout(dc::statemachine(mSMDebug), "Running state " << state_str(state) << " [" << (void*)this << "]");
#endif
#ifdef SHOW_ASSERT
	  if (state != bs_reset)
	  {
		switch(mDebugLastState)
		{
		  case bs_reset:
			llassert(state == bs_initialize || state == bs_killed);
			break;
		  case bs_initialize:
			llassert(state == bs_multiplex || state == bs_abort);
			break;
		  case bs_multiplex:
			llassert(state == bs_multiplex || state == bs_finish || state == bs_abort);
			break;
		  case bs_abort:
			llassert(state == bs_finish);
			break;
		  case bs_finish:
			llassert(state == bs_callback);
			break;
		  case bs_callback:
			llassert(state == bs_killed || state == bs_reset);
			break;
		  case bs_killed:
			llassert(state == bs_killed);
			break;
		}
	  }
	  if (state == bs_multiplex)
	  {
		mDebugShouldRun |= mDebugSetStatePending;
		llassert(mDebugShouldRun);
	  }
	  mDebugShouldRun = false;
#endif
	  mRunMutex.lock();
	  bool const late_abort = (state == bs_multiplex || state == bs_initialize) && sub_state_type_rat(mSubState)->aborted;
	  if (LL_UNLIKELY(late_abort))
	  {
		state = (state == bs_initialize) ? bs_reset : bs_abort;
#ifdef CWDEBUG
		Dout(dc::statemachine(mSMDebug), "Late abort detected! Running state " << state_str(state) << " instead [" << (void*)this << "]");
#endif
	  }
#ifdef SHOW_ASSERT
	  mDebugLastState = state;
	  if (state == bs_initialize)
	  {
		llassert(!mDebugRefCalled);
		mDebugRefCalled = true;
	  }
#endif
	  switch(state)
	  {
		case bs_reset:
		  break;
		case bs_initialize:
		  ref();
		  initialize_impl();
		  break;
		case bs_multiplex:
		  llassert(!mDebugAborted);
		  multiplex_impl(run_state);
		  break;
		case bs_abort:
		  abort_impl();
		  break;
		case bs_finish:
		  sub_state_type_wat(mSubState)->reset = false;
		  finish_impl();
		  break;
		case bs_callback:
		  callback();
		  break;
		case bs_killed:
		  mRunMutex.unlock();
		  llassert(!multiplex_state_type_rat(mState)->current_engine);
		  return;
	  }
	  mRunMutex.unlock();
	}
	{
	  multiplex_state_type_wat state_w(mState);
	  bool need_new_run = true;
	  if (event == normal_run || event == insert_abort)
	  {
		sub_state_type_rat sub_state_r(mSubState);
		if (event == normal_run)
		{
		  switch(state)
		  {
			case bs_reset:
			  if (sub_state_r->aborted)
			  {
				state_w->base_state = bs_killed;
				need_new_run = false;
			  }
			  else
			  {
				state_w->base_state = bs_initialize;
			  }
			  break;
			case bs_initialize:
			  if (sub_state_r->aborted)
			  {
				state_w->base_state = bs_abort;
			  }
			  else
			  {
				state_w->base_state = bs_multiplex;
				need_new_run = sub_state_r->need_run || !sub_state_r->idle;
			  }
			  break;
			case bs_multiplex:
			  if (sub_state_r->aborted)
			  {
				state_w->base_state = bs_abort;
			  }
			  else if (sub_state_r->finished)
			  {
				state_w->base_state = bs_finish;
			  }
			  else
			  {
				need_new_run = sub_state_r->need_run || !sub_state_r->idle;
				llassert_always(!(need_new_run && !sub_state_r->skip_idle && !mYieldEngine && sub_state_r->run_state == run_state));
			  }
			  break;
			case bs_abort:
			  state_w->base_state = bs_finish;
			  break;
			case bs_finish:
			  state_w->base_state = bs_callback;
			  break;
			case bs_callback:
			  if (sub_state_r->reset)
			  {
				state_w->base_state = bs_reset;
			  }
			  else
			  {
				state_w->base_state = bs_killed;
				destruct = true;
				need_new_run = false;
			  }
			  break;
			default:
			  break;
		  }
		}
		else
		{
		  if (state_w->base_state == bs_multiplex)
		  {
			llassert(sub_state_r->aborted);
			state_w->base_state = bs_abort;
		  }
		}
#ifdef CWDEBUG
		if (state != state_w->base_state)
		  Dout(dc::statemachine(mSMDebug), "Base state changed from " << state_str(state) << " to " << state_str(state_w->base_state) <<
			  "; need_new_run = " << (need_new_run ? "true" : "false") << " [" << (void*)this << "]");
#endif
	  }
	  AIEngine* engine = mYieldEngine ? mYieldEngine : (state_w->current_engine ? state_w->current_engine : mDefaultEngine);
	  AIEngine* current_engine = (event == normal_run) ? state_w->current_engine : NULL;
	  keep_looping = need_new_run && !mYieldEngine && engine == current_engine;
	  mYieldEngine = NULL;
	  if (keep_looping)
	  {
		run_state = begin_loop((state = state_w->base_state));
		event = normal_run;
	  }
	  else
	  {
		if (need_new_run)
		{
		  if (engine != state_w->current_engine)
		  {
			engine->add(this);
			state_w->current_engine = engine;
		  }
#ifdef SHOW_ASSERT
		  mDebugShouldRun = true;
#endif
		}
		else
		{
		  state_w->current_engine = NULL;
		}
#ifdef SHOW_ASSERT
		mThreadId.clear();
		if (destruct)
		{
		  llassert(mDebugRefCalled);
		  mDebugRefCalled  = false;
		}
#endif
		mMultiplexMutex.unlock();
	  }
	}
  }
  while (keep_looping);
  if (destruct)
  {
	unref();
  }
}
#if STATE_MACHINE_PROFILING
std::vector<AIStateMachine::StateTimerBase*> AIStateMachine::StateTimerBase::mTimerStack;
AIStateMachine::StateTimerBase::TimeData AIStateMachine::StateTimerBase::TimeData::sRoot("");
void AIStateMachine::StateTimer::TimeData::DumpTimer(std::ostringstream& msg, std::string prefix)
{
	F64 const tfactor = 1000 / calc_clock_frequency();
	msg << prefix << mName << " " << (mEnd - mStart)*tfactor << "ms" << std::endl;
	prefix.push_back(' ');
	std::vector<TimeData>::iterator it;
	for (it = mChildren.begin(); it != mChildren.end(); ++it)
	{
		it->DumpTimer(msg, prefix);
	}
}
#endif
AIStateMachine::state_type AIStateMachine::begin_loop(base_state_type base_state)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::begin_loop(" << state_str(base_state) << ") [" << (void*)this << "]");
  sub_state_type_wat sub_state_w(mSubState);
  sub_state_w->skip_idle = false;
  sub_state_w->need_run = false;
  if (base_state == bs_multiplex && sub_state_w->advance_state > sub_state_w->run_state)
  {
	Dout(dc::statemachine(mSMDebug), "Copying advance_state to run_state, because it is larger [" << state_str_impl(sub_state_w->advance_state) << " > " << state_str_impl(sub_state_w->run_state) << "]");
	sub_state_w->run_state = sub_state_w->advance_state;
  }
#ifdef SHOW_ASSERT
  else
  {
	mDebugAdvanceStatePending = false;
  }
#endif
  sub_state_w->advance_state = 0;
#ifdef SHOW_ASSERT
  mThreadId.reset();
  mDebugShouldRun |= mDebugContPending;
  mDebugContPending = false;
  mDebugShouldRun |= mDebugAdvanceStatePending;
  mDebugAdvanceStatePending = false;
#endif
  return sub_state_w->run_state;
}
void AIStateMachine::run(AIStateMachine* parent, state_type new_parent_state, bool abort_parent, bool on_abort_signal_parent, AIEngine* default_engine)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::run(" <<
	  (void*)parent << ", " <<
	  (parent ? parent->state_str_impl(new_parent_state) : "NA") <<
	  ", abort_parent = " << (abort_parent ? "true" : "false") <<
	  ", on_abort_signal_parent = " << (on_abort_signal_parent ? "true" : "false") <<
	  ", default_engine = " << (default_engine ? default_engine->name() : "NULL") << ") [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_reset || state_r->base_state == bs_finish || state_r->base_state == bs_callback);
	llassert(!(state_r->base_state == bs_reset && (mParent || mCallback)));
  }
#endif
  mDefaultEngine = default_engine;
  mSleep = 0;
  if (parent)
  {
	mParent = parent;
	if (mCallback)
	{
	  delete mCallback;
	  mCallback = NULL;
	}
	mNewParentState = new_parent_state;
	mAbortParent = abort_parent;
	mOnAbortSignalParent = on_abort_signal_parent;
  }
  llassert(!abort_parent || mParent);
  llassert(!mParent || mParent->running());
  reset();
}
void AIStateMachine::run(callback_type::signal_type::slot_type const& slot, AIEngine* default_engine)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::run(<slot>, default_engine = " << default_engine->name() << ") [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_reset || state_r->base_state == bs_finish || state_r->base_state == bs_callback);
	llassert(!(state_r->base_state == bs_reset && (mParent || mCallback)));
  }
#endif
  mDefaultEngine = default_engine;
  mSleep = 0;
  mParent = NULL;
  if (mCallback)
  {
	delete mCallback;
	mCallback = NULL;
  }
  mCallback = new callback_type(slot);
  reset();
}
void AIStateMachine::callback(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::callback() [" << (void*)this << "]");
  bool aborted = sub_state_type_rat(mSubState)->aborted;
  if (mParent)
  {
	if (mParent->running())
	{
	  if (aborted && mAbortParent)
	  {
		mParent->abort();
		mParent = NULL;
	  }
	  else if (!aborted || mOnAbortSignalParent)
	  {
		mParent->advance_state(mNewParentState);
	  }
	}
  }
  if (mCallback)
  {
	mCallback->callback(!aborted);
	if (multiplex_state_type_rat(mState)->base_state != bs_reset)
	{
	  delete mCallback;
	  mCallback = NULL;
	  mParent = NULL;
	}
  }
  else
  {
	mParent = NULL;
  }
}
void AIStateMachine::force_killed(void)
{
  multiplex_state_type_wat state_w(mState);
  state_w->base_state = bs_killed;
}
void AIStateMachine::kill(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::kill() [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_callback);
	llassert(mThreadId.equals_current_thread());
  }
#endif
  sub_state_type_wat sub_state_w(mSubState);
  sub_state_w->reset = false;
}
void AIStateMachine::reset()
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::reset() [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  mDebugAborted = false;
  mDebugContPending = false;
  mDebugSetStatePending = false;
  mDebugRefCalled = false;
#endif
  mRuntime = 0;
  bool inside_multiplex;
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_reset || state_r->base_state == bs_finish || state_r->base_state == bs_callback);
	inside_multiplex = state_r->base_state != bs_reset;
  }
  {
	sub_state_type_wat sub_state_w(mSubState);
	sub_state_w->aborted = sub_state_w->finished = false;
	sub_state_w->reset = true;
	sub_state_w->idle = false;
	sub_state_w->blocked = NULL;
	sub_state_w->need_run = true;
  }
  if (!inside_multiplex)
  {
	multiplex(initial_run);
  }
}
void AIStateMachine::set_state(state_type new_state)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::set_state(" << state_str_impl(new_state) << ") [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_initialize || state_r->base_state == bs_multiplex);
	llassert(mThreadId.equals_current_thread());
  }
#endif
  sub_state_type_wat sub_state_w(mSubState);
  llassert(!sub_state_w->blocked);
  sub_state_w->run_state = new_state;
  sub_state_w->advance_state = 0;
  sub_state_w->idle = false;
  sub_state_w->skip_idle = false;
#ifdef SHOW_ASSERT
  mDebugSetStatePending = true;
#endif
}
void AIStateMachine::advance_state(state_type new_state)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::advance_state(" << state_str_impl(new_state) << ") [" << (void*)this << "]");
  {
	sub_state_type_wat sub_state_w(mSubState);
	if (sub_state_w->advance_state >= new_state)
	{
	  Dout(dc::statemachine(mSMDebug), "Ignored, because " << state_str_impl(sub_state_w->advance_state) << " >= " << state_str_impl(new_state) << ".");
	  return;
	}
	if (sub_state_w->run_state > new_state)
	{
	  Dout(dc::statemachine(mSMDebug), "Ignored, because " << state_str_impl(sub_state_w->run_state) << " > " << state_str_impl(new_state) << " (current state).");
	  return;
	}
	sub_state_w->advance_state = new_state;
	sub_state_w->idle = false;
	sub_state_w->skip_idle = true;
	if (sub_state_w->blocked)
	{
	  Dout(dc::statemachine(mSMDebug), "Removing statemachine from condition " << (void*)sub_state_w->blocked);
	  sub_state_w->blocked->remove(this);
	  sub_state_w->blocked = NULL;
	}
	sub_state_w->need_run = true;
#ifdef SHOW_ASSERT
	mDebugAdvanceStatePending = true;
	if (sub_state_w->run_state == new_state)
	{
	  mDebugContPending = true;
	}
#endif
  }
  if (!mMultiplexMutex.isSelfLocked())
  {
	multiplex(schedule_run);
  }
}
void AIStateMachine::idle(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::idle() [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_multiplex || state_r->base_state == bs_initialize);
	llassert(mThreadId.equals_current_thread());
  }
  mDebugSetStatePending = false;
#endif
  sub_state_type_wat sub_state_w(mSubState);
  llassert(!sub_state_w->idle);
  if (sub_state_w->skip_idle)
  {
	Dout(dc::statemachine(mSMDebug), "Ignored, because skip_idle is true (advance_state() was called last).");
	return;
  }
  sub_state_w->idle = true;
  mSleep = 0;
}
void AIStateMachine::wait(AIConditionBase& condition)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::wait(" << (void*)&condition << ") [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_multiplex);
	llassert(mThreadId.equals_current_thread());
  }
  mDebugSetStatePending = false;
#endif
  sub_state_type_wat sub_state_w(mSubState);
  llassert(!sub_state_w->idle);
  if (sub_state_w->skip_idle)
  {
	Dout(dc::statemachine(mSMDebug), "Ignored, because skip_idle is true (advance_state() was called last).");
	return;
  }
  condition.wait(this);
  sub_state_w->idle = true;
  sub_state_w->blocked = &condition;
  mSleep = 0;
}
void AIStateMachine::cont(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::cont() [" << (void*)this << "]");
  {
	sub_state_type_wat sub_state_w(mSubState);
	sub_state_w->idle = false;
	if (sub_state_w->blocked)
	{
	  Dout(dc::statemachine(mSMDebug), "Removing statemachine from condition " << (void*)sub_state_w->blocked);
	  sub_state_w->blocked->remove(this);
	  sub_state_w->blocked = NULL;
	}
	sub_state_w->need_run = true;
#ifdef SHOW_ASSERT
	mDebugContPending = true;
#endif
  }
  if (!mMultiplexMutex.isSelfLocked())
  {
	multiplex(schedule_run);
  }
}
bool AIStateMachine::signalled(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::signalled() [" << (void*)this << "]");
  {
	sub_state_type_wat sub_state_w(mSubState);
	if (sub_state_w->blocked)
	{
	  Dout(dc::statemachine(mSMDebug), "Removing statemachine from condition " << (void*)sub_state_w->blocked);
	  sub_state_w->blocked->remove(this);
	  sub_state_w->blocked = NULL;
	}
	else
	{
	  return false;
	}
	sub_state_w->idle = false;
	sub_state_w->need_run = true;
#ifdef SHOW_ASSERT
	mDebugContPending = true;
#endif
  }
  if (!mMultiplexMutex.isSelfLocked())
  {
	multiplex(schedule_run);
  }
  return true;
}
void AIStateMachine::abort(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::abort() [" << (void*)this << "]");
  bool is_waiting = false;
  {
	multiplex_state_type_rat state_r(mState);
	sub_state_type_wat sub_state_w(mSubState);
	sub_state_w->aborted = !sub_state_w->finished;
	if (sub_state_w->blocked)
	{
	  Dout(dc::statemachine(mSMDebug), "Removing statemachine from condition " << (void*)sub_state_w->blocked);
	  sub_state_w->blocked->remove(this);
	  sub_state_w->blocked = NULL;
	}
	sub_state_w->need_run = true;
	is_waiting = state_r->base_state == bs_multiplex && sub_state_w->idle;
  }
  if (is_waiting && !mMultiplexMutex.isSelfLocked())
  {
	multiplex(insert_abort);
  }
  if (!mRunMutex.try_lock())
  {
	LL_WARNS() << "AIStateMachine::abort() blocks because the statemachine is still executing code in another thread." << LL_ENDL;
	mRunMutex.lock();
  }
  mRunMutex.unlock();
#ifdef SHOW_ASSERT
  mDebugAborted = true;
#endif
}
void AIStateMachine::finish(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::finish() [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_multiplex);
	llassert(mThreadId.equals_current_thread());
  }
#endif
  sub_state_type_wat sub_state_w(mSubState);
  llassert(!sub_state_w->idle);
  sub_state_w->finished = true;
}
void AIStateMachine::yield(void)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::yield() [" << (void*)this << "]");
  multiplex_state_type_rat state_r(mState);
  llassert(state_r->base_state == bs_multiplex);
  llassert(mThreadId.equals_current_thread());
  mYieldEngine = state_r->current_engine ? state_r->current_engine : (mDefaultEngine ? mDefaultEngine : &gStateMachineThreadEngine);
}
void AIStateMachine::yield(AIEngine* engine)
{
  llassert(engine);
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::yield(" << engine->name() << ") [" << (void*)this << "]");
#ifdef SHOW_ASSERT
  {
	multiplex_state_type_rat state_r(mState);
	llassert(state_r->base_state == bs_multiplex);
	llassert(mThreadId.equals_current_thread());
  }
#endif
  mYieldEngine = engine;
}
bool AIStateMachine::yield_if_not(AIEngine* engine)
{
  if (engine && multiplex_state_type_rat(mState)->current_engine != engine)
  {
	yield(engine);
	return true;
  }
  return false;
}
void AIStateMachine::yield_frame(unsigned int frames)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::yield_frame(" << frames << ") [" << (void*)this << "]");
  mSleep = -(S64)frames;
  yield(&gMainThreadEngine);
}
void AIStateMachine::yield_ms(unsigned int ms)
{
  DoutEntering(dc::statemachine(mSMDebug), "AIStateMachine::yield_ms(" << ms << ") [" << (void*)this << "]");
  mSleep = get_clock_count() + calc_clock_frequency() * ms / 1000;
  yield(&gMainThreadEngine);
}
char const* AIStateMachine::state_str(base_state_type state)
{
  switch(state)
  {
	AI_CASE_RETURN(bs_reset);
	AI_CASE_RETURN(bs_initialize);
	AI_CASE_RETURN(bs_multiplex);
	AI_CASE_RETURN(bs_abort);
	AI_CASE_RETURN(bs_finish);
	AI_CASE_RETURN(bs_callback);
	AI_CASE_RETURN(bs_killed);
  }
  llassert(false);
  return "UNKNOWN BASE STATE";
}
AIEngine gMainThreadEngine("gMainThreadEngine");
AIEngine gStateMachineThreadEngine("gStateMachineThreadEngine");
void AIEngine::threadloop(void)
{
  queued_type::iterator queued_element, end;
  {
	engine_state_type_wat engine_state_w(mEngineState);
	end = engine_state_w->list.end();
	queued_element = engine_state_w->list.begin();
	if (queued_element == end)
	{
	  engine_state_w->waiting = true;
	  engine_state_w.wait();
	  engine_state_w->waiting = false;
	  return;
	}
  }
  do
  {
	AIStateMachine& state_machine(queued_element->statemachine());
	state_machine.multiplex(AIStateMachine::normal_run);
	bool active = state_machine.active(this);
	engine_state_type_wat engine_state_w(mEngineState);
	if (!active)
	{
	  Dout(dc::statemachine(state_machine.mSMDebug), "Erasing state machine [" << (void*)&state_machine << "] from " << mName);
	  engine_state_w->list.erase(queued_element++);
	}
	else
	{
	  ++queued_element;
	}
  }
  while (queued_element != end);
}
void AIEngine::wake_up(void)
{
  engine_state_type_wat engine_state_w(mEngineState);
  if (engine_state_w->waiting)
  {
	engine_state_w.signal();
  }
}
class AIEngineThread : public LLThread
{
  public:
	static AIEngineThread* sInstance;
	bool volatile mRunning;
  public:
    AIEngineThread(void);
    virtual ~AIEngineThread();
  protected:
	virtual void run(void);
};
AIEngineThread* AIEngineThread::sInstance;
AIEngineThread::AIEngineThread(void) : LLThread("AIEngineThread"), mRunning(true)
{
}
AIEngineThread::~AIEngineThread(void)
{
}
void AIEngineThread::run(void)
{
  while(mRunning)
  {
	gStateMachineThreadEngine.threadloop();
  }
}
void startEngineThread(void)
{
  AIEngineThread::sInstance = new AIEngineThread;
  AIEngineThread::sInstance->start();
}
void stopEngineThread(void)
{
  AIEngineThread::sInstance->mRunning = false;
  gStateMachineThreadEngine.wake_up();
  int count = 401;
  while(--count && !AIEngineThread::sInstance->isStopped())
  {
	ms_sleep(10);
  }
  LL_INFOS() << "State machine thread" << (!AIEngineThread::sInstance->isStopped() ? " not" : "") << " stopped after " << ((400 - count) * 10) << "ms." << LL_ENDL;
}
