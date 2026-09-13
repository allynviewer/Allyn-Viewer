/** 
 * @file file llhttpretrypolicy.h
 * @brief declarations for http retry policy class.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#ifndef LL_RETRYPOLICY_H
#define LL_RETRYPOLICY_H
#include "lltimer.h"
#include "llthread.h"
class AIHTTPReceivedHeaders;
class LLHTTPRetryPolicy: public LLThreadSafeRefCount
{
public:
	LLHTTPRetryPolicy() {}
	virtual ~LLHTTPRetryPolicy() {}
	virtual void onSuccess() = 0;
	virtual void onFailure(S32 status, const AIHTTPReceivedHeaders& headers) = 0;
	virtual bool shouldRetry(F32& seconds_to_wait) const = 0;
	virtual void reset() = 0;
};
class LLAdaptiveRetryPolicy: public LLHTTPRetryPolicy
{
public:
	LLAdaptiveRetryPolicy(F32 min_delay, F32 max_delay, F32 backoff_factor, U32 max_retries, bool retry_on_4xx = false);
	void onSuccess();
	void reset();
	void onFailure(S32 status, const AIHTTPReceivedHeaders& headers);
	bool shouldRetry(F32& seconds_to_wait) const;
protected:
	void init();
	bool getRetryAfter(const AIHTTPReceivedHeaders& headers, F32& retry_header_time);
	void onFailureCommon(S32 status, bool has_retry_header_time, F32 retry_header_time);
private:
	F32 mMinDelay;
	F32 mMaxDelay;
	F32 mBackoffFactor;
	U32 mMaxRetries;
	F32 mDelay;
	U32 mRetryCount;
	LLTimer mRetryTimer;
	bool mShouldRetry;
	bool mRetryOn4xx;
};
#endif
