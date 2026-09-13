/**
 * @file   llcoros.cpp
 * @author Nat Goodspeed
 * @date   2009-06-03
 * @brief  Implementation for llcoros.
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
#include "llwin32headers.h"
#include "linden_common.h"
#include "llcoros.h"
#include <atomic>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/fiber/fiber.hpp>
#ifndef BOOST_DISABLE_ASSERTS
#define UNDO_BOOST_DISABLE_ASSERTS
#define BOOST_DISABLE_ASSERTS
#endif
#include <boost/fiber/protected_fixedsize_stack.hpp>
#ifdef UNDO_BOOST_DISABLE_ASSERTS
#undef UNDO_BOOST_DISABLE_ASSERTS
#undef BOOST_DISABLE_ASSERTS
#endif
#include "llapp.h"
#include "lltimer.h"
#include "llevents.h"
#include "llerror.h"
#include "stringize.h"
#include "llexception.h"
#include "aithreadid.h"
#if LL_WINDOWS
#include <excpt.h>
#endif
bool LLCoros::on_main_coro()
{
    if (!LLCoros::instanceExists() || LLCoros::getName().empty())
    {
        return true;
    }
    return false;
}
bool LLCoros::on_main_thread_main_coro()
{
    return on_main_coro() && is_main_thread();
}
LLCoros::CoroData& LLCoros::get_CoroData(const std::string& caller)
{
    CoroData* current{ nullptr };
    if (! destroyed())
    {
        current = instance().mCurrent.get();
    }
    if (! current)
    {
        static std::atomic<int> which_thread(0);
        static thread_local CoroData sMain(which_thread++);
        current = &sMain;
    }
    return *current;
}
LLCoros::coro::id LLCoros::get_self()
{
    return boost::this_fiber::get_id();
}
void LLCoros::set_consuming(bool consuming)
{
    CoroData& data(get_CoroData("set_consuming()"));
    llassert_always(! data.mName.empty());
    data.mConsuming = consuming;
}
bool LLCoros::get_consuming()
{
    return get_CoroData("get_consuming()").mConsuming;
}
void LLCoros::setStatus(const std::string& status)
{
    get_CoroData("setStatus()").mStatus = status;
}
std::string LLCoros::getStatus()
{
    return get_CoroData("getStatus()").mStatus;
}
LLCoros::LLCoros():
    mStackSize(1024*1024),
    mCurrent{ [](CoroData*){} }
{
}
LLCoros::~LLCoros()
{
    cleanupSingleton();
}
void LLCoros::cleanupSingleton()
{
    printActiveCoroutines("at entry to ~LLCoros()");
    for (size_t count = 0; count < 10 && CoroData::instanceCount() > 0; ++count)
    {
        boost::this_fiber::yield();
    }
    printActiveCoroutines("after pumping");
}
std::string LLCoros::generateDistinctName(const std::string& prefix) const
{
    static int unique = 0;
    if (prefix.empty())
    {
        LL_ERRS("LLCoros") << "LLCoros::launch(): pass non-empty name string" << LL_ENDL;
    }
    std::string name(prefix);
    while (CoroData::getInstance(name))
    {
        name = STRINGIZE(prefix << unique++);
    }
    return name;
}
std::string LLCoros::getName()
{
    return get_CoroData("getName()").mName;
}
std::string LLCoros::logname()
{
    LLCoros::CoroData& data(get_CoroData("logname()"));
    return data.mName.empty() ? std::string("main") : data.mName;
}
void LLCoros::saveException(const std::string& name, std::exception_ptr exc)
{
    mExceptionQueue.emplace(name, exc);
}
void LLCoros::rethrow()
{
    if (! mExceptionQueue.empty())
    {
        ExceptionData front = mExceptionQueue.front();
        mExceptionQueue.pop();
        LL_WARNS("LLCoros") << "Rethrowing exception from coroutine " << front.name << LL_ENDL;
        std::rethrow_exception(front.exception);
    }
}
void LLCoros::setStackSize(S32 stacksize)
{
    LL_DEBUGS("LLCoros") << "Setting coroutine stack size to " << stacksize << LL_ENDL;
    mStackSize = stacksize;
}
void LLCoros::printActiveCoroutines(const std::string& when)
{
    LL_INFOS("LLCoros") << "Number of active coroutines " << when
                        << ": " << CoroData::instanceCount() << LL_ENDL;
    if (CoroData::instanceCount() > 0)
    {
        LL_INFOS("LLCoros") << "-------------- List of active coroutines ------------"
                            << " (detailed coroutine list requires instance_snapshot)" << LL_ENDL;
        LL_INFOS("LLCoros") << "-----------------------------------------------------" << LL_ENDL;
    }
}
std::string LLCoros::launch(const std::string& prefix, const callable_t& callable)
{
    std::string name(generateDistinctName(prefix));
    try
    {
        boost::fibers::fiber newCoro(boost::fibers::launch::dispatch,
            std::allocator_arg,
            boost::fibers::protected_fixedsize_stack(mStackSize),
            [this, &name, &callable]() { toplevel(name, callable); });
        newCoro.detach();
    }
    catch (std::bad_alloc&)
    {
        LL_WARNS("LLCoros") << "Bad memory allocation in LLCoros::launch(" << prefix << ")!" << LL_ENDL;
    }
    return name;
}
namespace
{
#if LL_WINDOWS
static const U32 STATUS_MSC_EXCEPTION = 0xE06D7363;
U32 exception_filter(U32 code, struct _EXCEPTION_POINTERS* exception_infop)
{
    if (code == STATUS_MSC_EXCEPTION)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }
}
void cpphandle(const LLCoros::callable_t& callable, const std::string& name)
{
    try
    {
        callable();
    }
    catch (const LLCoros::Stop& exc)
    {
        LL_INFOS("LLCoros") << "coroutine " << name << " terminating because "
            << exc.what() << LL_ENDL;
    }
    catch (const LLContinueError&)
    {
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << name));
    }
}
void sehandle(const LLCoros::callable_t& callable, const std::string& name)
{
    __try
    {
        cpphandle(callable, name);
    }
    __except (exception_filter(GetExceptionCode(), GetExceptionInformation()))
    {
        char integer_string[512];
        sprintf(integer_string, "SEH, code: %lu\n", GetExceptionCode());
        throw std::exception(integer_string);
    }
}
#endif
}
void LLCoros::toplevel(std::string name, callable_t callable)
{
    CoroData corodata(name);
    mCurrent.reset(&corodata);
#ifdef LL_WINDOWS
    sehandle(callable, name);
#else
    try
    {
        callable();
    }
    catch (const Stop& exc)
    {
        LL_INFOS("LLCoros") << "coroutine " << name << " terminating because "
                            << exc.what() << LL_ENDL;
    }
    catch (const LLContinueError&)
    {
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << name));
    }
    catch (...)
    {
        LL_WARNS("LLCoros") << "Capturing uncaught exception in coroutine "
                            << name << LL_ENDL;
        LLCoros::instance().saveException(name, std::current_exception());
    }
#endif
}
void LLCoros::checkStop()
{
    if (destroyed())
    {
        LLTHROW(Shutdown("LLCoros was deleted"));
    }
    if (getName().empty())
    {
        return;
    }
    if (LLApp::isStopped())
    {
        LLTHROW(Stopped("viewer is stopped"));
    }
    if (! LLApp::isRunning())
    {
        LLTHROW(Stopping("viewer is stopping"));
    }
}
LLCoros::CoroData::CoroData(const std::string& name):
    LLInstanceTracker<CoroData, std::string>(name),
    mName(name),
    mConsuming(false),
    mCreationTime(LLTimer::getTotalSeconds())
{
}
LLCoros::CoroData::CoroData(int n):
    LLInstanceTracker<CoroData, std::string>("main" + stringize(n)),
    mName(),
    mConsuming(false),
    mCreationTime(LLTimer::getTotalSeconds())
{
}
