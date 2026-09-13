/**
 * @file aistatemachinethread.cpp
 * @brief Implementation of AIStateMachineThread
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
#include "linden_common.h"
#include "aistatemachinethread.h"
class AIStateMachineThreadBase::Thread : public LLThread {
  private:
	LLPointer<AIStateMachineThreadBase> mImpl;
	bool mNeedCleanup;
  public:
	Thread(AIStateMachineThreadBase* impl) :
#ifdef LL_DEBUG
	  LLThread(impl->impl().getName()),
#else
	  LLThread("AIStateMachineThreadBase::Thread"),
#endif
	  mImpl(impl) { }
  protected:
	void run(void)
	{
	  mNeedCleanup = mImpl->impl().thread_done(mImpl->impl().run());
	}
	void terminated(void)
	{
	  mStatus = STOPPED;
	  if (mNeedCleanup)
	  {
		Thread::completed(this);
	  }
	}
  public:
	static Thread* allocate(AIStateMachineThreadBase* impl) { return new Thread(impl); }
	static void completed(Thread* threadp) { delete threadp; }
};
char const* AIStateMachineThreadBase::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(start_thread);
	AI_CASE_RETURN(wait_stopped);
  }
  return "UNKNOWN STATE";
}
void AIStateMachineThreadBase::initialize_impl(void)
{
  mThread = NULL;
  mAbort = false;
  set_state(start_thread);
}
void AIStateMachineThreadBase::multiplex_impl(state_type run_state)
{
  switch(run_state)
  {
	case start_thread:
	  mThread = Thread::allocate(this);
	  set_state(wait_stopped);
	  idle();
	  mThread->start();
	  break;
	case wait_stopped:
	  if (!mThread->isStopped())
	  {
		yield();
		break;
	  }
	  Thread::completed(mThread);
	  mThread = NULL;
	  if (mAbort)
		abort();
	  else
		finish();
	  break;
  }
}
void AIStateMachineThreadBase::abort_impl(void)
{
  if (mThread)
  {
	bool need_cleanup = impl().state_machine_done(mThread);
	if (need_cleanup)
	{
	  while (!mThread->isStopped())
	  {
		mThread->yield();
	  }
	  Thread::completed(mThread);
	}
	else
	{
	  LL_WARNS() << "Thread state machine aborted while the thread is still running. That is a waste of CPU and should be avoided." << LL_ENDL;
	}
  }
}
bool AIThreadImpl::state_machine_done(LLThread* threadp)
{
  StateMachineThread_wat state_machine_thread_w(mStateMachineThread);
  AIStateMachineThreadBase* state_machine_thread = *state_machine_thread_w;
  bool need_cleanup = !state_machine_thread;
  if (!need_cleanup)
  {
	threadp->setQuitting();
	*state_machine_thread_w = NULL;
  }
  return need_cleanup;
}
bool AIThreadImpl::thread_done(bool result)
{
  StateMachineThread_wat state_machine_thread_w(mStateMachineThread);
  AIStateMachineThreadBase* state_machine_thread = *state_machine_thread_w;
  bool need_cleanup = !state_machine_thread;
  if (!need_cleanup)
  {
	llassert(state_machine_thread->waiting_or_aborting());
	state_machine_thread->schedule_abort(!result);
	state_machine_thread->cont();
	*state_machine_thread_w = NULL;
  }
  return need_cleanup;
}
