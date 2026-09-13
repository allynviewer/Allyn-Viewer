/**
 * @file   lleventapi.cpp
 * @author Nat Goodspeed
 * @date   2009-11-10
 * @brief  Implementation for lleventapi.
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
#include "lleventapi.h"
#include "llerror.h"
LLEventAPI::LLEventAPI(const std::string& name, const std::string& desc, const std::string& field):
    lbase(name, field),
    ibase(name),
    mDesc(desc)
{
}
LLEventAPI::~LLEventAPI()
{
}
LLEventAPI::Response::Response(const LLSD& seed, const LLSD& request, const LLSD::String& replyKey):
    mResp(seed),
    mReq(request),
    mKey(replyKey)
{}
LLEventAPI::Response::~Response()
{
    sendReply(mResp, mReq, mKey);
}
void LLEventAPI::Response::warn(const std::string& warning)
{
    LL_WARNS("LLEventAPI::Response") << warning << LL_ENDL;
    mResp["warnings"].append(warning);
}
void LLEventAPI::Response::error(const std::string& error)
{
    LL_WARNS("LLEventAPI::Response") << error << LL_ENDL;
    mResp["error"] = error;
}
