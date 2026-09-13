/** 
 * @file llhttpretrypolicy.h
 * @brief Header for a retry policy class intended for use with http responders.
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
#include "llviewerprecompiledheaders.h"
#include "llhttpretrypolicy.h"
#include "llhttpstatuscodes.h"
#include "aihttpheaders.h"
#include <curl/curl.h>
bool getSecondsUntilRetryAfter(const std::string& retry_after, F32& seconds_to_wait)
{
	char* end = 0;
	double seconds = std::strtod(retry_after.c_str(), &end);
	if (end != 0 && *end == 0)
	{
		seconds_to_wait = (F32)seconds;
		return true;
	}
	time_t date = curl_getdate(retry_after.c_str(), NULL);
	if (-1 == date) return false;
	seconds_to_wait = (F64)date - LLTimer::getTotalSeconds();
	return true;
}
LLAdaptiveRetryPolicy::LLAdaptiveRetryPolicy(F32 min_delay, F32 max_delay, F32 backoff_factor, U32 max_retries, bool retry_on_4xx):
	mMinDelay(min_delay),
	mMaxDelay(max_delay),
	mBackoffFactor(backoff_factor),
	mMaxRetries(max_retries),
	mRetryOn4xx(retry_on_4xx)
{
	init();
}
void LLAdaptiveRetryPolicy::init()
{
	mDelay = mMinDelay;
	mRetryCount = 0;
	mShouldRetry = true;
}
void LLAdaptiveRetryPolicy::reset()
{
	init();
}
bool LLAdaptiveRetryPolicy::getRetryAfter(const AIHTTPReceivedHeaders& headers, F32& retry_header_time)
{
	AIHTTPReceivedHeaders::range_type results;
	return headers.getValues("retry-after", results) && getSecondsUntilRetryAfter(results.first->second, retry_header_time);
}
void LLAdaptiveRetryPolicy::onSuccess()
{
	init();
}
void LLAdaptiveRetryPolicy::onFailure(S32 status, const AIHTTPReceivedHeaders& headers)
{
	F32 retry_header_time;
	bool has_retry_header_time = getRetryAfter(headers,retry_header_time);
	onFailureCommon(status, has_retry_header_time, retry_header_time);
}
void LLAdaptiveRetryPolicy::onFailureCommon(S32 status, bool has_retry_header_time, F32 retry_header_time)
{
	if (!mShouldRetry)
	{
		LL_INFOS() << "keep on failing" << LL_ENDL;
		return;
	}
	if (mRetryCount > 0)
	{
		mDelay = llclamp(mDelay*mBackoffFactor,mMinDelay,mMaxDelay);
	}
	F32 wait_time = mDelay;
	if (has_retry_header_time)
	{
		wait_time = retry_header_time;
	}
	if (mRetryCount>=mMaxRetries)
	{
		LL_INFOS() << "Too many retries " << mRetryCount << ", will not retry" << LL_ENDL;
		mShouldRetry = false;
	}
	if (!mRetryOn4xx && !(status == HTTP_INTERNAL_ERROR_OTHER || ((500 <= status) && (status < 600))))
	{
		LL_INFOS() << "Non-server error " << status << ", will not retry" << LL_ENDL;
		mShouldRetry = false;
	}
	if (mShouldRetry)
	{
		LL_INFOS() << "Retry count " << mRetryCount << " should retry after " << wait_time << LL_ENDL;
		mRetryTimer.reset();
		mRetryTimer.setTimerExpirySec(wait_time);
	}
	mRetryCount++;
}
bool LLAdaptiveRetryPolicy::shouldRetry(F32& seconds_to_wait) const
{
	if (mRetryCount == 0)
	{
		seconds_to_wait = F32_MAX;
		return false;
	}
	seconds_to_wait = mShouldRetry ? (F32) mRetryTimer.getRemainingTimeF32() : F32_MAX;
	return mShouldRetry;
}
