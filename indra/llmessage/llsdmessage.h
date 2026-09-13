/**
 * @file   llsdmessage.h
 * @author Nat Goodspeed
 * @date   2008-10-30
 * @brief  API intended to unify sending capability, UDP and TCP messages:
 *         https://wiki.lindenlab.com/wiki/Viewer:Messaging/Messaging_Notes
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#if ! defined(LL_LLSDMESSAGE_H)
#define LL_LLSDMESSAGE_H
#include "llerror.h"
#include "llevents.h"
#include "llhttpclient.h"
#include <string>
#include <stdexcept>
class LLSD;
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy eventResponder_timeout;
class LLSDMessage
{
    LOG_CLASS(LLSDMessage);
public:
    LLSDMessage();
    struct ArgError: public std::runtime_error
    {
        ArgError(const std::string& what):
            std::runtime_error(std::string("ArgError: ") + what) {}
    };
    class ResponderAdapter
    {
    public:
        ResponderAdapter(LLHTTPClient::ResponderWithResult* responder,
                         const std::string& name="ResponderAdapter");
        std::string getReplyName() const { return mReplyPump.getName(); }
        std::string getErrorName() const { return mErrorPump.getName(); }
        std::string getTimeoutPolicyName() const;
    private:
        bool listener(const LLSD&, bool success);
        LLHTTPClient::ResponderPtr mResponder;
        LLEventStream mReplyPump, mErrorPump;
    };
    static void link();
private:
    friend class LLCapabilityListener;
    class EventResponder: public LLHTTPClient::ResponderWithResult
    {
    public:
        EventResponder(LLEventPumps& pumps,
                       const LLSD& request,
                       const std::string& target, const std::string& message,
                       const std::string& replyPump, const std::string& errorPump);
		void setTimeoutPolicy(std::string const& name);
        void httpSuccess(void);
        void httpFailure(void);
		AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return *mHTTPTimeoutPolicy; }
		char const* getName(void) const { return "EventResponder"; }
    private:
        LLEventPumps& mPumps;
        LLReqID mReqID;
        const std::string mTarget, mMessage, mReplyPump, mErrorPump;
        AIHTTPTimeoutPolicy const* mHTTPTimeoutPolicy;
    };
private:
    bool httpListener(const LLSD&);
    LLEventStream mEventPump;
};
#endif
