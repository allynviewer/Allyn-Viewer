/**
 * @file aihttptimeout.cpp
 * @brief Implementation of HTTPTimeout
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
#ifndef HTTPTIMEOUT_TESTSUITE
#include "linden_common.h"
#include "aihttptimeoutpolicy.h"
#include "lltimer.h"
#include "aicurl.h"
#undef AICurlPrivate
#else
#include "../llcommon/stdtypes.h"
#include <iostream>
#include <cassert>
#define llassert assert
#define llassert_always assert
#define DoutCurl(...) do { std::cout << __VA_ARGS__ ; } while(0)
#define Debug(...) do { } while(0)
#define llmin std::min
#define llwarns std::cout
#define llendl std::endl
int const CURLE_OPERATION_TIMEDOUT  = 1;
int const CURLE_COULDNT_RESOLVE_HOST = 2;
int const CURLINFO_NAMELOOKUP_TIME = 3;
F64 calc_clock_frequency(void) { return 1000000.0; }
template<typename T>
struct AIAccess {
  T* mP;
  AIAccess(T* p) : mP(p) { }
  T* operator->() const { return mP; }
};
struct AIHTTPTimeoutPolicy {
  U16 getReplyDelay(void) const { return 60; }
  U16 getLowSpeedTime(void) const { return 30; }
  U32 getLowSpeedLimit(void) const { return 7000; }
  static bool connect_timed_out(std::string const&) { return false; }
};
namespace AICurlPrivate {
class BufferedCurlEasyRequest {
public:
  char const* getLowercaseHostname(void) const { return "hostname.com"; }
  char const* getLowercaseServicename(void) const { return "hostname.com:12047"; }
  void getinfo(const int&, double* p) { *p = 0.1; }
};
}
#endif
#include "aihttptimeout.h"
#ifdef CWDEBUG
bool gCurlIo;
#endif
namespace AICurlPrivate {
namespace curlthread {
F64 const HTTPTimeout::sClockWidth_10ms = 100.0 / calc_clock_frequency();
F64 const HTTPTimeout::sClockWidth_40ms = HTTPTimeout::sClockWidth_10ms * 0.25;
U64 HTTPTimeout::sTime_10ms;
bool HTTPTimeout::data_sent(size_t n, bool finished)
{
  if (!mLowSpeedOn)
  {
	reset_lowspeed();
  }
  return lowspeed(n, finished);
}
void HTTPTimeout::reset_lowspeed(void)
{
  mLowSpeedClock = sTime_10ms;
  mLowSpeedOn = true;
  mLastBytesSent = false;
  mLastSecond = -1;
  mStalled = (U64)-1;
  DoutCurl("reset_lowspeed: mLowSpeedClock = " << mLowSpeedClock << "; mStalled = -1");
}
void HTTPTimeout::being_redirected(void)
{
  mBeingRedirected = true;
}
void HTTPTimeout::upload_starting(void)
{
  llassert(!mUploadFinished || mBeingRedirected);
  mUploadFinished = false;
  reset_lowspeed();
}
void HTTPTimeout::upload_finished(void)
{
  if (mUploadFinished)
  {
	return;
  }
  mUploadFinished = true;
  mBeingRedirected = false;
  mLowSpeedOn = false;
  mStalled = sTime_10ms + 100 * mPolicy->getReplyDelay();
  DoutCurl("upload_finished: mStalled set to Time_10ms (" << sTime_10ms << ") + " << (mStalled - sTime_10ms) << " (" << mPolicy->getReplyDelay() << " seconds)");
}
bool HTTPTimeout::data_received(size_t n
#ifdef CWDEBUG
	ASSERT_ONLY_COMMA(bool upload_error_status)
#else
	ASSERT_ONLY_COMMA(bool)
#endif
	)
{
  if (mNothingReceivedYet && n > 0)
  {
	if (!mUploadFinished)
	{
	  Debug(llassert(upload_error_status || AICurlEasyRequest_wat(*mLockObj)->mDebugIsHeadOrGetMethod || !dc::curlio.is_on() || gCurlIo));
	  upload_finished();
	}
	mNothingReceivedYet = false;
	reset_lowspeed();
  }
  return mLowSpeedOn ? lowspeed(n) : false;
}
bool HTTPTimeout::lowspeed(size_t bytes, bool finished)
{
  S32 second = (sTime_10ms - mLowSpeedClock) / 100;
  llassert(second >= 0);
  mLastBytesSent = finished;
  if (second == mLastSecond)
  {
	mTotalBytes += bytes;
	mBuckets[mBucket] += bytes;
	return false;
  }
  U16 const low_speed_time = mPolicy->getLowSpeedTime();
  if (low_speed_time > mBuckets.size())
  {
	mBuckets.resize(low_speed_time, 0);
  }
  S32 s = mLastSecond;
  mLastSecond = second;
  if (s == -1)
  {
	mBucket = 0;
	mOverwriteSecond = second + low_speed_time;
	mTotalBytes = bytes;
	mBuckets[mBucket] = bytes;
	return false;
  }
  U16 bucket = mBucket;
  while(1)
  {
	if (++bucket == low_speed_time)
	  bucket = 0;
	if (++s == second)
	  break;
	if (s >= mOverwriteSecond)
	{
	  mTotalBytes -= mBuckets[bucket];
	}
	mBuckets[bucket] = 0;
  }
  mBucket = bucket;
  if (s >= mOverwriteSecond)
  {
	mTotalBytes -= mBuckets[mBucket];
  }
  mTotalBytes += bytes;
  mBuckets[mBucket] = bytes;
  U32 const low_speed_limit = mPolicy->getLowSpeedLimit();
  U32 mintotalbytes = low_speed_limit * low_speed_time;
  DoutCurl("Transfered " << mTotalBytes << " bytes in " << llmin(second, (S32)low_speed_time) << " seconds after " << second << " second" << ((second == 1) ? "" : "s") << ".");
  if (second >= low_speed_time)
  {
	DoutCurl("Average transfer rate is " << (mTotalBytes / low_speed_time) << " bytes/s (low speed limit is " << low_speed_limit << " bytes/s)");
	if (mTotalBytes < mintotalbytes)
	{
	  if (finished)
	  {
		LL_WARNS() <<
#ifdef CWDEBUG
		(void*)get_lockobj() << ": "
#endif
		"Transfer rate timeout (average transfer rate below " << low_speed_limit <<
		" bytes/s for more than " << low_speed_time << " second" << ((low_speed_time == 1) ? "" : "s") <<
		") but we just sent the LAST bytes! Waiting an additional 4 seconds." << LL_ENDL;
		mStalled = sTime_10ms + 400;
		DoutCurl("mStalled set to sTime_10ms (" << sTime_10ms << ") + 400 (4 seconds)");
		return false;
	  }
	  LL_WARNS() <<
#ifdef CWDEBUG
		(void*)get_lockobj() << ": "
#endif
		"aborting slow connection (average transfer rate below " << low_speed_limit <<
		" bytes/s for more than " << low_speed_time << " second" << ((low_speed_time == 1) ? "" : "s") << ")." << LL_ENDL;
	  return true;
	}
  }
  llassert_always(mintotalbytes > 0);
  S32 max_stall_time = low_speed_time - second;
  if (mTotalBytes >= mintotalbytes)
  {
	max_stall_time = mOverwriteSecond - second - 1;
	U32 total_bytes = mTotalBytes;
	int bucket = -1;
	do
	{
	  ++max_stall_time;
	  ++bucket;
	  llassert_always(bucket < low_speed_time);
	  total_bytes -= mBuckets[bucket];
	}
	while(total_bytes >= mintotalbytes);
  }
  mStalled = sTime_10ms + 100 * max_stall_time;
  DoutCurl("mStalled set to sTime_10ms (" << sTime_10ms << ") + " << (mStalled - sTime_10ms) << " (" << max_stall_time << " seconds)");
  return false;
}
void HTTPTimeout::done(AICurlEasyRequest_wat const& curlEasyRequest_w, CURLcode code)
{
  if (code == CURLE_OPERATION_TIMEDOUT || code == CURLE_COULDNT_RESOLVE_HOST)
  {
	bool dns_problem = false;
	if (code == CURLE_COULDNT_RESOLVE_HOST)
	{
	  LL_WARNS() << "Failed to resolve hostname " << curlEasyRequest_w->getLowercaseHostname() << LL_ENDL;
	  dns_problem = true;
	}
	else if (mNothingReceivedYet)
	{
	  double namelookup_time;
	  curlEasyRequest_w->getinfo(CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
	  dns_problem = (namelookup_time == 0);
	}
	if (dns_problem)
	{
	  AIHTTPTimeoutPolicy::connect_timed_out(curlEasyRequest_w->getLowercaseHostname());
	}
  }
  mLowSpeedOn = false;
  mStalled = (U64)-1;
  DoutCurl("done: mStalled set to -1");
}
bool HTTPTimeout::maybe_upload_finished(void)
{
  if (!mUploadFinished && mLastBytesSent)
  {
	upload_finished();
	return true;
  }
  return false;
}
#define LOWRESTIMER LL_WINDOWS
void HTTPTimeout::print_diagnostics(CurlEasyRequest const* curl_easy_request, char const* eff_url)
{
#ifndef HTTPTIMEOUT_TESTSUITE
  LL_WARNS() << "Request to \"" << curl_easy_request->getLowercaseServicename() << "\" timed out for " << curl_easy_request->getTimeoutPolicy()->name() << LL_ENDL;
  LL_INFOS() << "Effective URL: \"" << eff_url << "\"." << LL_ENDL;
  double namelookup_time, connect_time, appconnect_time, pretransfer_time, starttransfer_time;
  curl_easy_request->getinfo(CURLINFO_NAMELOOKUP_TIME, &namelookup_time);
  curl_easy_request->getinfo(CURLINFO_CONNECT_TIME, &connect_time);
  curl_easy_request->getinfo(CURLINFO_APPCONNECT_TIME, &appconnect_time);
  curl_easy_request->getinfo(CURLINFO_PRETRANSFER_TIME, &pretransfer_time);
  curl_easy_request->getinfo(CURLINFO_STARTTRANSFER_TIME, &starttransfer_time);
  if (namelookup_time == 0
#if LOWRESTIMER
	  && connect_time == 0
#endif
	  )
  {
#if LOWRESTIMER
	LL_INFOS() << "Hostname seems to have been still in the DNS cache." << LL_ENDL;
#else
	LL_WARNS() << "Curl returned CURLE_OPERATION_TIMEDOUT and DNS lookup did not occur according to timings. Apparently the resolve attempt timed out (bad network?)" << LL_ENDL;
	llassert(connect_time == 0);
	llassert(appconnect_time == 0);
	llassert(pretransfer_time == 0);
	llassert(starttransfer_time == 0);
	return;
#endif
  }
  else if (namelookup_time < 500e-6)
  {
#if LOWRESTIMER
	LL_INFOS() << "Hostname was most likely still in DNS cache (or lookup occured in under ~10ms)." << LL_ENDL;
#else
	LL_INFOS() << "Hostname was still in DNS cache." << LL_ENDL;
#endif
  }
  else
  {
	LL_INFOS() << "DNS lookup of " << curl_easy_request->getLowercaseHostname() << " took " << namelookup_time << " seconds." << LL_ENDL;
  }
  if (connect_time == 0
#if LOWRESTIMER
	  && namelookup_time > 0
#endif
	  )
  {
	LL_WARNS() << "Curl returned CURLE_OPERATION_TIMEDOUT and connection did not occur according to timings: apparently the connect attempt timed out (bad network?)" << LL_ENDL;
	llassert(appconnect_time == 0);
	llassert(pretransfer_time == 0);
	llassert(starttransfer_time == 0);
	return;
  }
  if (connect_time - namelookup_time <= 1e-5)
  {
#if LOWRESTIMER
	LL_INFOS() << "The socket was most likely already connected (or you connected to a proxy with a connect time of under ~10 ms)." << LL_ENDL;
#else
	LL_INFOS() << "The socket was already connected (to remote or proxy)." << LL_ENDL;
#endif
	if (appconnect_time == 0)
	{
	  LL_WARNS() << "The SSL/TLS handshake never occurred according to the timings!" << LL_ENDL;
	  return;
	}
	if (appconnect_time - connect_time <= 1e-5)
	{
	  LL_INFOS() << "Connection with HTTP server was already established; this was a re-used connection." << LL_ENDL;
	}
	else
	{
	  LL_INFOS() << "SSL/TLS handshake with HTTP server took " << (appconnect_time - connect_time) << " seconds." << LL_ENDL;
	}
  }
  else
  {
	LL_INFOS() << "Socket connected to remote host (or proxy) in " << (connect_time - namelookup_time) << " seconds." << LL_ENDL;
	if (appconnect_time == 0)
	{
	  LL_WARNS() << "The SSL/TLS handshake never occurred according to the timings!" << LL_ENDL;
	  return;
	}
	LL_INFOS() << "SSL/TLS handshake with HTTP server took " << (appconnect_time - connect_time) << " seconds." << LL_ENDL;
  }
  if (pretransfer_time == 0)
  {
	LL_WARNS() << "The transfer never happened because there was too much in the pipeline (apparently)." << LL_ENDL;
	return;
  }
  else if (pretransfer_time - appconnect_time >= 1e-5)
  {
	LL_INFOS() << "Apparently there was a delay, due to waits in line for the pipeline, of " << (pretransfer_time - appconnect_time) << " seconds before the transfer began." << LL_ENDL;
  }
  if (starttransfer_time == 0)
  {
	LL_WARNS() << "No data was ever received from the server according to the timings." << LL_ENDL;
  }
  else
  {
	LL_INFOS() << "The time it took to send the request to the server plus the time it took before the server started to reply was " << (starttransfer_time - pretransfer_time) << " seconds." << LL_ENDL;
  }
  if (mNothingReceivedYet)
  {
	LL_INFOS() << "No data at all was actually received from the server." << LL_ENDL;
  }
  if (mUploadFinished)
  {
	LL_INFOS() << "The request upload finished successfully." << LL_ENDL;
  }
  else if (mLastBytesSent)
  {
	LL_INFOS() << "All bytes where sent to libcurl for upload." << LL_ENDL;
  }
  if (mLastSecond > 0 && mLowSpeedOn)
  {
	LL_INFOS() << "The " << (mNothingReceivedYet ? "upload" : "download") << " did last " << mLastSecond << " second" << ((mLastSecond == 1) ? "" : "s") << ", before it timed out." << LL_ENDL;
  }
#endif
}
}
}
#ifdef HTTPTIMEOUT_TESTSUITE
int main()
{
  using namespace AICurlPrivate::curlthread;
  AIHTTPTimeoutPolicy policy;
  HTTPTimeout test(&policy, NULL);
}
#endif
