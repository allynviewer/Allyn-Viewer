/**
 * @file aicurleasyrequeststatemachine.cpp
 * @brief Implementation of AICurlEasyRequestStateMachine
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
 *   06/05/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#include "linden_common.h"
#include "aicurleasyrequeststatemachine.h"
#include "aihttptimeoutpolicy.h"
#include "llcontrol.h"
enum curleasyrequeststatemachine_state_type {
  AICurlEasyRequestStateMachine_addRequest = AIStateMachine::max_state,
  AICurlEasyRequestStateMachine_waitAdded,
  AICurlEasyRequestStateMachine_timedOut,
  AICurlEasyRequestStateMachine_removed,
  AICurlEasyRequestStateMachine_removed_after_finished,
  AICurlEasyRequestStateMachine_bad_file_descriptor
};
char const* AICurlEasyRequestStateMachine::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_addRequest);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_waitAdded);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_timedOut);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_removed);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_removed_after_finished);
	AI_CASE_RETURN(AICurlEasyRequestStateMachine_bad_file_descriptor);
  }
  return "UNKNOWN STATE";
}
void AICurlEasyRequestStateMachine::initialize_impl(void)
{
  {
	AICurlEasyRequest_wat curlEasyRequest_w(*mCurlEasyRequest);
	llassert(curlEasyRequest_w->is_finalized());
	curlEasyRequest_w->send_handle_events_to(this);
  }
  mAdded = false;
  mTimedOut = false;
  mFinished = false;
  mHandled = false;
  set_state(AICurlEasyRequestStateMachine_addRequest);
}
void AICurlEasyRequestStateMachine::added_to_multi_handle(AICurlEasyRequest_wat&)
{
}
void AICurlEasyRequestStateMachine::finished(AICurlEasyRequest_wat&)
{
  mFinished = true;
}
void AICurlEasyRequestStateMachine::removed_from_multi_handle(AICurlEasyRequest_wat&)
{
  llassert(mFinished || mTimedOut);
  advance_state(mFinished ? AICurlEasyRequestStateMachine_removed_after_finished : AICurlEasyRequestStateMachine_removed);
}
void AICurlEasyRequestStateMachine::bad_file_descriptor(AICurlEasyRequest_wat&)
{
  if (!mFinished)
  {
	mFinished = true;
	advance_state(AICurlEasyRequestStateMachine_bad_file_descriptor);
  }
}
#ifdef SHOW_ASSERT
void AICurlEasyRequestStateMachine::queued_for_removal(AICurlEasyRequest_wat&)
{
  llassert(mFinished || mTimedOut);
}
#endif
void AICurlEasyRequestStateMachine::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
	case AICurlEasyRequestStateMachine_addRequest:
	{
	  set_state(AICurlEasyRequestStateMachine_waitAdded);
	  idle();
	  bool empty_url = AICurlEasyRequest_rat(*mCurlEasyRequest)->getLowercaseServicename().empty();
	  if (empty_url)
	  {
		AICurlEasyRequest_wat(*mCurlEasyRequest)->aborted(HTTP_INTERNAL_ERROR_OTHER, "Not a valid URL.");
		abort();
		break;
	  }
	  mAdded = true;
	  mCurlEasyRequest.addRequest();
	  if (mTotalDelayTimeout > 0.f)
	  {
		mTimer = new AITimer;
		mTimer->setInterval(mTotalDelayTimeout);
		mTimer->run(this, AICurlEasyRequestStateMachine_timedOut, false, false);
	  }
	  break;
	}
	case AICurlEasyRequestStateMachine_waitAdded:
	{
	  idle();
	  break;
	}
	case AICurlEasyRequestStateMachine_timedOut:
	{
	  mTimedOut = true;
	  llassert(mAdded);
	  mAdded = false;
	  mCurlEasyRequest.removeRequest();
	  idle();
	  break;
	}
	case AICurlEasyRequestStateMachine_removed_after_finished:
	{
	  if (!mHandled)
	  {
		mHandled = true;
		if (mTimer)
		{
		  mTimer->abort();
		}
		AICurlEasyRequest_wat easy_request_w(*mCurlEasyRequest);
		easy_request_w->processOutput();
	  }
	  mTimedOut = false;
	}
	case AICurlEasyRequestStateMachine_removed:
	{
	  if (mTimedOut)
	  {
		AICurlEasyRequest_wat(*mCurlEasyRequest)->aborted(HTTP_INTERNAL_ERROR_CURL_LOCKUP, "Request timeout, aborted.");
		abort();
	  }
	  else
		finish();
	  break;
	}
	case AICurlEasyRequestStateMachine_bad_file_descriptor:
	{
	  AICurlEasyRequest_wat(*mCurlEasyRequest)->aborted(HTTP_INTERNAL_ERROR_CURL_BADSOCKET, "File descriptor went bad! Aborted.");
	  abort();
	}
  }
}
void AICurlEasyRequestStateMachine::abort_impl(void)
{
  DoutEntering(dc::curl, "AICurlEasyRequestStateMachine::abort_impl() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  if (mAdded)
  {
	mAdded = false;
	mCurlEasyRequest.removeRequest();
  }
}
void AICurlEasyRequestStateMachine::finish_impl(void)
{
  DoutEntering(dc::curl, "AICurlEasyRequestStateMachine::finish_impl() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  {
	AICurlEasyRequest_wat curl_easy_request_w(*mCurlEasyRequest);
	curl_easy_request_w->send_buffer_events_to(NULL);
	curl_easy_request_w->send_handle_events_to(NULL);
	curl_easy_request_w->revokeCallbacks();
  }
  if (mTimer)
  {
	if (!mHandled)
	  mTimer->abort();
  }
}
AICurlEasyRequestStateMachine::AICurlEasyRequestStateMachine(CWD_ONLY(bool debug)) :
#ifdef CWDEBUG
	AIStateMachine(debug),
#endif
    mTotalDelayTimeout(AIHTTPTimeoutPolicy::getDebugSettingsCurlTimeout().getTotalDelay())
{
  Dout(dc::statemachine(mSMDebug), "Calling AICurlEasyRequestStateMachine(void) [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  AICurlInterface::Stats::AICurlEasyRequestStateMachine_count++;
}
void AICurlEasyRequestStateMachine::setTotalDelayTimeout(F32 totalDelayTimeout)
{
  mTotalDelayTimeout = totalDelayTimeout;
}
AICurlEasyRequestStateMachine::~AICurlEasyRequestStateMachine()
{
  Dout(dc::statemachine(mSMDebug), "Calling ~AICurlEasyRequestStateMachine() [" << (void*)this << "] [" << (void*)mCurlEasyRequest.get() << "]");
  --AICurlInterface::Stats::AICurlEasyRequestStateMachine_count;
}
