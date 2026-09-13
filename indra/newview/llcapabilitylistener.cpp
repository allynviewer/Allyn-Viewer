/**
 * @file   llcapabilitylistener.cpp
 * @author Nat Goodspeed
 * @date   2009-01-07
 * @brief  Implementation for llcapabilitylistener.
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
#include "llviewerprecompiledheaders.h"
#include "llcapabilitylistener.h"
#include <map>
#include <boost/bind.hpp>
#include "stringize.h"
#include "llcapabilityprovider.h"
#include "message.h"
class LLCapabilityListener::CapabilityMappers: public LLSingleton<LLCapabilityListener::CapabilityMappers>
{
public:
    void registerMapper(const LLCapabilityListener::CapabilityMapper*);
    void unregisterMapper(const LLCapabilityListener::CapabilityMapper*);
    const LLCapabilityListener::CapabilityMapper* find(const std::string& cap) const;
    struct DupCapMapper: public std::runtime_error
    {
        DupCapMapper(const std::string& what):
            std::runtime_error(std::string("DupCapMapper: ") + what)
        {}
    };
private:
    friend class LLSingleton<LLCapabilityListener::CapabilityMappers>;
    CapabilityMappers();
    typedef std::map<std::string, const LLCapabilityListener::CapabilityMapper*> CapabilityMap;
    CapabilityMap mMap;
};
LLCapabilityListener::LLCapabilityListener(const std::string& name,
                                           LLMessageSystem* messageSystem,
                                           const LLCapabilityProvider& provider,
                                           const LLUUID& agentID,
                                           const LLUUID& sessionID):
    mEventPump(name),
    mMessageSystem(messageSystem),
    mProvider(provider),
    mAgentID(agentID),
    mSessionID(sessionID)
{
    mEventPump.listen("self", boost::bind(&LLCapabilityListener::capListener, this, _1));
}
bool LLCapabilityListener::capListener(const LLSD& request)
{
    LLSD::String cap(request["message"]);
    LLSD payload(request["payload"]);
    LLSD::String reply(request["reply"]);
    LLSD::String error(request["error"]);
    LLSD::String timeoutpolicy(request["timeoutpolicy"]);
    if (cap.empty())
    {
        LL_ERRS("capListener") << "capability request event without 'message' key to '"
                               << getCapAPI().getName()
                               << "' on region\n" << mProvider.getDescription()
                               << LL_ENDL;
        return false;
    }
    std::string url = mProvider.getCapability(cap);
    if (! url.empty())
    {
        LLSDMessage::EventResponder* responder =
            new LLSDMessage::EventResponder(LLEventPumps::instance(), request, mProvider.getDescription(), cap, reply, error);
        responder->setTimeoutPolicy(timeoutpolicy);
        LLHTTPClient::post(url, payload, responder);
    }
    else
    {
        const CapabilityMapper* mapper = CapabilityMappers::instance().find(cap);
        if (! mapper)
        {
            LL_ERRS("capListener") << "unsupported capability '" << cap << "' request to '"
                                   << getCapAPI().getName() << "' on region\n"
                                   << mProvider.getDescription()
                                   << LL_ENDL;
        }
        else if (! mapper->getReplyName().empty())
        {
            LL_ERRS("capListener") << "Mapper for capability '" << cap
                                   << "' requires unimplemented support for reply message '"
                                   << mapper->getReplyName()
                                   << "' on '" << getCapAPI().getName() << "' on region\n"
                                   << mProvider.getDescription()
                                   << LL_ENDL;
        }
        else
        {
            LL_INFOS("capListener") << "fallback invoked for capability '" << cap
                                    << "' request to '" << getCapAPI().getName()
                                    << "' on region\n" << mProvider.getDescription()
                                    << LL_ENDL;
            mapper->buildMessage(mMessageSystem, mAgentID, mSessionID, cap, payload);
            mMessageSystem->sendReliable(mProvider.getHost());
        }
    }
    return false;
}
LLCapabilityListener::CapabilityMapper::CapabilityMapper(const std::string& cap, const std::string& reply):
    mCapName(cap),
    mReplyName(reply)
{
    LLCapabilityListener::CapabilityMappers::instance().registerMapper(this);
}
LLCapabilityListener::CapabilityMapper::~CapabilityMapper()
{
    LLCapabilityListener::CapabilityMappers::instance().unregisterMapper(this);
}
LLSD LLCapabilityListener::CapabilityMapper::readResponse(LLMessageSystem* messageSystem) const
{
    return LLSD();
}
LLCapabilityListener::CapabilityMappers::CapabilityMappers() {}
void LLCapabilityListener::CapabilityMappers::registerMapper(const LLCapabilityListener::CapabilityMapper* mapper)
{
    std::pair<CapabilityMap::iterator, bool> inserted =
        mMap.insert(CapabilityMap::value_type(mapper->getCapName(), mapper));
    if (! inserted.second)
    {
        throw DupCapMapper(std::string("Duplicate capability name ") + mapper->getCapName());
    }
}
void LLCapabilityListener::CapabilityMappers::unregisterMapper(const LLCapabilityListener::CapabilityMapper* mapper)
{
    CapabilityMap::iterator found = mMap.find(mapper->getCapName());
    if (found != mMap.end())
    {
        mMap.erase(found);
    }
}
const LLCapabilityListener::CapabilityMapper*
LLCapabilityListener::CapabilityMappers::find(const std::string& cap) const
{
    CapabilityMap::const_iterator found = mMap.find(cap);
    if (found != mMap.end())
    {
        return found->second;
    }
    return NULL;
}
