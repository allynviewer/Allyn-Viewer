/**
 * @file   llcapabilitylistener.h
 * @author Nat Goodspeed
 * @date   2009-01-07
 * @brief  Provide an event-based API for capability requests
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#if ! defined(LL_LLCAPABILITYLISTENER_H)
#define LL_LLCAPABILITYLISTENER_H
#include "llevents.h"
#include "llsdmessage.h"
#include "llerror.h"
class LLCapabilityProvider;
class LLMessageSystem;
class LLSD;
class LLCapabilityListener
{
    LOG_CLASS(LLCapabilityListener);
public:
    LLCapabilityListener(const std::string& name, LLMessageSystem* messageSystem,
                         const LLCapabilityProvider& provider,
                         const LLUUID& agentID, const LLUUID& sessionID);
    typedef LLSDMessage::ArgError ArgError;
    LLEventPump& getCapAPI() { return mEventPump; }
    class CapabilityMapper
    {
    public:
        CapabilityMapper(const std::string& cap, const std::string& reply = "");
        virtual ~CapabilityMapper();
        std::string getCapName() const { return mCapName; }
        std::string getReplyName() const { return mReplyName; }
        virtual void buildMessage(LLMessageSystem* messageSystem,
                                  const LLUUID& agentID,
                                  const LLUUID& sessionID,
                                  const std::string& capabilityName,
                                  const LLSD& payload) const = 0;
        virtual LLSD readResponse(LLMessageSystem* messageSystem) const;
    private:
        const std::string mCapName;
        const std::string mReplyName;
    };
private:
    const LLCapabilityProvider& mProvider;
    LLEventStream mEventPump;
    LLMessageSystem* mMessageSystem;
    LLUUID mAgentID, mSessionID;
    bool capListener(const LLSD&);
    class CapabilityMappers;
};
#endif
