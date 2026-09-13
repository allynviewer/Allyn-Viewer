/** 
 * @file llhttpclient.cpp
 * @brief Implementation of classes for making HTTP requests.
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
#include "linden_common.h"
#include <boost/shared_ptr.hpp>
#include <xmlrpc-epi/xmlrpc.h>
#include "llhttpclient.h"
#include "llbufferstream.h"
#include "llsdserialize.h"
#include "llvfile.h"
#include "llurlrequest.h"
#include "llxmltree.h"
#include "aihttptimeoutpolicy.h"
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy blockingLLSDPost_timeout;
extern AIHTTPTimeoutPolicy blockingLLSDGet_timeout;
extern AIHTTPTimeoutPolicy blockingRawGet_timeout;
class XMLRPCInjector : public Injector {
public:
	XMLRPCInjector(XMLRPC_REQUEST request) : mRequest(request), mRequestText(NULL) { }
	~XMLRPCInjector() { XMLRPC_RequestFree(mRequest, 1); XMLRPC_Free(const_cast<char*>(mRequestText)); }
	char const* contentType(void) const { return "text/xml"; }
	U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		int requestTextSize;
		mRequestText = XMLRPC_REQUEST_ToXML(mRequest, &requestTextSize);
		if (!mRequestText)
			throw AICurlNoBody("XMLRPC_REQUEST_ToXML returned NULL.");
		LLBufferStream ostream(channels, buffer.get());
		ostream.write(mRequestText, requestTextSize);
		ostream << std::flush;
		return requestTextSize;
	}
private:
	XMLRPC_REQUEST const mRequest;
	char const* mRequestText;
};
class XmlTreeInjector : public Injector
{
	public:
		XmlTreeInjector(LLXmlTree const* tree) : mTree(tree) { }
		~XmlTreeInjector() { delete const_cast<LLXmlTree*>(mTree); }
		char const* contentType(void) const { return "application/xml"; }
		U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
		{
			std::string data;
			mTree->write(data);
			LLBufferStream ostream(channels, buffer.get());
			ostream.write(data.data(), data.size());
			ostream << std::flush;
			return data.size();
		}
	private:
		LLXmlTree const* mTree;
};
class LLSDInjector : public Injector
{
  public:
	LLSDInjector(LLSD const& sd) : mSD(sd) { }
	char const* contentType(void) const { return "application/llsd+xml"; }
	U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		LLBufferStream ostream(channels, buffer.get());
		LLSDSerialize::toXML(mSD, ostream);
		ostream << std::flush;
		return ostream.count_out();
	}
	LLSD const mSD;
};
class RawInjector : public Injector
{
  public:
	RawInjector(U8 const* data, U32 size) : mData(data), mSize(size) { }
	~RawInjector() { delete [] mData; }
	char const* contentType(void) const { return "application/octet-stream"; }
	U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		LLBufferStream ostream(channels, buffer.get());
		ostream.write((const char*)mData, mSize);
		ostream << std::flush;
		return mSize;
	}
	U8 const* mData;
	U32 mSize;
};
class FileInjector : public Injector
{
  public:
	FileInjector(std::string const& filename) : mFilename(filename) { }
	char const* contentType(void) const { return "application/octet-stream"; }
	U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		llifstream fstream(mFilename, std::ios::binary);
		if (!fstream.is_open())
		  throw AICurlNoBody(llformat("Failed to open \"%s\".", mFilename.c_str()));
		LLBufferStream ostream(channels, buffer.get());
		char tmpbuf[4096];
#ifdef SHOW_ASSERT
		size_t total_len = 0;
		fstream.seekg(0, std::ios::end);
		size_t file_size = fstream.tellg();
		fstream.seekg(0, std::ios::beg);
#endif
		while (fstream)
		{
			fstream.read(tmpbuf, sizeof(tmpbuf));
			std::streamsize len = fstream.gcount();
			if (len > 0)
			{
				ostream.write(tmpbuf, len);
#ifdef SHOW_ASSERT
				total_len += len;
#endif
			}
		}
		if (fstream.bad())
		  throw AICurlNoBody(llformat("An error occured while reading \"%s\".", mFilename.c_str()));
		fstream.close();
		ostream << std::flush;
		llassert(total_len == file_size && total_len == ostream.count_out());
		return ostream.count_out();
	}
	std::string const mFilename;
};
class VFileInjector : public Injector
{
public:
	VFileInjector(LLUUID const& uuid, LLAssetType::EType asset_type) : mUUID(uuid), mAssetType(asset_type) { }
	char const* contentType(void) const { return "application/octet-stream"; }
	U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer)
	{
		LLBufferStream ostream(channels, buffer.get());
		LLVFile vfile(gVFS, mUUID, mAssetType, LLVFile::READ);
		S32 fileSize = vfile.getSize();
		std::vector<U8> fileBuffer(fileSize);
		vfile.read(&fileBuffer[0], fileSize);
		ostream.write((char*)&fileBuffer[0], fileSize);
		ostream << std::flush;
		return fileSize;
	}
	LLUUID const mUUID;
	LLAssetType::EType mAssetType;
};
void LLHTTPClient::request(
	std::string const& url,
	LLURLRequest::ERequestAction method,
	Injector* body_injector,
	LLHTTPClient::ResponderPtr responder,
	AIHTTPHeaders& headers,
	AIPerService::Approvement* approved
	DEBUG_CURLIO_PARAM(EDebugCurl debug),
	EKeepAlive keepalive,
	EDoesAuthentication does_auth,
	EAllowCompressedReply allow_compression,
	AIStateMachine* parent,
	AIStateMachine::state_type new_parent_state,
	AIEngine* default_engine)
{
	llassert(responder);
	if (!responder) {
		responder = new LLHTTPClient::ResponderIgnore;
	}
	responder->setURL(url);
	LLURLRequest* req;
	try
	{
		req = new LLURLRequest(method, url, body_injector, responder, headers, approved, keepalive, does_auth, allow_compression);
#ifdef DEBUG_CURLIO
		req->mCurlEasyRequest.debug(debug);
#endif
	}
	catch(AICurlNoEasyHandle& error)
	{
		LL_WARNS() << "Failed to create LLURLRequest: " << error.what() << LL_ENDL;
		return ;
	}
	req->run(parent, new_parent_state, parent != NULL, true, default_engine);
}
bool LLHTTPClient::getByteRange(std::string const& url, AIHTTPHeaders& headers, S32 offset, S32 bytes, ResponderPtr responder DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	try
	{
		if (offset > 0 || bytes > 0)
		{
			int const range_end = offset + bytes - 1;
			char const* const range_format = (range_end >= HTTP_REQUESTS_RANGE_END_MAX) ? "bytes=%d-" : "bytes=%d-%d";
			headers.addHeader("Range", llformat(range_format, offset, range_end));
		}
		request(url, HTTP_GET, NULL, responder, headers, NULL DEBUG_CURLIO_PARAM(debug));
	}
	catch(AICurlNoEasyHandle const&)
	{
		return false;
	}
	return true;
}
void LLHTTPClient::head(std::string const& url, ResponderHeadersOnly* responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	request(url, HTTP_HEAD, NULL, responder, headers, NULL DEBUG_CURLIO_PARAM(debug));
}
void LLHTTPClient::get(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	request(url, HTTP_GET, NULL, responder, headers, NULL DEBUG_CURLIO_PARAM(debug));
}
void LLHTTPClient::getHeaderOnly(std::string const& url, ResponderHeadersOnly* responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	request(url, HTTP_HEAD, NULL, responder, headers, NULL DEBUG_CURLIO_PARAM(debug));
}
void LLHTTPClient::get(std::string const& url, LLSD const& query, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	LLURI uri;
	uri = LLURI::buildHTTP(url, LLSD::emptyArray(), query);
	get(uri.asString(), responder, headers DEBUG_CURLIO_PARAM(debug));
}
LLHTTPClient::ResponderBase::ResponderBase(void) : mReferenceCount(0), mCode(CURLE_FAILED_INIT), mFinished(false)
{
	DoutEntering(dc::curl, "AICurlInterface::Responder() with this = " << (void*)this);
	AICurlInterface::Stats::ResponderBase_count++;
}
LLHTTPClient::ResponderBase::~ResponderBase()
{
	DoutEntering(dc::curl, "AICurlInterface::ResponderBase::~ResponderBase() with this = " << (void*)this << "; mReferenceCount = " << mReferenceCount);
	llassert(mReferenceCount == 0);
	--AICurlInterface::Stats::ResponderBase_count;
}
void LLHTTPClient::ResponderBase::setURL(std::string const& url)
{
	mURL = url;
}
AIHTTPTimeoutPolicy const& LLHTTPClient::ResponderBase::getHTTPTimeoutPolicy(void) const
{
	return AIHTTPTimeoutPolicy::getDebugSettingsCurlTimeout();
}
void LLHTTPClient::ResponderBase::decode_llsd_body(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
{
	AICurlInterface::Stats::llsd_body_count++;
	if (is_internal_http_error(mStatus))
	{
	    mContent = mReason;
		return;
	}
	bool const should_be_llsd = isGoodStatus(mStatus);
	if (should_be_llsd)
	{
		LLBufferStream istr(channels, buffer.get());
		if (LLSDSerialize::fromXML(mContent, istr) == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS() << "Failed to deserialize LLSD. " << mURL << " [" << mStatus << "]: " << mReason << LL_ENDL;
			AICurlInterface::Stats::llsd_body_parse_error++;
		}
		return;
	}
	std::stringstream ss;
	buffer->writeChannelTo(ss, channels.in());
	mContent = ss.str();
#ifdef SHOW_ASSERT
	if (!should_be_llsd)
	{
		char const* str = ss.str().c_str();
		LLSD dummy;
		bool server_sent_llsd_with_http_error =
			strncmp(str, "<!DOCTYPE", 9) &&
			strncmp(str, "cap not found:", 14) &&
			str[0] &&
			strncmp(str, "Not Found", 9) &&
			strncmp(str, "Upstream error: ", 16) &&
			LLSDSerialize::fromXML(dummy, ss) > 0;
		if (server_sent_llsd_with_http_error)
		{
			LL_WARNS() << "The server sent us a response with http status " << mStatus << " and LLSD(!) body: \"" << ss.str() << "\"!" << LL_ENDL;
		}
		llassert(!server_sent_llsd_with_http_error);
	}
#endif
}
void LLHTTPClient::ResponderBase::decode_raw_body(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer, std::string& content)
{
	AICurlInterface::Stats::raw_body_count++;
	if (is_internal_http_error(mStatus))
	{
	    content = mReason;
		return;
	}
	LLMutexLock lock(buffer->getMutex());
	LLBufferArray::const_segment_iterator_t const end = buffer->endSegment();
	for (LLBufferArray::const_segment_iterator_t iter = buffer->beginSegment(); iter != end; ++iter)
	{
		if (iter->isOnChannel(channels.in()))
		{
			content.append((char*)iter->data(), iter->size());
		}
	}
}
std::string const& LLHTTPClient::ResponderBase::get_cookie(std::string const& key)
{
	AIHTTPReceivedHeaders::range_type cookies;
	if (mReceivedHeaders.getValues("set-cookie", cookies))
	{
		for (AIHTTPReceivedHeaders::iterator_type cookie = cookies.first; cookie != cookies.second; ++cookie)
		{
			if (key == cookie->second.substr(0, cookie->second.find('=')))
			{
				return cookie->second;
			}
		}
	}
	static std::string empty_dummy;
	return empty_dummy;
}
std::string LLHTTPClient::ResponderBase::dumpResponse(void) const
{
	std::ostringstream s;
	s << "[responder:" << getName() << "] "
	  << "[URL:" << mURL << "] "
	  << "[status:" << mStatus << "] "
	  << "[reason:" << mReason << "] ";
	AIHTTPReceivedHeaders::range_type content_type;
	if (mReceivedHeaders.getValues("content-type", content_type))
	{
		for (AIHTTPReceivedHeaders::iterator_type iter = content_type.first; iter != content_type.second; ++iter)
		{
			s << "[content-type:" << iter->second << "] ";
		}
	}
	if (mContent.isDefined())
	{
		s << "[content:" << LLSDOStreamer<LLSDXMLFormatter>(mContent, LLSDFormatter::OPTIONS_PRETTY) << "]";
	}
	return s.str();
}
void LLHTTPClient::ResponderWithCompleted::completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
{
  decode_llsd_body(channels, buffer);
  httpCompleted();
}
void LLHTTPClient::ResponderWithCompleted::httpCompleted(void)
{
  llassert_always(false);
}
void LLHTTPClient::ResponderWithResult::finished(CURLcode code, U32 http_status, std::string const& reason, LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
{
  mCode = code;
  mStatus = http_status;
  mReason = reason;
  decode_llsd_body(channels, buffer);
  if (isGoodStatus(http_status))
  {
	httpSuccess();
  }
  else
  {
	httpFailure();
  }
  mFinished = true;
}
void LLHTTPClient::ResponderWithResult::httpFailure(void)
{
  LL_INFOS() << mURL << " [" << mStatus << "]: " << mReason << LL_ENDL;
}
void intrusive_ptr_add_ref(LLHTTPClient::ResponderBase* responder)
{
  responder->mReferenceCount++;
}
void intrusive_ptr_release(LLHTTPClient::ResponderBase* responder)
{
  if (--responder->mReferenceCount == 0)
  {
	delete responder;
  }
}
class BlockingResponder : public LLHTTPClient::ResponderWithCompleted {
private:
	LLCondition mSignal;
	static std::string Raw_dummy;
public:
	void wait(void);
	virtual std::string const& getRaw(void) const { llassert(false); return Raw_dummy; }
	LLSD const& getContent(void) const { llassert(false); return LLHTTPClient::ResponderWithCompleted::getContent(); }
protected:
	void wakeup(void);
};
std::string BlockingResponder::Raw_dummy;
void BlockingResponder::wait(void)
{
  if (AIThreadID::in_main_thread())
  {
	while (!mFinished)
	{
	  gMainThreadEngine.mainloop();
	  ms_sleep(10);
	}
  }
  else
  {
	mSignal.lock();
	while (!mFinished)
	  mSignal.wait();
	mSignal.unlock();
  }
}
void BlockingResponder::wakeup(void)
{
  mSignal.lock();
  mFinished = true;
  mSignal.unlock();
  mSignal.signal();
}
class BlockingLLSDResponder : public BlockingResponder {
protected:
	LLSD const& getContent(void) const { llassert(mFinished && mCode == CURLE_OK); return LLHTTPClient::ResponderWithCompleted::getContent(); }
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
	{
		decode_llsd_body(channels, buffer);
		wakeup();
	}
};
class BlockingRawResponder : public BlockingResponder {
private:
	std::string mResponse;
protected:
	std::string const& getRaw(void) const { llassert(mFinished && mCode == CURLE_OK); return mResponse; }
	void completedRaw(LLChannelDescriptors const& channels, buffer_ptr_t const& buffer)
	{
		decode_raw_body(channels, buffer, mResponse);
		wakeup();
	}
};
class BlockingLLSDPostResponder : public BlockingLLSDResponder {
public:
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return blockingLLSDPost_timeout; }
	char const* getName(void) const { return "BlockingLLSDPostResponder"; }
};
class BlockingLLSDGetResponder : public BlockingLLSDResponder {
public:
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return blockingLLSDGet_timeout; }
	char const* getName(void) const { return "BlockingLLSDGetResponder"; }
};
class BlockingRawGetResponder : public BlockingRawResponder {
public:
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return blockingRawGet_timeout; }
	char const* getName(void) const { return "BlockingRawGetResponder"; }
};
enum EBlockingRequestAction {
  HTTP_LLSD_POST,
  HTTP_LLSD_GET,
  HTTP_RAW_GET
};
static LLSD blocking_request(
	std::string const& url,
	EBlockingRequestAction method,
	LLSD const& body
	DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	LL_DEBUGS() << "blockingRequest of " << url << LL_ENDL;
	AIHTTPHeaders headers;
	boost::intrusive_ptr<BlockingResponder> responder;
	if (method == HTTP_LLSD_POST)
	{
		responder = new BlockingLLSDPostResponder;
		LLHTTPClient::post(url, body, responder, headers DEBUG_CURLIO_PARAM(debug));
	}
	else if (method == HTTP_LLSD_GET)
	{
		responder = new BlockingLLSDGetResponder;
		LLHTTPClient::get(url, responder, headers DEBUG_CURLIO_PARAM(debug));
	}
	else
	{
		responder = new BlockingRawGetResponder;
		LLHTTPClient::get(url, responder, headers DEBUG_CURLIO_PARAM(debug));
	}
	responder->wait();
	LLSD response = LLSD::emptyMap();
	CURLcode result = responder->result_code();
	S32 http_status = responder->getStatus();
	bool http_success = http_status >= 200 && http_status < 300;
	if (result == CURLE_OK && http_success)
	{
		if (method == HTTP_RAW_GET)
		{
			response["body"] = responder->getRaw();
		}
		else
		{
			response["body"] = responder->getContent();
		}
	}
	else if (result == CURLE_OK && !is_internal_http_error(http_status))
	{
		if (http_status != 404)
		{
			LL_WARNS() << "CURL REQ URL: " << url << LL_ENDL;
			LL_WARNS() << "CURL REQ METHOD TYPE: " << method << LL_ENDL;
			LL_WARNS() << "CURL REQ HEADERS: " << headers << LL_ENDL;
			if (method == HTTP_LLSD_POST)
			{
				LL_WARNS() << "CURL REQ BODY: " << body.asString() << LL_ENDL;
			}
			LL_WARNS() << "CURL HTTP_STATUS: " << http_status << LL_ENDL;
			if (method == HTTP_RAW_GET)
			{
				LL_WARNS() << "CURL ERROR BODY: " << responder->getRaw() << LL_ENDL;
			}
			else
			{
				LL_WARNS() << "CURL ERROR BODY: " << responder->getContent().asString() << LL_ENDL;
			}
		}
		if (method == HTTP_RAW_GET)
		{
			response["body"] = responder->getRaw();
		}
		else
		{
			response["body"] = responder->getContent().asString();
		}
	}
	else
	{
		response["body"] = responder->getReason();
	}
	response["status"] = http_status;
	return response;
}
LLSD LLHTTPClient::blockingPost(const std::string& url, const LLSD& body DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	return blocking_request(url, HTTP_LLSD_POST, body DEBUG_CURLIO_PARAM(debug));
}
LLSD LLHTTPClient::blockingGet(const std::string& url DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	return blocking_request(url, HTTP_LLSD_GET, LLSD() DEBUG_CURLIO_PARAM(debug));
}
U32 LLHTTPClient::blockingGetRaw(const std::string& url, std::string& body DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	LLSD result = blocking_request(url, HTTP_RAW_GET, LLSD() DEBUG_CURLIO_PARAM(debug));
	body = result["body"].asString();
	return result["status"].asInteger();
}
void LLHTTPClient::put(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	request(url, HTTP_PUT, new LLSDInjector(body), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), no_keep_alive, no_does_authentication, no_allow_compressed_reply);
}
void LLHTTPClient::putRaw(const std::string& url, const U8* data, S32 size, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	request(url, HTTP_PUT, new RawInjector(data, size), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), no_keep_alive, no_does_authentication, no_allow_compressed_reply);
}
void LLHTTPClient::patch(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive, AIStateMachine* parent, AIStateMachine::state_type new_parent_state)
{
	request(url, HTTP_PATCH, new LLSDInjector(body), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), keepalive, no_does_authentication, allow_compressed_reply, parent, new_parent_state);
}
void LLHTTPClient::post(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive, AIStateMachine* parent, AIStateMachine::state_type new_parent_state)
{
	request(url, HTTP_POST, new LLSDInjector(body), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), keepalive, no_does_authentication, allow_compressed_reply, parent, new_parent_state);
}
void LLHTTPClient::post_approved(std::string const& url, LLSD const& body, ResponderPtr responder, AIHTTPHeaders& headers, AIPerService::Approvement* approved DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive, AIStateMachine* parent, AIStateMachine::state_type new_parent_state)
{
	request(url, HTTP_POST, new LLSDInjector(body), responder, headers, approved DEBUG_CURLIO_PARAM(debug), keepalive, no_does_authentication, allow_compressed_reply, parent, new_parent_state, &gMainThreadEngine);
}
void LLHTTPClient::postXMLRPC(std::string const& url, XMLRPC_REQUEST xmlrpc_request, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive)
{
  	request(url, HTTP_POST, new XMLRPCInjector(xmlrpc_request), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), keepalive, does_authentication, allow_compressed_reply);
}
void LLHTTPClient::postXMLRPC(std::string const& url, char const* method, XMLRPC_VALUE value, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive)
{
	XMLRPC_REQUEST xmlrpc_request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(xmlrpc_request, method);
	XMLRPC_RequestSetRequestType(xmlrpc_request, xmlrpc_request_call);
	XMLRPC_RequestSetData(xmlrpc_request, value);
  	request(url, HTTP_POST, new XMLRPCInjector(xmlrpc_request), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), keepalive, does_authentication, no_allow_compressed_reply);
}
void LLHTTPClient::postRaw(std::string const& url, U8 const* data, S32 size, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive)
{
	request(url, HTTP_POST, new RawInjector(data, size), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), keepalive);
}
void LLHTTPClient::postFile(std::string const& url, std::string const& filename, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive)
{
	request(url, HTTP_POST, new FileInjector(filename), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), keepalive);
}
void LLHTTPClient::postFile(std::string const& url, LLUUID const& uuid, LLAssetType::EType asset_type, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug), EKeepAlive keepalive)
{
	request(url, HTTP_POST, new VFileInjector(uuid, asset_type), responder, headers, NULL DEBUG_CURLIO_PARAM(debug), keepalive);
}
void LLHTTPClient::del(std::string const& url, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	request(url, HTTP_DELETE, NULL, responder, headers, NULL DEBUG_CURLIO_PARAM(debug));
}
void LLHTTPClient::move(std::string const& url, std::string const& destination, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	headers.addHeader("Destination", destination);
	request(url, HTTP_MOVE, NULL, responder, headers, NULL DEBUG_CURLIO_PARAM(debug));
}
void LLHTTPClient::copy(std::string const& url, std::string const& destination, ResponderPtr responder, AIHTTPHeaders& headers DEBUG_CURLIO_PARAM(EDebugCurl debug))
{
	headers.addHeader("Destination", destination);
	request(url, HTTP_COPY, NULL, responder, headers, NULL DEBUG_CURLIO_PARAM(debug));
}
