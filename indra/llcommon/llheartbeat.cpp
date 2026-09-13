/**
 * @file llheartbeat.cpp
 * @brief Class encapsulating logic for telling a watchdog that we live.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#include <errno.h>
#include <signal.h>
#include "linden_common.h"
#include "llapp.h"
#include "llheartbeat.h"
LLHeartbeat::LLHeartbeat(F32 secs_between_heartbeat,
			 F32 aggressive_heartbeat_panic_secs,
			 F32 aggressive_heartbeat_max_blocking_secs)
	: mSecsBetweenHeartbeat(secs_between_heartbeat),
	  mAggressiveHeartbeatPanicSecs(aggressive_heartbeat_panic_secs),
	  mAggressiveHeartbeatMaxBlockingSecs(aggressive_heartbeat_max_blocking_secs),
	  mSuppressed(false)
{
	mBeatTimer.reset();
	mBeatTimer.setTimerExpirySec(mSecsBetweenHeartbeat);
	mPanicTimer.reset();
	mPanicTimer.setTimerExpirySec(mAggressiveHeartbeatPanicSecs);
}
LLHeartbeat::~LLHeartbeat()
{
}
void
LLHeartbeat::setSuppressed(bool is_suppressed)
{
	mSuppressed = is_suppressed;
}
int
LLHeartbeat::rawSend()
{
#if LL_WINDOWS
	return 0;
#else
	if (mSuppressed)
		return 0;
	int result;
#ifndef LL_DARWIN
	static union sigval dummy;
	result = sigqueue(getppid(), LL_HEARTBEAT_SIGNAL, dummy);
#else
	result = kill(getppid(), LL_HEARTBEAT_SIGNAL);
#endif
	if (result == 0)
		return 0;
	int err = errno;
	if (err == EAGAIN)
		return 1;
	return -1;
#endif
}
int
LLHeartbeat::rawSendWithTimeout(F32 timeout_sec)
{
	int result = 0;
	mTimeoutTimer.reset();
	mTimeoutTimer.setTimerExpirySec(timeout_sec);
	do {
		result = rawSend();
	} while (result==1 && !mTimeoutTimer.hasExpired());
	return result;
}
bool
LLHeartbeat::send(F32 timeout_sec)
{
	bool total_success = false;
	int result = 1;
	if (timeout_sec > 0.f) {
		result = rawSendWithTimeout(timeout_sec);
	} else {
		if (mBeatTimer.hasExpired()) {
			result = rawSend();
		}
	}
	if (result == -1) {
	} else if (result == 0) {
		total_success = true;
	} else {
	}
	if (total_success) {
		mBeatTimer.reset();
		mBeatTimer.setTimerExpirySec(mSecsBetweenHeartbeat);
		mPanicTimer.reset();
		mPanicTimer.setTimerExpirySec(mAggressiveHeartbeatPanicSecs);
	} else {
	}
	if (mPanicTimer.hasExpired()) {
		LL_WARNS() << "Unable to deliver heartbeat to launcher for " << mPanicTimer.getElapsedTimeF32() << " seconds.  Going to try very hard for up to " << mAggressiveHeartbeatMaxBlockingSecs << " seconds." << LL_ENDL;
		result = rawSendWithTimeout(mAggressiveHeartbeatMaxBlockingSecs);
		if (result == 0) {
			total_success = true;
		} else {
			LL_WARNS() << "Could not deliver heartbeat to launcher even after trying very hard for " << mAggressiveHeartbeatMaxBlockingSecs << " seconds." << LL_ENDL;
		}
		mPanicTimer.reset();
		mPanicTimer.setTimerExpirySec(mAggressiveHeartbeatPanicSecs);
	}
	return total_success;
}
