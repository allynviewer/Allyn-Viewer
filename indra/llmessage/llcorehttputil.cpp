/**
 * @file llcorehttputil.cpp
 * @brief Minimal LLCoreHttpUtil shim over LLHTTPClient for WebRTC voice.
 */
#include "linden_common.h"
#include "llcorehttputil.h"
#include "llhttpclient.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llsdutil.h"
namespace LLCoreHttpUtil
{
const char* HttpCoroutineAdapter::HTTP_RESULTS = "http_result";
namespace
{
class PostSuspendResponder : public LLHTTPClient::ResponderWithResult
{
public:
	PostSuspendResponder(const std::string& reply_pump)
	:	mReplyPump(reply_pump)
	{}
	void httpSuccess(void)
	{
		LLSD result = getContent();
		if (!result.isMap())
		{
			LLSD wrap;
			wrap["body"] = result;
			result = wrap;
		}
		LLSD meta;
		meta["http_status"] = (S32)getStatus();
		meta["success"] = true;
		result[HttpCoroutineAdapter::HTTP_RESULTS] = meta;
		LLEventPumps::instance().obtain(mReplyPump).post(result);
	}
	void httpFailure(void)
	{
		LLSD result;
		LLSD meta;
		meta["http_status"] = (S32)getStatus();
		meta["success"] = false;
		meta["reason"] = getReason();
		result[HttpCoroutineAdapter::HTTP_RESULTS] = meta;
		LLEventPumps::instance().obtain(mReplyPump).post(result);
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const
	{
		return responderIgnore_timeout;
	}
	char const* getName(void) const
	{
		return "PostSuspendResponder";
	}
private:
	std::string mReplyPump;
};
class MessagePostResponder : public LLHTTPClient::ResponderWithResult
{
public:
	MessagePostResponder(const std::string& success_msg, const std::string& failure_msg)
	:	mSuccess(success_msg)
	,	mFailure(failure_msg)
	{}
	void httpSuccess(void)
	{
		LL_INFOS("Voice") << mSuccess << LL_ENDL;
	}
	void httpFailure(void)
	{
		LL_WARNS("Voice") << mFailure << " status=" << getStatus() << " reason=" << getReason() << LL_ENDL;
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const
	{
		return responderIgnore_timeout;
	}
	char const* getName(void) const
	{
		return "MessagePostResponder";
	}
private:
	std::string mSuccess;
	std::string mFailure;
};
}
HttpCoroutineAdapter::HttpCoroutineAdapter(const std::string& name, int )
:	mName(name)
{
}
HttpCoroutineAdapter::~HttpCoroutineAdapter()
{
}
LLSD HttpCoroutineAdapter::postAndSuspend(const LLCore::HttpRequest::ptr_t& request,
										  const std::string& url,
										  const LLSD& body)
{
	return postAndSuspend(request, url, body, LLCore::HttpOptions::ptr_t());
}
LLSD HttpCoroutineAdapter::postAndSuspend(const LLCore::HttpRequest::ptr_t& ,
										  const std::string& url,
										  const LLSD& body,
										  const LLCore::HttpOptions::ptr_t& )
{
	static U32 sCounter = 0;
	const std::string reply_pump = llformat("HttpCoroutineAdapter.%s.%u", mName.c_str(), ++sCounter);
	if (url.empty())
	{
		LLSD result;
		LLSD meta;
		meta["http_status"] = 0;
		meta["success"] = false;
		meta["reason"] = "empty url";
		result[HTTP_RESULTS] = meta;
		return result;
	}
	LLHTTPClient::post(url, body, new PostSuspendResponder(reply_pump));
	return llcoro::suspendUntilEventOn(reply_pump);
}
LLCore::HttpStatus HttpCoroutineAdapter::getStatusFromLLSD(const LLSD& httpResults)
{
	if (!httpResults.isMap())
	{
		return LLCore::HttpStatus(0);
	}
	if (httpResults.has("success") && httpResults["success"].asBoolean())
	{
		return LLCore::HttpStatus(httpResults.has("http_status") ? httpResults["http_status"].asInteger() : 200);
	}
	return LLCore::HttpStatus(httpResults.has("http_status") ? httpResults["http_status"].asInteger() : 500);
}
void HttpCoroutineAdapter::messageHttpPost(const std::string& url,
										   const LLSD& body,
										   const std::string& success_msg,
										   const std::string& failure_msg)
{
	if (url.empty())
	{
		LL_WARNS("Voice") << failure_msg << " (empty url)" << LL_ENDL;
		return;
	}
	LLHTTPClient::post(url, body, new MessagePostResponder(success_msg, failure_msg));
}
}
