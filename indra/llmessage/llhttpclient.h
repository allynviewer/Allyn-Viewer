/** 
 * @file llhttpclient.h
 * @brief Declaration of classes for making HTTP client requests.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#ifndef LL_LLHTTPCLIENT_H
#define LL_LLHTTPCLIENT_H
#include <string>
#include <curl/curl.h>
#include <boost/intrusive_ptr.hpp>
#include "llassettype.h"
#include "llhttpstatuscodes.h"
#include "aihttpheaders.h"
#include "aicurlperservice.h"
class LLUUID;
class LLPumpIO;
class LLSD;
class AIHTTPTimeoutPolicy;
class LLBufferArray;
class LLChannelDescriptors;
class AIStateMachine;
class Injector;
class AIEngine;
extern AIHTTPTimeoutPolicy responderIgnore_timeout;
typedef struct _xmlrpc_request* XMLRPC_REQUEST;
typedef struct _xmlrpc_value* XMLRPC_VALUE;
extern AIEngine gMainThreadEngine;
static const S32 HTTP_REQUESTS_RANGE_END_MAX = 20000000;
struct AITransferInfo {
  AITransferInfo() : mSizeDownload(0.0), mTotalTime(0.0), mSpeedDownload(0.0) { }
  F64 mSizeDownload;
  F64 mTotalTime;
  F64 mSpeedDownload;
};
struct AIBufferedCurlEasyRequestEvents {
	virtual void received_HTTP_header(void) = 0;
	virtual void received_header(std::string const& key, std::string const& value) = 0;
	virtual void completed_headers(U32 status, std::string const& reason, AITransferInfo* info) = 0;
};
enum EKeepAlive {
  no_keep_alive = 0,
  keep_alive
};
enum EDoesAuthentication {
  no_does_authentication = 0,
  does_authentication
};
enum EAllowCompressedReply {
  no_allow_compressed_reply = 0,
  allow_compressed_reply
};
#ifdef DEBUG_CURLIO
enum EDebugCurl {
  debug_off = 0,
  debug_on
};
#define DEBUG_CURLIO_PARAM(p) ,p
#else
#define DEBUG_CURLIO_PARAM(p)
#endif
class LLHTTPClient {
public:
	enum ERequestAction
	{
		INVALID,
		HTTP_HEAD,
		HTTP_GET,
		HTTP_PUT,
		HTTP_POST,
		HTTP_DELETE,
		HTTP_MOVE,
		HTTP_PATCH,
		HTTP_COPY,
		REQUEST_ACTION_COUNT
	};
	class ResponderBase : public AIBufferedCurlEasyRequestEvents {
	public:
		typedef boost::shared_ptr<LLBufferArray> buffer_ptr_t;
		static bool isGoodStatus(S32 status)
		{
			return((200 <= status) && (status < 300));
		}
	protected:
		ResponderBase(void);
		virtual ~ResponderBase();
		void decode_llsd_body(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer);
		void decode_raw_body(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer, std::string& content);
	protected:
		std::string mURL;
		AIHTTPReceivedHeaders mReceivedHeaders;
		CURLcode mCode;
		S32 mStatus;
		std::string mReason;
		LLSD mContent;
		bool mFinished;
	public:
		void setURL(std::string const& url);
		std::string const& getURL(void) const { return mURL; }
		CURLcode result_code(void) const { return mCode; }
	protected:
		void setResult(S32 status, std::string const& reason, LLSD const& content) { mStatus = status; mReason = reason; mContent = content; mFinished = true; }
		LLSD const& getContent(void) const { return mContent; }
		AIHTTPReceivedHeaders const& getResponseHeaders(void) const
		{
			llassert(needsHeaders());
			return mReceivedHeaders;
		}
		std::string dumpResponse(void) const;
	public:
		S32 getStatus(void) const { return mStatus; }
		std::string const& getReason(void) const { return mReason; }
	public:
		virtual void finished(CURLcode code, U32 http_status, std::string const& reason, LLChannelDescriptors const& channels, buffer_ptr_t const& buffer) = 0;
		bool is_finished(void) const { return mFinished; }
	protected:
		void received_HTTP_header(void)
		{
			AIHTTPReceivedHeaders set_cookie_headers;
			AIHTTPReceivedHeaders::range_type cookies;
			if (mReceivedHeaders.getValues("set-cookie", cookies))
			{
				for (AIHTTPReceivedHeaders::iterator_type cookie = cookies.first; cookie != cookies.second; ++cookie)
				{
					set_cookie_headers.addHeader(cookie->first, cookie->second);
				}
			}
			mReceivedHeaders.swap(set_cookie_headers);
		}
		void received_header(std::string const& key, std::string const& value)
		{
			mReceivedHeaders.addHeader(key, value);
		}
		void completed_headers(U32 status, std::string const& reason, AITransferInfo* info)
		{
			mStatus = status;
			mReason = reason;
			completedHeaders();
		}
		std::string const& get_cookie(std::string const& key);
	public:
		virtual bool needsHeaders(void) const { return false; }
		virtual bool forbidReuse(void) const { return false; }
		virtual bool pass_redirect_status(void) const { return false; }
		virtual bool redirect_status_ok(void) const { return true; }
		virtual bool is_event_poll(void) const { return false; }
		virtual AICapabilityType capability_type(void) const { return cap_other; }
		virtual AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const;
		virtual char const* getName(void) const = 0;
	protected:
		virtual void completedHeaders(void)
		{
		}
	private:
		LLAtomicU32 mReferenceCount;
		friend void intrusive_ptr_add_ref(ResponderBase* p);
		friend void intrusive_ptr_release(ResponderBase* p);
	};
	class ResponderHeadersOnly : public ResponderBase {
	private:
		bool needsHeaders(void) const { return true; }
	protected:
		void finished(CURLcode code, U32 http_status, std::string const& reason, LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
		{
			mCode = code;
			mStatus = http_status;
			mReason = reason;
			completedHeaders();
			mFinished = true;
		}
	protected:
#ifdef SHOW_ASSERT
		enum YOU_MAY_ONLY_OVERRIDE_COMPLETED_HEADERS { };
		virtual void completedRaw(YOU_MAY_ONLY_OVERRIDE_COMPLETED_HEADERS) { }
		virtual void httpCompleted(YOU_MAY_ONLY_OVERRIDE_COMPLETED_HEADERS) { }
		virtual void httpSuccess(YOU_MAY_ONLY_OVERRIDE_COMPLETED_HEADERS) { }
		virtual void httpFailure(YOU_MAY_ONLY_OVERRIDE_COMPLETED_HEADERS) { }
#endif
	};
	class ResponderWithCompleted : public ResponderBase {
	protected:
		void finished(CURLcode code, U32 http_status, std::string const& reason, LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
		{
			mCode = code;
			mStatus = http_status;
			mReason = reason;
			completedRaw(channels, buffer);
			mFinished = true;
		}
	protected:
		virtual void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer);
		virtual void httpCompleted(void);
    public:
		void completeResult(S32 status, std::string const& reason, LLSD const& content) { mCode = CURLE_OK; setResult(status, reason, content); httpCompleted(); }
#ifdef SHOW_ASSERT
		enum YOU_ARE_DERIVING_FROM_THE_WRONG_CLASS { };
		virtual void httpSuccess(YOU_ARE_DERIVING_FROM_THE_WRONG_CLASS) { }
		virtual void httpFailure(YOU_ARE_DERIVING_FROM_THE_WRONG_CLASS) { }
#endif
	};
	class ResponderWithResult : public ResponderBase {
	protected:
		void finished(CURLcode code, U32 http_status, std::string const& reason, LLChannelDescriptors const& channels, buffer_ptr_t const& buffer);
	protected:
		virtual void httpSuccess(void) = 0;
		virtual void httpFailure(void);
	public:
		void failureResult(U32 status, std::string const& reason, LLSD const& content, CURLcode code = CURLE_OK) { mCode = code; setResult(status, reason, content); httpFailure(); }
		void successResult(LLSD const& content) { mCode = CURLE_OK; setResult(HTTP_OK, "", content); httpSuccess(); }
#ifdef SHOW_ASSERT
		enum YOU_ARE_DERIVING_FROM_THE_WRONG_CLASS { };
		virtual void completedRaw(YOU_ARE_DERIVING_FROM_THE_WRONG_CLASS) { }
		virtual void httpCompleted(YOU_ARE_DERIVING_FROM_THE_WRONG_CLASS) { }
#endif
	};
	class ResponderIgnoreBody : public ResponderWithResult {
		void httpSuccess(void) { }
	};
	class ResponderIgnore : public ResponderIgnoreBody {
		AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return responderIgnore_timeout;}
		char const* getName(void) const { return "ResponderIgnore"; }
	};
	typedef boost::intrusive_ptr<ResponderBase> ResponderPtr;
	static void request(
		std::string const& url,
		ERequestAction method,
		Injector* body_injector,
		ResponderPtr responder,
		AIHTTPHeaders& headers,
		AIPerService::Approvement* approved
		DEBUG_CURLIO_PARAM(EDebugCurl debug),
		EKeepAlive keepalive = keep_alive,
		EDoesAuthentication does_auth = no_does_authentication,
		EAllowCompressedReply allow_compression = allow_compressed_reply,
		AIStateMachine* parent = NULL,
		U32 new_parent_state = 0,
		AIEngine* default_engine = &gMainThreadEngine);
	static void head(std::string const& url, ResponderHeadersOnly* responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void head(std::string const& url, ResponderHeadersOnly* responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; head(url, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static bool getByteRange(std::string const& url, AIHTTPHeaders& headers, S32 offset, S32 bytes, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static bool getByteRange(std::string const& url, S32 offset, S32 bytes, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
		{ AIHTTPHeaders headers; return getByteRange(url, headers, offset, bytes, responder DEBUG_CURLIO_PARAM(debug)); }
	static void get(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void get(std::string const& url, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; get(url, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static void get(std::string const& url, LLSD const& query, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void get(std::string const& url, LLSD const& query, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; get(url, query, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static void put(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void put(std::string const& url, LLSD const& body, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; put(url, body, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static void putRaw(const std::string& url, const U8* data, S32 size, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void putRaw(const std::string& url, const U8* data, S32 size, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; putRaw(url, data, size, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static void getHeaderOnly(std::string const& url, ResponderHeadersOnly* responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void getHeaderOnly(std::string const& url, ResponderHeadersOnly* responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; getHeaderOnly(url, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static void patch(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive, AIStateMachine* parent = NULL, U32 new_parent_state = 0);
	static void patch(std::string const& url, LLSD const& body, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive, AIStateMachine* parent = NULL, U32 new_parent_state = 0)
		{ AIHTTPHeaders headers; patch(url, body, responder, headers DEBUG_CURLIO_PARAM(debug), keepalive, parent, new_parent_state); }
	static void post(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive, AIStateMachine* parent = NULL, U32 new_parent_state = 0);
	static void post(std::string const& url, LLSD const& body, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive, AIStateMachine* parent = NULL, U32 new_parent_state = 0)
	    { AIHTTPHeaders headers; post(url, body, responder, headers DEBUG_CURLIO_PARAM(debug), keepalive, parent, new_parent_state); }
	static void post_approved(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers, AIPerService::Approvement* approved DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive, AIStateMachine* parent = NULL, U32 new_parent_state = 0);
	static void post_approved(std::string const& url, LLSD const& body, ResponderPtr responder, AIPerService::Approvement* approved DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive, AIStateMachine* parent = NULL, U32 new_parent_state = 0)
	    { AIHTTPHeaders headers; post_approved(url, body, responder, headers, approved DEBUG_CURLIO_PARAM(debug), keepalive, parent, new_parent_state); }
	static void postXMLRPC(std::string const& url, XMLRPC_REQUEST request, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive);
	static void postXMLRPC(std::string const& url, XMLRPC_REQUEST request, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive)
	    { AIHTTPHeaders headers; postXMLRPC(url, request, responder, headers DEBUG_CURLIO_PARAM(debug), keepalive); }
	static void postXMLRPC(std::string const& url, char const* method, XMLRPC_VALUE value, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive);
	static void postXMLRPC(std::string const& url, char const* method, XMLRPC_VALUE value, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive)
	    { AIHTTPHeaders headers; postXMLRPC(url, method, value, responder, headers DEBUG_CURLIO_PARAM(debug), keepalive); }
	static void postRaw(std::string const& url, const U8* data, S32 size, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive);
	static void postRaw(std::string const& url, const U8* data, S32 size, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive)
	    { AIHTTPHeaders headers; postRaw(url, data, size, responder, headers DEBUG_CURLIO_PARAM(debug), keepalive); }
	static void postFile(std::string const& url, std::string const& filename, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive);
	static void postFile(std::string const& url, std::string const& filename, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive)
	    { AIHTTPHeaders headers; postFile(url, filename, responder, headers DEBUG_CURLIO_PARAM(debug), keepalive); }
	static void postFile(std::string const& url, const LLUUID& uuid, LLAssetType::EType asset_type, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive);
	static void postFile(std::string const& url, const LLUUID& uuid, LLAssetType::EType asset_type, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off), EKeepAlive keepalive = keep_alive)
	    { AIHTTPHeaders headers; postFile(url, uuid, asset_type, responder, headers DEBUG_CURLIO_PARAM(debug), keepalive); }
	static void del(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void del(std::string const& url, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; del(url, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static void move(std::string const& url, std::string const& destination, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void move(std::string const& url, std::string const& destination, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	    { AIHTTPHeaders headers; move(url, destination, responder, headers DEBUG_CURLIO_PARAM(debug)); }
	static void copy(std::string const& url, std::string const& destination, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static void copy(std::string const& url, std::string const& destination, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off))
	{
		AIHTTPHeaders headers; copy(url, destination, responder, headers DEBUG_CURLIO_PARAM(debug));
	}
	static LLSD blockingGet(std::string const& url DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static U32 blockingGetRaw(const std::string& url, std::string& result DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
	static LLSD blockingPost(std::string const& url, LLSD const& body DEBUG_CURLIO_PARAM(EDebugCurl debug = debug_off));
};
#endif
