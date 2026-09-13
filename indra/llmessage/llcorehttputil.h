/**
 * @file llcorehttputil.h
 * @brief Minimal LLCoreHttpUtil shim over LLHTTPClient for WebRTC voice.
 */
#ifndef LL_LLCOREHTTPUTIL_H
#define LL_LLCOREHTTPUTIL_H
#include <memory>
#include <string>
#include "llsd.h"
#include "llhttpstatuscodes.h"
namespace LLCore
{
class HttpStatus
{
public:
	HttpStatus() : mSuccess(true), mStatus(200) {}
	HttpStatus(int status) : mSuccess(status >= 200 && status < 400), mStatus(status) {}
	explicit operator bool() const { return mSuccess; }
	bool operator!() const { return !mSuccess; }
	bool operator==(const HttpStatus& other) const { return mStatus == other.mStatus; }
	bool operator!=(const HttpStatus& other) const { return mStatus != other.mStatus; }
	int getType() const { return mStatus; }
	int getStatus() const { return mStatus; }
private:
	bool mSuccess;
	int mStatus;
};
class HttpRequest
{
public:
	typedef std::shared_ptr<HttpRequest> ptr_t;
	typedef int policy_t;
	static const int DEFAULT_POLICY_ID = 0;
};
class HttpOptions
{
public:
	typedef std::shared_ptr<HttpOptions> ptr_t;
	void setWantHeaders(bool) {}
};
}
namespace LLCoreHttpUtil
{
class HttpCoroutineAdapter
{
public:
	typedef std::shared_ptr<HttpCoroutineAdapter> ptr_t;
	static const char* HTTP_RESULTS;
	HttpCoroutineAdapter(const std::string& name, int );
	~HttpCoroutineAdapter();
	LLSD postAndSuspend(const LLCore::HttpRequest::ptr_t& request,
						const std::string& url,
						const LLSD& body);
	LLSD postAndSuspend(const LLCore::HttpRequest::ptr_t& request,
						const std::string& url,
						const LLSD& body,
						const LLCore::HttpOptions::ptr_t& options);
	static LLCore::HttpStatus getStatusFromLLSD(const LLSD& httpResults);
	static void messageHttpPost(const std::string& url,
								const LLSD& body,
								const std::string& success_msg,
								const std::string& failure_msg);
private:
	std::string mName;
};
}
#endif
