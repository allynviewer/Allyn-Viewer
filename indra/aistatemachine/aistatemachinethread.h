/**
 * @file aistatemachinethread.h
 * @brief Run code in a thread.
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
 *   23/01/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AISTATEMACHINETHREAD_H
#define AISTATEMACHINETHREAD_H
#include "aistatemachine.h"
#include "llthread.h"
#include "aithreadsafe.h"
#include <boost/format.hpp>
#ifdef EXAMPLE_CODE
class HelloWorldThread : public AIThreadImpl {
  private:
	bool mStdErr;
	bool mSuccess;
  public:
	HelloWorldThread(void) : AIThreadImpl("HelloWorldThread"),
		mStdErr(false), mSuccess(false) { }
	void init(bool err)	{ mStdErr = err; }
	bool successful(void) const { return mSuccess; }
	bool run(void)
	{
	  if (mStdErr)
		std::cerr << "Hello world" << std::endl;
	  else
		std::cout << "Hello world" << std::endl;
	  mSuccess = true;
	  return true;
	}
};
enum hello_world_state_type {
  HelloWorld_start = AIStateMachine::max_state,
  HelloWorld_done
};
class HelloWorld : public AIStateMachine {
  private:
	LLPointer<AIStateMachineThread<HelloWorldThread> > mHelloWorld;
	bool mErr;
  public:
	HelloWorld() : mHelloWorld(new AIStateMachineThread<HelloWorldThread>), mErr(false) { }
	void init(bool err) { mErr = err; }
  protected:
	~HelloWorld() { }
	void initialize_impl(void);
	void multiplex_impl(state_type run_state);
	char const* state_str_impl(state_type run_state) const
	{
	  switch(run_state)
	  {
		AI_CASE_RETURN(HelloWorld_start);
		AI_CASE_RETURN(HelloWorld_done);
	  }
	  return "UNKNOWN STATE";
	}
};
void HelloWorld::initialize_impl(void)
{
  mHelloWorld->thread_impl().init(mErr);
  set_state(HelloWorld_start);
}
void HelloWorld::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
	case HelloWorld_start:
	{
	  mHelloWorld->run(this, HelloWorld_done);
	  idle(HelloWorld_start);
	  break;
	}
	case HelloWorld_done:
	{
	  if (mHelloWorld->thread_impl().successful())
		finish();
	  else
		abort();
	  break;
	}
  }
}
#endif
class AIStateMachineThreadBase;
class AIThreadImpl {
  private:
    template<typename THREAD_IMPL> friend class AIStateMachineThread;
	typedef AIAccess<AIStateMachineThreadBase*> StateMachineThread_wat;
	AIThreadSafeSimpleDC<AIStateMachineThreadBase*> mStateMachineThread;
  public:
	virtual bool run(void) = 0;
	bool thread_done(bool result);
	bool state_machine_done(LLThread* threadp);
#ifdef LL_DEBUG
  private:
	char const* mName;
  protected:
	AIThreadImpl(char const* name = "AIStateMachineThreadBase::Thread") : mName(name) { }
  public:
	char const* getName(void) const { return mName; }
#endif
  protected:
	virtual ~AIThreadImpl() { }
};
class AIStateMachineThreadBase : public AIStateMachine {
  private:
	class Thread;
  protected:
	typedef AIStateMachine direct_base_type;
	enum thread_state_type {
	  start_thread = direct_base_type::max_state,
	  wait_stopped
	};
  public:
	static state_type const max_state = wait_stopped + 1;
  protected:
	AIStateMachineThreadBase(CWD_ONLY(bool debug))
#ifdef CWDEBUG
	  : AIStateMachine(debug)
#endif
	{ }
  private:
	void initialize_impl(void);
	void multiplex_impl(state_type run_state);
	void abort_impl(void);
	char const* state_str_impl(state_type run_state) const;
	virtual AIThreadImpl& impl(void) = 0;
  private:
	Thread* mThread;
	bool mAbort;
  public:
	void schedule_abort(bool do_abort) { mAbort = do_abort; }
};
template<typename THREAD_IMPL>
class AIStateMachineThread : public AIStateMachineThreadBase {
  private:
	THREAD_IMPL mThreadImpl;
  public:
	AIStateMachineThread(CWD_ONLY(bool debug))
#ifdef CWDEBUG
		: AIStateMachineThreadBase(debug)
#endif
	{
	  *AIThreadImpl::StateMachineThread_wat(mThreadImpl.mStateMachineThread) = this;
	}
	THREAD_IMPL& thread_impl(void) { return mThreadImpl; }
	const char* getName() const
	{
#define STRIZE(arg) #arg
		return (boost::format("%1%%2%%3%") % "AIStateMachineThread<" % STRIZE(THREAD_IMPL) % ">").str().c_str();
#undef STRIZE
	}
  protected:
	AIThreadImpl& impl(void) { return mThreadImpl; }
};
#endif
