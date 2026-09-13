/**
 * @file   llareslistener.cpp
 * @author Nat Goodspeed
 * @date   2009-03-18
 * @brief  Implementation for llareslistener.
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
#include "linden_common.h"
#include "llareslistener.h"
#include "llares.h"
#include "llerror.h"
#include "llevents.h"
#include "llsdutil.h"
LLAresListener::LLAresListener(LLAres* llares):
    LLEventAPI("LLAres",
               "LLAres listener to request DNS operations"),
    mAres(llares)
{
    add("rewriteURI",
        "Given [\"uri\"], return on [\"reply\"] an array of alternative URIs.\n"
        "On failure, returns an array containing only the original URI, so\n"
        "failure case can be processed like success case.",
        &LLAresListener::rewriteURI,
        LLSD().with("uri", LLSD()).with("reply", LLSD()));
}
class UriRewriteResponder: public LLAres::UriRewriteResponder
{
public:
    UriRewriteResponder(const LLSD& request):
        mReqID(request),
        mPumpName(request["reply"])
    {}
    virtual void rewriteResult(const std::vector<std::string>& uris)
    {
        LLSD result;
        for (std::vector<std::string>::const_iterator ui(uris.begin()), uend(uris.end());
             ui != uend; ++ui)
        {
            result.append(*ui);
        }
        mReqID.stamp(result);
        LLEventPumps::instance().obtain(mPumpName).post(result);
    }
private:
    LLReqID mReqID;
    const std::string mPumpName;
};
void LLAresListener::rewriteURI(const LLSD& data)
{
	if (mAres)
	{
		mAres->rewriteURI(data["uri"], new UriRewriteResponder(data));
	}
	else
	{
		LL_INFOS() << "LLAresListener::rewriteURI requested without Ares present. Ignoring: " << data << LL_ENDL;
	}
}
