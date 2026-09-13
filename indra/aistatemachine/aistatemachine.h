/**
 * @file aistatemachine.h
 * @brief State machine base class
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
#ifndef AISTATEMACHINE_H
#define AISTATEMACHINE_H
#include "aithreadsafe.h"
#include <llpointer.h>
#include "lltimer.h"
#include <list>
#include <boost/signals2.hpp>
class AIConditionBase;
class AIStateMachine;
class AIEngine
{
  private:
	struct QueueElementComp;
	class QueueElement {
	  private:
		LLPointer<AIStateMachine> mStateMachine;
	  public:
		QueueElement(AIStateMachine* statemachine) : mStateMachine(statemachine) { }
		friend bool operator==(QueueElement const& e1, QueueElement const& e2) { return e1.mStateMachine == e2.mStateMachine; }
		friend bool operator!=(QueueElement const& e1, QueueElement const& e2) { return e1.mStateMachine != e2.mStateMachine; }
		friend struct QueueElementComp;
		AIStateMachine const& statemachine(void) const { return *mStateMachine; }
		AIStateMachine& statemachine(void) { return *mStateMachine; }
	};
	struct QueueElementComp {
	  inline bool operator()(QueueElement const& e1, QueueElement const& e2) const;
	};
  public:
	typedef std::list<QueueElement> queued_type;
	struct engine_state_type {
	  queued_type list;
	  bool waiting;
	  engine_state_type(void) : waiting(false) { }
	};
  private:
	AIThreadSafeSimpleDC<engine_state_type, LLCondition>	mEngineState;
	typedef AIAccessConst<engine_state_type, LLCondition>	engine_state_type_crat;
	typedef AIAccess<engine_state_type, LLCondition>		engine_state_type_rat;
	typedef AIAccess<engine_state_type, LLCondition>		engine_state_type_wat;
	char const* mName;
	static U64 sMaxCount;
  public:
	AIEngine(char const* name) : mName(name) { }
	void add(AIStateMachine* state_machine);
	void mainloop(void);
	void threadloop(void);
	void wake_up(void);
	void flush(void);
	char const* name(void) const { return mName; }
	static void setMaxCount(F32 StateMachineMaxTime);
};
extern AIEngine gMainThreadEngine;
extern AIEngine gStateMachineThreadEngine;
#ifndef STATE_MACHINE_PROFILING
#ifndef LL_RELEASE_FOR_DOWNLOAD
#define STATE_MACHINE_PROFILING 1
#endif
#endif
class AIStateMachine : public LLThreadSafeRefCount
{
  public:
	typedef U32 state_type;
	class StateTimerBase
	{
	public:
		class TimeData
		{
			friend class StateTimerBase;
		public:
			TimeData() : mStart(-1), mEnd(-1) {}
			U64 GetDuration() {	return mEnd - mStart; }
		private:
			U64 mStart, mEnd;
#if !STATE_MACHINE_PROFILING
			TimeData(const std::string& name) : mStart(get_clock_count()), mEnd(get_clock_count()) {}
#else
			TimeData(const std::string& name) : mName(name), mStart(get_clock_count()), mEnd(get_clock_count()) {}
			void DumpTimer(std::ostringstream& msg, std::string prefix);
			std::vector<TimeData> mChildren;
			std::string mName;
			static TimeData sRoot;
#endif
		};
#if !STATE_MACHINE_PROFILING
		StateTimerBase(const std::string& name) : mData(name) {}
		~StateTimerBase() {}
	protected:
		TimeData mData;
	public:
		const TimeData GetTimerData()
		{
			mData.mEnd = get_clock_count();
			return mData;
		}
#else
	protected:
		StateTimerBase() : mData(NULL) {}
		~StateTimerBase()
		{
			if (!mData)
				return;
			mData->mEnd = get_clock_count();
			mTimerStack.pop_back();
		}
		bool AddAsRoot(const std::string& name)
		{
			if (!is_main_thread())
				return true;
			if (!mTimerStack.empty())
				return false;
			TimeData::sRoot = TimeData(name);
			mData = &TimeData::sRoot;
			mData->mChildren.clear();
			mTimerStack.push_back(this);
			return true;
		}
		bool AddAsChild(const std::string& name)
		{
			if (!is_main_thread())
				return true;
			if (mTimerStack.empty())
				return false;
			mTimerStack.back()->mData->mChildren.push_back(TimeData(name));
			mData = &mTimerStack.back()->mData->mChildren.back();
			mTimerStack.push_back(this);
			return true;
		}
		TimeData* mData;
		static std::vector<StateTimerBase*> mTimerStack;
	public:
		static void DumpTimers(std::ostringstream& msg)
		{
			TimeData::sRoot.DumpTimer(msg, "");
		}
		const TimeData GetTimerData() const
		{
			if (mData)
			{
				TimeData ret = *mData;
				ret.mEnd = get_clock_count();
				return ret;
			}
			return TimeData();
		}
#endif
	};
public:
#if !STATE_MACHINE_PROFILING
	typedef StateTimerBase StateTimerRoot;
	typedef StateTimerBase StateTimer;
#else
	class StateTimerRoot : public StateTimerBase
	{
	public:
		StateTimerRoot(const std::string& name)
		{
			if(!AddAsRoot(name))
				AddAsChild(name);
		}
	};
	class StateTimer : public StateTimerBase
	{
	public:
		StateTimer(const std::string& name)
		{
			AddAsChild(name);
		}
	};
#endif
  protected:
	enum event_type {
	  initial_run,
	  schedule_run,
	  normal_run,
	  insert_abort
	};
	enum base_state_type {
	  bs_reset,
	  bs_initialize,
	  bs_multiplex,
	  bs_abort,
	  bs_finish,
	  bs_callback,
	  bs_killed
	};
  public:
	static state_type const max_state = bs_killed + 1;
  protected:
	struct multiplex_state_type {
	  base_state_type	base_state;
	  AIEngine*			current_engine;
	  multiplex_state_type(void) : base_state(bs_reset), current_engine(NULL) { }
	};
	struct sub_state_type {
	  state_type		run_state;
	  state_type		advance_state;
	  AIConditionBase*	blocked;
	  bool				reset;
	  bool				need_run;
	  bool				idle;
	  bool				skip_idle;
	  bool				aborted;
	  bool				finished;
	};
  private:
	AIThreadSafeSimpleDC<multiplex_state_type>	mState;
	typedef AIAccessConst<multiplex_state_type>	multiplex_state_type_crat;
	typedef AIAccess<multiplex_state_type>		multiplex_state_type_rat;
	typedef AIAccess<multiplex_state_type>		multiplex_state_type_wat;
  protected:
	AIThreadSafeSimpleDC<sub_state_type>	mSubState;
	typedef AIAccessConst<sub_state_type>	sub_state_type_crat;
	typedef AIAccess<sub_state_type>		sub_state_type_rat;
	typedef AIAccess<sub_state_type>		sub_state_type_wat;
  private:
	LLMutex mMultiplexMutex;
	LLMutex mRunMutex;
	S64 mSleep;
	LLPointer<AIStateMachine> mParent;
	state_type mNewParentState;
	bool mAbortParent;
	bool mOnAbortSignalParent;
	struct callback_type {
		typedef boost::signals2::signal<void (bool)> signal_type;
		callback_type(signal_type::slot_type const& slot) { connection = signal.connect(slot); }
		~callback_type() { connection.disconnect(); }
		void callback(bool success) const { signal(success); }
	  private:
		boost::signals2::connection connection;
		signal_type signal;
	};
	callback_type* mCallback;
	AIEngine* mDefaultEngine;
	AIEngine* mYieldEngine;
#ifdef SHOW_ASSERT
	AIThreadID mThreadId;
	base_state_type mDebugLastState;
	bool mDebugShouldRun;
	bool mDebugAborted;
	bool mDebugContPending;
	bool mDebugSetStatePending;
	bool mDebugAdvanceStatePending;
	bool mDebugRefCalled;
#endif
#ifdef CWDEBUG
  protected:
	bool mSMDebug;
#endif
  private:
	U64 mRuntime;
  public:
	AIStateMachine(CWD_ONLY(bool debug)) : mCallback(NULL), mDefaultEngine(NULL), mYieldEngine(NULL),
#ifdef SHOW_ASSERT
		mThreadId(AIThreadID::none), mDebugLastState(bs_killed), mDebugShouldRun(false), mDebugAborted(false), mDebugContPending(false),
		mDebugSetStatePending(false), mDebugAdvanceStatePending(false), mDebugRefCalled(false),
#endif
#ifdef CWDEBUG
		mSMDebug(debug),
#endif
		mRuntime(0)
	{ }
  protected:
	virtual ~AIStateMachine()
	{
#ifdef SHOW_ASSERT
	  base_state_type state = multiplex_state_type_rat(mState)->base_state;
	  llassert(state == bs_killed || state == bs_reset);
#endif
	}
  public:
	void run(AIStateMachine* parent, state_type new_parent_state, bool abort_parent = true, bool on_abort_signal_parent = true, AIEngine* default_engine = &gMainThreadEngine);
	void run(callback_type::signal_type::slot_type const& slot, AIEngine* default_engine = &gMainThreadEngine);
	void run(void) { run(NULL, 0, false, true, mDefaultEngine); }
	void kill(void);
  protected:
	void set_state(state_type new_state);
	void idle(void);
	void wait(AIConditionBase& condition);
	void finish(void);
	void yield(void);
	void yield(AIEngine* engine);
	void yield_frame(unsigned int frames);
	void yield_ms(unsigned int ms);
	bool yield_if_not(AIEngine* engine);
  public:
	void abort(void);
	void cont(void);
	void advance_state(state_type new_state);
	bool signalled(void);
  public:
	bool running(void) const { return multiplex_state_type_crat(mState)->base_state == bs_multiplex; }
	bool waiting(void) const
	{
	  multiplex_state_type_crat state_r(mState);
	  return state_r->base_state == bs_multiplex && sub_state_type_crat(mSubState)->idle;
	}
	bool waiting_or_aborting(void) const
	{
	  multiplex_state_type_crat state_r(mState);
	  return state_r->base_state == bs_abort || ( state_r->base_state == bs_multiplex && sub_state_type_crat(mSubState)->idle);
	}
	bool active(AIEngine const* engine) const { return multiplex_state_type_crat(mState)->current_engine == engine; }
	bool aborted(void) const { return sub_state_type_crat(mSubState)->aborted; }
	typedef state_type AIStateMachine::* const bool_type;
	operator bool_type() const
	{
	  sub_state_type_crat sub_state_r(mSubState);
	  return (sub_state_r->finished && !sub_state_r->aborted) ? &AIStateMachine::mNewParentState : 0;
	}
	char const* state_str(base_state_type state);
#ifdef CWDEBUG
	char const* event_str(event_type event);
#endif
	void add(U64 count) { mRuntime += count; }
	U64 getRuntime(void) const { return mRuntime; }
	virtual const char* getName() const = 0;
  protected:
	virtual void initialize_impl(void) = 0;
	virtual void multiplex_impl(state_type run_state) = 0;
	virtual void abort_impl(void) { }
	virtual void finish_impl(void) { }
	virtual char const* state_str_impl(state_type run_state) const = 0;
	virtual void force_killed(void);
  private:
	void reset(void);
	void multiplex(event_type event);
	state_type begin_loop(base_state_type base_state);
	void callback(void);
	bool sleep(U64 current_time)
	{
	  if (mSleep == 0)
		return false;
	  else if (mSleep < 0)
		++mSleep;
	  else if ((U64)mSleep <= current_time)
		mSleep = 0;
	  return mSleep != 0;
	}
	friend class AIEngine;
};
bool AIEngine::QueueElementComp::operator()(QueueElement const& e1, QueueElement const& e2) const
{
  return e1.mStateMachine->getRuntime() < e2.mStateMachine->getRuntime();
}
#define AI_CASE_RETURN(x) do { case x: return #x; } while(0)
#endif
