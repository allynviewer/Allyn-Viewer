/**
 * @file   lleventcoro.cpp
 * @author Nat Goodspeed
 * @date   2009-04-29
 * @brief  Implementation for lleventcoro.
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
#include "lleventcoro.h"
#include <chrono>
#include <exception>
#include <boost/fiber/operations.hpp>
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llerror.h"
#include "llcoros.h"
#include "stringize.h"
namespace
{
std::string listenerNameForCoro()
{
    std::string name(LLCoros::getName());
    if (! name.empty())
    {
        return name;
    }
    name = LLEventPump::inventName("coro");
    LL_INFOS("LLEventCoro") << "listenerNameForCoro(): inventing coro name '"
                            << name << "'" << LL_ENDL;
    return name;
}
void storeToLLSDPath(LLSD& dest, const LLSD& rawPath, const LLSD& value)
{
    if (rawPath.isUndefined())
    {
        return;
    }
    LLSD path;
    if (rawPath.isArray())
    {
        path = rawPath;
    }
    else
    {
        path.append(rawPath);
    }
    LLSD* pdest = &dest;
    for (LLSD::Integer i = 0; i < path.size(); ++i)
    {
        if (path[i].isString())
        {
            pdest = &((*pdest)[path[i].asString()]);
        }
        else if (path[i].isInteger())
        {
            pdest = &((*pdest)[path[i].asInteger()]);
        }
        else
        {
            LL_ERRS("lleventcoro") << "storeToLLSDPath(" << dest << ", " << rawPath << ", " << value
                                   << "): path[" << i << "] bad type " << path[i].type() << LL_ENDL;
        }
    }
    *pdest = value;
}
}
void llcoro::suspend()
{
    LLCoros::checkStop();
    LLCoros::TempStatus st("waiting one tick");
    boost::this_fiber::yield();
}
void llcoro::suspendUntilTimeout(float seconds)
{
    LLCoros::checkStop();
    LLEventStream bogus("xyzzy", true);
    static LLSD timedout;
    suspendUntilEventOnWithTimeout(bogus, seconds, timedout);
}
void llcoro::suspendUntilNextFrame()
{
    LLCoros::checkStop();
    LLCoros::TempStatus st("waiting for next frame");
    LLEventPumpOrPumpName mainloop_pump("mainloop");
    suspendUntilEventOn(mainloop_pump);
}
namespace
{
std::pair<LLBoundListener, LLBoundListener>
postAndSuspendSetup(const std::string& callerName,
                    const std::string& listenerName,
                    LLCoros::Promise<LLSD>& promise,
                    const LLSD& event,
                    const LLEventPumpOrPumpName& requestPumpP,
                    const LLEventPumpOrPumpName& replyPumpP,
                    const LLSD& replyPumpNamePath)
{
    LLCoros::checkStop();
    bool consuming(LLCoros::get_consuming());
    llassert_always_msg(replyPumpP, ("replyPump required for " + callerName));
    LLEventPump& replyPump(replyPumpP.getPump());
    LLBoundListener stopper(
        LLEventPumps::instance().obtain("LLApp").listen(
            listenerName,
            [&promise, listenerName](const LLSD& status)
            {
                auto& statsd = status["status"];
                if (statsd.asString() != "running")
                {
                    LL_DEBUGS("lleventcoro") << listenerName
                                             << " spotted status " << statsd
                                             << ", throwing Stopping" << LL_ENDL;
                    try
                    {
                        promise.set_exception(
                            std::make_exception_ptr(
                                LLCoros::Stopping("status " + statsd.asString())));
                    }
                    catch (const boost::fibers::promise_already_satisfied&)
                    {
                        LL_WARNS("lleventcoro") << listenerName
                                                << " couldn't throw Stopping "
                                                   "because promise already set" << LL_ENDL;
                    }
                }
                return false;
            }));
    LLBoundListener connection(
        replyPump.listen(
            listenerName,
            [&promise, consuming, listenerName](const LLSD& result)
            {
                try
                {
                    promise.set_value(result);
                    return consuming;
                }
                catch(boost::fibers::promise_already_satisfied & ex)
                {
                    LL_DEBUGS("lleventcoro") << "promise already satisfied in '"
                        << listenerName << "': "  << ex.what() << LL_ENDL;
                    return false;
                }
            }));
    if (requestPumpP)
    {
        LLEventPump& requestPump(requestPumpP.getPump());
        LLSD modevent(event);
        storeToLLSDPath(modevent, replyPumpNamePath, replyPump.getName());
        LL_DEBUGS("lleventcoro") << callerName << ": coroutine " << listenerName
                                 << " posting to " << requestPump.getName()
                                 << LL_ENDL;
        requestPump.post(modevent);
    }
    LL_DEBUGS("lleventcoro") << callerName << ": coroutine " << listenerName
                             << " about to wait on LLEventPump " << replyPump.getName()
                             << LL_ENDL;
    return { connection, stopper };
}
}
LLSD llcoro::postAndSuspend(const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                 const LLEventPumpOrPumpName& replyPump, const LLSD& replyPumpNamePath)
{
    LLCoros::Promise<LLSD> promise;
    std::string listenerName(listenerNameForCoro());
    auto connections =
        postAndSuspendSetup("postAndSuspend()", listenerName, promise,
                            event, requestPump, replyPump, replyPumpNamePath);
    LLTempBoundListener connection(connections.first), stopper(connections.second);
    LLCoros::Future<LLSD> future = LLCoros::getFuture(promise);
    LLCoros::TempStatus st(STRINGIZE("waiting for " << replyPump.getPump().getName()));
    LLSD value(future.get());
    LL_DEBUGS("lleventcoro") << "postAndSuspend(): coroutine " << listenerName
                             << " resuming with " << value << LL_ENDL;
    return value;
}
LLSD llcoro::postAndSuspendWithTimeout(const LLSD& event,
                                       const LLEventPumpOrPumpName& requestPump,
                                       const LLEventPumpOrPumpName& replyPump,
                                       const LLSD& replyPumpNamePath,
                                       F32 timeout, const LLSD& timeoutResult)
{
    LLCoros::Promise<LLSD> promise;
    std::string listenerName(listenerNameForCoro());
    auto connections =
        postAndSuspendSetup("postAndSuspendWithTimeout()", listenerName, promise,
                            event, requestPump, replyPump, replyPumpNamePath);
    LLTempBoundListener connection(connections.first), stopper(connections.second);
    LLCoros::Future<LLSD> future = LLCoros::getFuture(promise);
    boost::fibers::future_status status;
    {
        LLCoros::TempStatus st(STRINGIZE("waiting for " << replyPump.getPump().getName()
                                         << " for " << timeout << "s"));
        status = future.wait_for(std::chrono::milliseconds(long(timeout * 1000)));
    }
    if (status == boost::fibers::future_status::timeout)
    {
        LL_DEBUGS("lleventcoro") << "postAndSuspendWithTimeout(): coroutine " << listenerName
                                 << " timed out after " << timeout << " seconds,"
                                 << " resuming with " << timeoutResult << LL_ENDL;
        return timeoutResult;
    }
    else
    {
        llassert_always(status == boost::fibers::future_status::ready);
        LLSD value(future.get());
        LL_DEBUGS("lleventcoro") << "postAndSuspendWithTimeout(): coroutine " << listenerName
                                 << " resuming with " << value << LL_ENDL;
        return value;
    }
}
