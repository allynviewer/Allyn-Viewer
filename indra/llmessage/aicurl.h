/**
 * @file aicurl.h
 * @brief Thread safe wrapper for libcurl.
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
 *   17/03/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AICURL_H
#define AICURL_H
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "llpreprocessor.h"
#include <curl/curl.h>
#ifdef DEBUG_CURLIO
#include "debug_libcurl.h"
#endif
#undef CURLOPT_DNS_USE_GLOBAL_CACHE
#define CURLOPT_DNS_USE_GLOBAL_CACHE do_not_use_CURLOPT_DNS_USE_GLOBAL_CACHE
#include "stdtypes.h"
#include "llatomic.h"
#include "aithreadsafe.h"
#include "aicurlperservice.h"
extern bool gNoVerifySSLCert;
class LLSD;
class LLBufferArray;
class LLChannelDescriptors;
class AIHTTPTimeoutPolicy;
class LLControlGroup;
#ifdef CWDEBUG
#include <libcwd/buf2str.h>
#include <sstream>
#define DoutCurl(x) do { \
	using namespace libcwd; \
	std::ostringstream marker; \
	marker << (void*)this->get_lockobj(); \
	libcw_do.push_marker(); \
	libcw_do.marker().assign(marker.str().data(), marker.str().size()); \
	libcw_do.inc_indent(2); \
	Dout(dc::curl, x); \
	libcw_do.dec_indent(2); \
	libcw_do.pop_marker(); \
  } while(0)
#define DoutCurlEntering(x) do { \
	using namespace libcwd; \
	std::ostringstream marker; \
	marker << (void*)this->get_lockobj(); \
	libcw_do.push_marker(); \
	libcw_do.marker().assign(marker.str().data(), marker.str().size()); \
	libcw_do.inc_indent(2); \
	DoutEntering(dc::curl, x); \
	libcw_do.dec_indent(2); \
	libcw_do.pop_marker(); \
  } while(0)
#else
#define DoutCurl(x) Dout(dc::curl, x << " [" << (void*)this->get_lockobj() << ']')
#define DoutCurlEntering(x) DoutEntering(dc::curl, x << " [" << (void*)this->get_lockobj() << ']')
#endif
class AICurlError : public std::runtime_error {
  public:
	AICurlError(std::string const& message) : std::runtime_error(message) { }
};
class AICurlNoEasyHandle : public AICurlError {
  public:
	AICurlNoEasyHandle(std::string const& message) : AICurlError(message) { }
};
class AICurlNoMultiHandle : public AICurlError {
  public:
	AICurlNoMultiHandle(std::string const& message) : AICurlError(message) { }
};
class AICurlNoBody : public AICurlError {
  public:
	AICurlNoBody(std::string const& message) : AICurlError(message) { }
};
namespace AICurlInterface {
struct Stats {
  static LLAtomicU32 easy_calls;
  static LLAtomicU32 easy_errors;
  static LLAtomicU32 easy_init_calls;
  static LLAtomicU32 easy_init_errors;
  static LLAtomicU32 easy_cleanup_calls;
  static LLAtomicU32 multi_calls;
  static LLAtomicU32 multi_errors;
  static LLAtomicU32 running_handles;
  static LLAtomicU32 AICurlEasyRequest_count;
  static LLAtomicU32 AICurlEasyRequestStateMachine_count;
  static LLAtomicU32 BufferedCurlEasyRequest_count;
  static LLAtomicU32 ResponderBase_count;
  static LLAtomicU32 ThreadSafeBufferedCurlEasyRequest_count;
  static LLAtomicU32 status_count[100];
  static LLAtomicU32 llsd_body_count;
  static LLAtomicU32 llsd_body_parse_error;
  static LLAtomicU32 raw_body_count;
 static void print(void);
 static U32 status2index(U32 status);
 static U32 index2status(U32 index);
};
bool handleCurlMaxTotalConcurrentConnections(LLSD const& newvalue);
bool handleCurlConcurrentConnectionsPerService(LLSD const& newvalue);
bool handleNoVerifySSLCert(LLSD const& newvalue);
void initCurl(void);
void startCurlThread(LLControlGroup* control_group);
void shutdownCurl(void);
void cleanupCurl(void);
std::string getVersionString(void);
void setCAFile(std::string const& file);
void setCAPath(std::string const& file);
U32 getNumHTTPCommands(void);
U32 getNumHTTPQueued(void);
U32 getNumHTTPAdded(void);
U32 getMaxHTTPAdded(void);
U32 getNumHTTPRunning(void);
extern LLControlGroup* sConfigGroup;
}
namespace AICurlPrivate {
  class BufferedCurlEasyRequest;
}
typedef AIAccessConst<AICurlPrivate::BufferedCurlEasyRequest> AICurlEasyRequest_rat;
typedef AIAccess<AICurlPrivate::BufferedCurlEasyRequest> AICurlEasyRequest_wat;
struct AICurlEasyHandleEvents {
	virtual void added_to_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
	virtual void finished(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
	virtual void removed_from_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
	virtual void bad_file_descriptor(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
#ifdef SHOW_ASSERT
	virtual void queued_for_removal(AICurlEasyRequest_wat& curl_easy_request_w) = 0;
#endif
	virtual ~AICurlEasyHandleEvents() { }
};
class AIPostField : public LLThreadSafeRefCount {
  protected:
	char const* mData;
  public:
	AIPostField(char const* data) : mData(data) { }
	char const* data(void) const { return mData; }
};
typedef LLPointer<AIPostField> AIPostFieldPtr;
#include "aicurlprivate.h"
class AICurlEasyRequest {
  private:
	friend class AICurlEasyRequestStateMachine;
	AICurlEasyRequest(void) :
	    mBufferedCurlEasyRequest(new AICurlPrivate::ThreadSafeBufferedCurlEasyRequest) { AICurlInterface::Stats::AICurlEasyRequest_count++; }
  public:
	~AICurlEasyRequest() { --AICurlInterface::Stats::AICurlEasyRequest_count; }
	AICurlEasyRequest(AICurlEasyRequest const& orig) : mBufferedCurlEasyRequest(orig.mBufferedCurlEasyRequest) { AICurlInterface::Stats::AICurlEasyRequest_count++; }
	AIThreadSafeSimple<AICurlPrivate::BufferedCurlEasyRequest>& operator*(void) const { llassert(mBufferedCurlEasyRequest.get()); return *mBufferedCurlEasyRequest; }
	AIThreadSafeSimple<AICurlPrivate::BufferedCurlEasyRequest>* operator->(void) const { llassert(mBufferedCurlEasyRequest.get()); return mBufferedCurlEasyRequest.get(); }
	AIThreadSafeSimple<AICurlPrivate::BufferedCurlEasyRequest>* get(void) const { return mBufferedCurlEasyRequest.get(); }
	bool operator==(AICurlEasyRequest const& cer) const { return mBufferedCurlEasyRequest == cer.mBufferedCurlEasyRequest; }
	bool operator!=(AICurlEasyRequest const& cer) const { return mBufferedCurlEasyRequest != cer.mBufferedCurlEasyRequest; }
	void addRequest(void);
	void removeRequest(void);
#ifdef DEBUG_CURLIO
	void debug(bool debug) { AICurlEasyRequest_wat(*mBufferedCurlEasyRequest)->debug(debug); }
#endif
  private:
	AICurlPrivate::BufferedCurlEasyRequestPtr mBufferedCurlEasyRequest;
  private:
	AICurlEasyRequest& operator=(AICurlEasyRequest const&) { return *this; }
  public:
	void swap(AICurlEasyRequest& cer) { mBufferedCurlEasyRequest.swap(cer.mBufferedCurlEasyRequest); }
  public:
	AICurlPrivate::BufferedCurlEasyRequestPtr const& get_ptr(void) const { return mBufferedCurlEasyRequest; }
	AICurlEasyRequest(AICurlPrivate::BufferedCurlEasyRequestPtr const& ptr) : mBufferedCurlEasyRequest(ptr) { AICurlInterface::Stats::AICurlEasyRequest_count++; }
	explicit AICurlEasyRequest(AICurlPrivate::ThreadSafeBufferedCurlEasyRequest* ptr) : mBufferedCurlEasyRequest(ptr) { AICurlInterface::Stats::AICurlEasyRequest_count++; }
};
#define AICurlPrivate DONTUSE_AICurlPrivate
#endif
