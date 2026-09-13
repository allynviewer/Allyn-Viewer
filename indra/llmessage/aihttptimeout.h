/**
 * @file aihttptimeout.h
 * @brief HTTP timeout control class.
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
 *   28/04/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AIHTTPTIMEOUT_H
#define AIHTTPTIMEOUT_H
#include <vector>
#ifndef HTTPTIMEOUT_TESTSUITE
#include "llrefcount.h"
#include "aithreadsafe.h"
#include <curl/curl.h>
#ifdef DEBUG_CURLIO
#include "debug_libcurl.h"
#endif
#else
#include <sys/types.h>
class LLRefCount { };
template<typename T> struct AIAccess;
typedef int CURLcode;
#define ASSERT_ONLY_COMMA(...) , __VA_ARGS__
#endif
class AIHTTPTimeoutPolicy;
namespace AICurlPrivate {
  class BufferedCurlEasyRequest;
}
typedef AIAccess<AICurlPrivate::BufferedCurlEasyRequest> AICurlEasyRequest_wat;
namespace AICurlPrivate {
class CurlEasyRequest;
class ThreadSafeBufferedCurlEasyRequest;
namespace curlthread {
class HTTPTimeout : public LLRefCount {
  private:
	AIHTTPTimeoutPolicy const* mPolicy;
	std::vector<U32> mBuckets;
	U16 mBucket;
	bool mNothingReceivedYet;
	bool mLowSpeedOn;
	bool mLastBytesSent;
	bool mBeingRedirected;
	bool mUploadFinished;
	S32 mLastSecond;
	S32 mOverwriteSecond;
	U32 mTotalBytes;
	U64 mLowSpeedClock;
	U64 mStalled;
  public:
	static F64 const sClockWidth_10ms;
	static F64 const sClockWidth_40ms;
	static U64 sTime_10ms;
#ifdef CWDEBUG
	ThreadSafeBufferedCurlEasyRequest* mLockObj;
#endif
  public:
	HTTPTimeout(AIHTTPTimeoutPolicy const* policy, ThreadSafeBufferedCurlEasyRequest* lock_obj) :
		mPolicy(policy), mNothingReceivedYet(true), mLowSpeedOn(false), mLastBytesSent(false), mBeingRedirected(false), mUploadFinished(false), mStalled((U64)-1)
#ifdef CWDEBUG
		, mLockObj(lock_obj)
#endif
		{ }
	void being_redirected(void);
	void upload_starting(void);
	void upload_finished(void);
	bool data_sent(size_t n, bool finished);
	bool data_received(size_t n ASSERT_ONLY_COMMA(bool upload_error_status = false));
	void done(AICurlEasyRequest_wat const& curlEasyRequest_w, CURLcode code);
	bool has_stalled(void) { return mStalled < sTime_10ms && !maybe_upload_finished(); }
	void print_diagnostics(CurlEasyRequest const* curl_easy_request, char const* eff_url);
#ifdef CWDEBUG
	void* get_lockobj(void) const { return mLockObj; }
#endif
  private:
	void reset_lowspeed(void);
	bool lowspeed(size_t bytes, bool finished = false);
	bool maybe_upload_finished(void);
};
}
}
#ifdef CWDEBUG
extern bool gCurlIo;
#endif
#endif
