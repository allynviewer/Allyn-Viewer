/** 
 * @file llurlrequest.h
 * @author Phoenix
 * @date 2005-04-21
 * @brief Declaration of url based requests on pipes.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * Copyright (C) 2012, Aleric Inglewood.
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
#ifndef LL_LLURLREQUEST_H
#define LL_LLURLREQUEST_H
#include <string>
#include "aicurleasyrequeststatemachine.h"
#include "aihttpheaders.h"
class Injector
{
  public:
	typedef LLHTTPClient::ResponderBase::buffer_ptr_t buffer_ptr_t;
	virtual char const* contentType(void) const = 0;
	virtual U32 get_body(LLChannelDescriptors const& channels, buffer_ptr_t& buffer) = 0;
	virtual ~Injector() { }
};
class LLURLRequest : public AICurlEasyRequestStateMachine {
  public:
	typedef LLHTTPClient::ERequestAction ERequestAction;
	static std::string actionAsVerb(ERequestAction action);
	LLURLRequest(ERequestAction action, std::string const& url, Injector* body,
				 LLHTTPClient::ResponderPtr responder, AIHTTPHeaders& headers,
				 AIPerService::Approvement* approved,
				 bool keepalive, bool is_auth, bool no_compression);
	const char* getName() const { return mResponderNameCache.c_str(); }
  protected:
	~LLURLRequest() { }
  public:
    void useProxy(bool use_proxy);
	void addHeader(char const* header);
  private:
	bool configure(AICurlEasyRequest_wat const& curlEasyRequest_w);
  private:
	ERequestAction mAction;
	std::string mURL;
	bool mKeepAlive;
	bool mIsAuth;
	bool mNoCompression;
	Injector* mBody;
	U32 mBodySize;
	LLHTTPClient::ResponderPtr mResponder;
	AIHTTPHeaders mHeaders;
	std::string mResponderNameCache;
  protected:
	void initialize_impl(void);
	void force_killed(void);
};
#endif
