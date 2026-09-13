/**
 * @file aicurleasyrequest.h
 * @brief Perform a curl easy request.
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
#ifndef AICURLEASYREQUEST_H
#define AICURLEASYREQUEST_H
#include "aistatemachine.h"
#include "aitimer.h"
#include "aicurl.h"
class AICurlEasyRequestStateMachine : public AIStateMachine, public AICurlEasyHandleEvents {
  public:
	AICurlEasyRequestStateMachine(CWD_ONLY(bool debug = false));
	AICurlEasyRequest mCurlEasyRequest;
  private:
	bool mAdded;
	bool mTimedOut;
	bool mFinished;
	bool mHandled;
	LLPointer<AITimer> mTimer;
	F32 mTotalDelayTimeout;
  public:
	void setTotalDelayTimeout(F32 totalDelayTimeout);
  protected:
	void added_to_multi_handle(AICurlEasyRequest_wat&);
	void finished(AICurlEasyRequest_wat&);
	void removed_from_multi_handle(AICurlEasyRequest_wat&);
	void bad_file_descriptor(AICurlEasyRequest_wat&);
#ifdef SHOW_ASSERT
	void queued_for_removal(AICurlEasyRequest_wat&);
#endif
  protected:
	~AICurlEasyRequestStateMachine();
	void initialize_impl(void);
	void multiplex_impl(state_type run_state);
	void abort_impl(void);
	void finish_impl(void);
	char const* state_str_impl(state_type run_state) const;
};
#endif
