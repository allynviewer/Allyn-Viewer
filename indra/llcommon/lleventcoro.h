/**
 * @file   lleventcoro.h
 * @author Nat Goodspeed
 * @date   2009-04-29
 * @brief  Utilities to interface between coroutines and events.
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
#if ! defined(LL_LLEVENTCORO_H)
#define LL_LLEVENTCORO_H
#include <string>
#include "llevents.h"
class LLEventPumpOrPumpName
{
public:
    LLEventPumpOrPumpName(LLEventPump& pump):
        mPump(pump)
    {}
    LLEventPumpOrPumpName(const std::string& pumpname):
        mPump(LLEventPumps::instance().obtain(pumpname))
    {}
    LLEventPumpOrPumpName(const char* pumpname):
        mPump(LLEventPumps::instance().obtain(pumpname))
    {}
    LLEventPumpOrPumpName() {}
    operator LLEventPump& () const { return *mPump; }
    LLEventPump& getPump() const { return *mPump; }
    operator bool() const { return bool(mPump); }
    bool operator!() const { return ! mPump; }
private:
    boost::optional<LLEventPump&> mPump;
};
namespace llcoro
{
void suspend();
void suspendUntilTimeout(float seconds);
void suspendUntilNextFrame();
LLSD postAndSuspend(const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                 const LLEventPumpOrPumpName& replyPump, const LLSD& replyPumpNamePath=LLSD());
inline
LLSD suspendUntilEventOn(const LLEventPumpOrPumpName& pump)
{
    return postAndSuspend(LLSD(), LLEventPumpOrPumpName(), pump);
}
LLSD postAndSuspendWithTimeout(const LLSD& event,
                               const LLEventPumpOrPumpName& requestPump,
                               const LLEventPumpOrPumpName& replyPump,
                               const LLSD& replyPumpNamePath,
                               F32 timeout, const LLSD& timeoutResult);
inline
LLSD suspendUntilEventOnWithTimeout(const LLEventPumpOrPumpName& suspendPumpOrName,
                                    F32 timeoutin, const LLSD &timeoutResult)
{
    return postAndSuspendWithTimeout(LLSD(),
                                     LLEventPumpOrPumpName(),
                                     suspendPumpOrName,
                                     LLSD(),
                                     timeoutin,
                                     timeoutResult);
}
}
class LL_COMMON_API LLCoroEventPump
{
public:
    LLCoroEventPump(const std::string& name="coro"):
        mPump(name, true)
    {}
    std::string getName() const { return mPump.getName(); }
    LLEventPump& getPump() { return mPump; }
    LLSD suspend()
    {
        return llcoro::suspendUntilEventOn(mPump);
    }
    LLSD postAndSuspend(const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                     const LLSD& replyPumpNamePath=LLSD())
    {
        return llcoro::postAndSuspend(event, requestPump, mPump, replyPumpNamePath);
    }
private:
    LLEventStream mPump;
};
#endif
