/**
 * @file aicurlprivate.h
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
 *   28/04/2012
 *   Initial version, written by Aleric Inglewood @ SL
 */
#ifndef AICURLPRIVATE_H
#define AICURLPRIVATE_H
#include <sstream>
#include "llatomic.h"
#include "llrefcount.h"
#include "aicurlperservice.h"
#include "aihttptimeout.h"
#include "llhttpclient.h"
class AIHTTPHeaders;
class AICurlEasyRequestStateMachine;
struct AITransferInfo;
namespace AICurlPrivate {
namespace curlthread {
class MultiHandle;
}
void handle_multi_error(CURLMcode code);
inline CURLMcode check_multi_code(CURLMcode code) { AICurlInterface::Stats::multi_calls++; if (code != CURLM_OK) handle_multi_error(code); return code; }
bool curlThreadIsRunning(void);
void wakeUpCurlThread(void);
void stopCurlThread(void);
void clearCommandQueue(void);
#define DECLARE_SETOPT(param_type) \
	  CURLcode setopt(CURLoption option, param_type parameter)
class CurlEasyHandle : public boost::noncopyable, protected AICurlEasyHandleEvents {
  public:
	CurlEasyHandle(void);
	~CurlEasyHandle();
  private:
	CurlEasyHandle& operator=(CurlEasyHandle const*);
  public:
	void reset(void) { llassert(!mActiveMultiHandle); curl_easy_reset(mEasyHandle); }
	DECLARE_SETOPT(long);
	DECLARE_SETOPT(long long);
	DECLARE_SETOPT(void const*);
	DECLARE_SETOPT(curl_debug_callback);
	DECLARE_SETOPT(curl_write_callback);
	DECLARE_SETOPT(curl_ssl_ctx_callback);
	DECLARE_SETOPT(curl_conv_callback);
	DECLARE_SETOPT(curl_progress_callback);
#if 0
	DECLARE_SETOPT(curl_seek_callback);
	DECLARE_SETOPT(curl_ioctl_callback);
	DECLARE_SETOPT(curl_sockopt_callback);
	DECLARE_SETOPT(curl_opensocket_callback);
	DECLARE_SETOPT(curl_closesocket_callback);
	DECLARE_SETOPT(curl_sshkeycallback);
	DECLARE_SETOPT(curl_chunk_bgn_callback);
	DECLARE_SETOPT(curl_chunk_end_callback);
	DECLARE_SETOPT(curl_fnmatch_callback);
#endif
	CURLcode setopt(CURLoption option, U32 parameter) { return setopt(option, (long)parameter); }
	CURLcode setopt(CURLoption option, S32 parameter) { return setopt(option, (long)parameter); }
	char* escape(char* url, int length);
	char* unescape(char* url, int inlength , int* outlength);
  private:
	CURLcode getinfo_priv(CURLINFO info, void* data) const;
  public:
	CURLcode getinfo(CURLINFO info, char** data) const { return getinfo_priv(info, data); }
	CURLcode getinfo(CURLINFO info, curl_slist** data) const { return getinfo_priv(info, data); }
	CURLcode getinfo(CURLINFO info, double* data) const { return getinfo_priv(info, data); }
	CURLcode getinfo(CURLINFO info, long* data) const { return getinfo_priv(info, data); }
#ifdef __LP64__
	CURLcode getinfo(CURLINFO info, S32* data) const { long ldata; CURLcode res = getinfo_priv(info, &ldata); *data = static_cast<S32>(ldata); return res; }
	CURLcode getinfo(CURLINFO info, U32* data) const { long ldata; CURLcode res = getinfo_priv(info, &ldata); *data = static_cast<U32>(ldata); return res; }
#else
	CURLcode getinfo(CURLINFO info, S32* data) const { return getinfo_priv(info, reinterpret_cast<long*>(data)); }
	CURLcode getinfo(CURLINFO info, U32* data) const { return getinfo_priv(info, reinterpret_cast<long*>(data)); }
#endif
	CURLcode perform(void);
	CURLcode pause(int bitmask);
	void setApproved(AIPerService::Approvement* approved) { mApproved = approved; }
	bool approved(void) const { return mApproved; }
	void remove_queued(void) { mQueuedForRemoval = true; }
	void add_queued(void) { mQueuedForRemoval = false; if (mApproved) { mApproved->honored(); } }
#ifdef DEBUG_CURLIO
	void debug(bool debug) { if (mDebug) debug_curl_remove_easy(mEasyHandle); if (debug) debug_curl_add_easy(mEasyHandle); mDebug = debug; }
#endif
  private:
	CURL* mEasyHandle;
	CURLM* mActiveMultiHandle;
	mutable char* mErrorBuffer;
	AIPostFieldPtr mPostField;
	bool mQueuedForRemoval;
	LLPointer<AIPerService::Approvement> mApproved;
#ifdef DEBUG_CURLIO
	bool mDebug;
#endif
#ifdef SHOW_ASSERT
  public:
	bool mRemovedPerCommand;
#endif
  private:
	friend class curlthread::MultiHandle;
	CURLMcode add_handle_to_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi_handle);
	CURLMcode remove_handle_from_multi(AICurlEasyRequest_wat& curl_easy_request_w, CURLM* multi_handle);
  public:
	bool active(void) const { return mActiveMultiHandle; }
	bool no_warning(void) const { return mQueuedForRemoval || LLApp::isExiting(); }
	bool operator==(CURL* easy_handle) const { return mEasyHandle == easy_handle; }
  private:
	void setErrorBuffer(void) const;
	static void handle_easy_error(CURLcode code);
	static inline CURLcode check_easy_code(CURLcode code)
	{
	  AICurlInterface::Stats::easy_calls++;
	  if (code != CURLE_OK)
		handle_easy_error(code);
	  return code;
	}
  protected:
	CURL* getEasyHandle(void) const { return mEasyHandle; }
	void setPostField(AIPostFieldPtr const& post_field_ptr) { mPostField = post_field_ptr; }
  private:
	static char* getTLErrorBuffer(void);
};
class CurlEasyRequest : public CurlEasyHandle {
  private:
	void setPost_raw(U32 size, char const* data, bool keepalive);
  public:
	void setPut(U32 size, bool keepalive = true);
	void setPatch(U32 size, bool keepalive = true);
	void setPost(U32 size, bool keepalive = true) { setPost_raw(size, NULL, keepalive); }
	void setPost(AIPostFieldPtr const& postdata, U32 size, bool keepalive = true);
	void setPost(char const* data, U32 size, bool keepalive = true) { setPost(new AIPostField(data), size, keepalive); }
	void setoptString(CURLoption option, std::string const& value);
	void addHeader(char const* str);
	void addHeaders(AIHTTPHeaders const& headers);
  private:
	static size_t headerCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static size_t readCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
	static CURLcode SSLCtxCallback(CURL* curl, void* sslctx, void* userdata);
	static int progressCallback(void* userdata, double, double, double, double);
	curl_write_callback mHeaderCallback;
	void* mHeaderCallbackUserData;
	curl_write_callback mWriteCallback;
	void* mWriteCallbackUserData;
	curl_read_callback mReadCallback;
	void* mReadCallbackUserData;
	curl_ssl_ctx_callback mSSLCtxCallback;
	void* mSSLCtxCallbackUserData;
	curl_progress_callback mProgressCallback;
	void* mProgressCallbackUserData;
  public:
	void setHeaderCallback(curl_write_callback callback, void* userdata);
	void setWriteCallback(curl_write_callback callback, void* userdata);
	void setReadCallback(curl_read_callback callback, void* userdata);
	void setSSLCtxCallback(curl_ssl_ctx_callback callback, void* userdata);
	void setProgressCallback(curl_progress_callback callback, void* userdata);
	void revokeCallbacks(void);
  protected:
	void resetState(void);
  private:
	void applyProxySettings(void);
	static CURLcode curlCtxCallback(CURL* curl, void* sslctx, void* parm);
	void create_timeout_object(void);
  public:
	void applyDefaultOptions(void);
	void finalizeRequest(std::string const& url, AIHTTPTimeoutPolicy const& policy, AICurlEasyRequestStateMachine* state_machine);
	void set_timeout_opts(void);
  public:
	void storeResult(CURLcode result) { mResult = result; }
	void done(AICurlEasyRequest_wat& curl_easy_request_w, CURLcode result)
	{
	  if (mTimeout)
	  {
		mTimeout->done(curl_easy_request_w, result);
	  }
	  finished(curl_easy_request_w);
	}
	void getTransferInfo(AITransferInfo* info);
	void getResult(CURLcode* result, AITransferInfo* info = NULL);
	void print_curl_timings(void) const;
  protected:
	curl_slist* mHeaders;
	AICurlEasyHandleEvents* mHandleEventsTarget;
	U32 mContentLength;
	CURLcode mResult;
	AIHTTPTimeoutPolicy const* mTimeoutPolicy;
	std::string mLowercaseServicename;
	AIPerServicePtr mPerServicePtr;
	LLPointer<curlthread::HTTPTimeout> mTimeout;
	bool mTimeoutIsOrphan;
	bool mIsHttps;
#ifdef CWDEBUG
  public:
	bool mDebugIsHeadOrGetMethod;
#endif
  public:
	AIHTTPTimeoutPolicy const* getTimeoutPolicy(void) const { return mTimeoutPolicy; }
	std::string const& getLowercaseServicename(void) const { return mLowercaseServicename; }
	std::string getLowercaseHostname(void) const;
	LLPointer<curlthread::HTTPTimeout>& get_timeout_object(void);
	LLPointer<curlthread::HTTPTimeout>& httptimeout(void) { if (!mTimeout) { create_timeout_object(); mTimeoutIsOrphan = true; } return mTimeout; }
	bool has_stalled(void) { return mTimeout && mTimeout->has_stalled(); }
  protected:
	CurlEasyRequest(void) : mHeaders(NULL), mHandleEventsTarget(NULL), mContentLength(0), mResult(CURLE_FAILED_INIT), mTimeoutPolicy(NULL), mTimeoutIsOrphan(false)
#ifdef CWDEBUG
		, mDebugIsHeadOrGetMethod(false)
#endif
		{ applyDefaultOptions(); }
  public:
	~CurlEasyRequest();
  public:
	void send_handle_events_to(AICurlEasyHandleEvents* target) { mHandleEventsTarget = target; }
	bool is_finalized(void) const { return mTimeoutPolicy; }
	inline ThreadSafeBufferedCurlEasyRequest* get_lockobj(void);
	inline ThreadSafeBufferedCurlEasyRequest const* get_lockobj(void) const;
	AIPerServicePtr getPerServicePtr(void);
	bool removeFromPerServiceQueue(AICurlEasyRequest const&, AICapabilityType capability_type) const;
  protected:
	void added_to_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);
	void finished(AICurlEasyRequest_wat& curl_easy_request_w);
	void removed_from_multi_handle(AICurlEasyRequest_wat& curl_easy_request_w);
  public:
	void bad_file_descriptor(AICurlEasyRequest_wat& curl_easy_request_w);
#ifdef SHOW_ASSERT
	void queued_for_removal(AICurlEasyRequest_wat& curl_easy_request_w);
#endif
};
class BufferedCurlEasyRequest : public CurlEasyRequest {
  public:
	typedef boost::shared_ptr<LLBufferArray> buffer_ptr_t;
	void resetState(void);
	void prepRequest(AICurlEasyRequest_wat& buffered_curl_easy_request_w, AIHTTPHeaders const& headers, LLHTTPClient::ResponderPtr responder);
	buffer_ptr_t& getInput(void) { return mInput; }
	buffer_ptr_t& getOutput(void) { return mOutput; }
	void aborted(U32 http_status, std::string const& reason);
	void processOutput(void);
	static void shutdown(void);
    void send_buffer_events_to(AIBufferedCurlEasyRequestEvents* target) { mBufferEventsTarget = target; }
	void update_body_bandwidth(void);
  protected:
	void received_HTTP_header(void);
	void received_header(std::string const& key, std::string const& value);
	void completed_headers(U32 status, std::string const& reason, AITransferInfo* info);
  private:
	buffer_ptr_t mInput;
	U8* mLastRead;
	buffer_ptr_t mOutput;
	LLHTTPClient::ResponderPtr mResponder;
	AICapabilityType mCapabilityType;
	bool mIsEventPoll;
	U32 mStatus;
	std::string mReason;
	U32 mRequestTransferedBytes;
	size_t mTotalRawBytes;
	AIBufferedCurlEasyRequestEvents* mBufferEventsTarget;
  public:
	static LLChannelDescriptors const sChannels;
	static LLGlobalMutex sResponderCallbackMutex;
	static bool sShuttingDown;
	static AIAverage sHTTPBandwidth;
  private:
	friend class ThreadSafeBufferedCurlEasyRequest;
	BufferedCurlEasyRequest(void);
  public:
	~BufferedCurlEasyRequest();
  private:
    static size_t curlWriteCallback(char* data, size_t size, size_t nmemb, void* user_data);
    static size_t curlReadCallback(char* data, size_t size, size_t nmemb, void* user_data);
    static size_t curlHeaderCallback(char* data, size_t size, size_t nmemb, void* user_data);
	static int curlProgressCallback(void* user_data, double dltotal, double dlnow, double ultotal, double ulnow);
	void setStatusAndReason(U32 status, std::string const& reason);
	void print_diagnostics(CURLcode code);
  public:
	ThreadSafeBufferedCurlEasyRequest* get_lockobj(void);
	ThreadSafeBufferedCurlEasyRequest const* get_lockobj(void) const;
	bool upload_error_status(void) const { return mStatus == HTTP_BAD_REQUEST; }
	bool success(void) const { return mResult == CURLE_OK && mStatus >= 200 && mStatus < 400; }
	bool isValid(void) const { return !!mResponder; }
	AICapabilityType capability_type(void) const { llassert(mCapabilityType != number_of_capability_types); return mCapabilityType; }
	bool is_event_poll(void) const { return mIsEventPoll; }
	bool received_data(void) const { return mTotalRawBytes > 0; }
#ifdef CWDEBUG
	void connection_established(int connectionnr);
	void connection_closed(int connectionnr);
#endif
};
inline ThreadSafeBufferedCurlEasyRequest* CurlEasyRequest::get_lockobj(void)
{
  return static_cast<BufferedCurlEasyRequest*>(this)->get_lockobj();
}
inline ThreadSafeBufferedCurlEasyRequest const* CurlEasyRequest::get_lockobj(void) const
{
  return static_cast<BufferedCurlEasyRequest const*>(this)->get_lockobj();
}
class ThreadSafeBufferedCurlEasyRequest : public AIThreadSafeSimple<BufferedCurlEasyRequest> {
  public:
	ThreadSafeBufferedCurlEasyRequest(void) : mReferenceCount(0)
        { new (ptr()) BufferedCurlEasyRequest;
		  Dout(dc::curl, "Creating ThreadSafeBufferedCurlEasyRequest with this = " << (void*)this);
		  AICurlInterface::Stats::ThreadSafeBufferedCurlEasyRequest_count++; }
	~ThreadSafeBufferedCurlEasyRequest()
	    { Dout(dc::curl, "Destructing ThreadSafeBufferedCurlEasyRequest with this = " << (void*)this);
		  --AICurlInterface::Stats::ThreadSafeBufferedCurlEasyRequest_count; }
  private:
	LLAtomicU32 mReferenceCount;
	friend void intrusive_ptr_add_ref(ThreadSafeBufferedCurlEasyRequest* p);
	friend void intrusive_ptr_release(ThreadSafeBufferedCurlEasyRequest* p);
};
typedef boost::intrusive_ptr<ThreadSafeBufferedCurlEasyRequest> BufferedCurlEasyRequestPtr;
class CurlMultiHandle : public boost::noncopyable {
  public:
	CurlMultiHandle(void);
	~CurlMultiHandle();
  private:
	CurlMultiHandle& operator=(CurlMultiHandle const*);
  private:
	static LLAtomicU32 sTotalMultiHandles;
  protected:
	CURLM* mMultiHandle;
  public:
	CURLMcode setopt(CURLMoption option, long parameter);
	CURLMcode setopt(CURLMoption option, curl_socket_callback parameter);
	CURLMcode setopt(CURLMoption option, curl_multi_timer_callback parameter);
	CURLMcode setopt(CURLMoption option, void* parameter);
	static U32 getTotalMultiHandles(void) { return sTotalMultiHandles; }
};
inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, long parameter)
{
  llassert(option == CURLMOPT_MAXCONNECTS || option == CURLMOPT_PIPELINING);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}
inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, curl_socket_callback parameter)
{
  llassert(option == CURLMOPT_SOCKETFUNCTION);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}
inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, curl_multi_timer_callback parameter)
{
  llassert(option == CURLMOPT_TIMERFUNCTION);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}
inline CURLMcode CurlMultiHandle::setopt(CURLMoption option, void* parameter)
{
  llassert(option == CURLMOPT_SOCKETDATA || option == CURLMOPT_TIMERDATA);
  return check_multi_code(curl_multi_setopt(mMultiHandle, option, parameter));
}
}
#endif
