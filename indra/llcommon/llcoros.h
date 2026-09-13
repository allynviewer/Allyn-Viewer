/**
 * @file   llcoros.h
 * @author Nat Goodspeed
 * @date   2009-06-02
 * @brief  Manage running boost::coroutine instances
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
#if ! defined(LL_LLCOROS_H)
#define LL_LLCOROS_H
#include "llexception.h"
#include <boost/fiber/fss.hpp>
#include <boost/fiber/future/future.hpp>
#include <boost/fiber/future/promise.hpp>
#include <boost/fiber/recursive_mutex.hpp>
#include "mutex.h"
#include "llsingleton.h"
#include "llinstancetracker.h"
#include <functional>
#include <string>
#include <exception>
#include <queue>
#define LLCOROS_MUTEX_HEADER   <boost/fiber/mutex.hpp>
#define LLCOROS_CONDVAR_HEADER <boost/fiber/condition_variable.hpp>
namespace boost {
    namespace fibers {
        class mutex;
        enum class cv_status;
        class condition_variable;
    }
}
class LL_COMMON_API LLCoros: public LLSingleton<LLCoros>
{
    friend class LLSingleton<LLCoros>;
    LLCoros();
    ~LLCoros();
public:
    static bool on_main_coro();
    static bool on_main_thread_main_coro();
    typedef boost::fibers::fiber coro;
    typedef std::function<void()> callable_t;
    std::string launch(const std::string& prefix, const callable_t& callable);
    static std::string getName();
    void rethrow();
    static std::string logname();
    void setStackSize(S32 stacksize);
    void printActiveCoroutines(const std::string& when=std::string());
    static coro::id get_self();
    static void set_consuming(bool consuming);
    static bool get_consuming();
    class OverrideConsuming
    {
    public:
        OverrideConsuming(bool consuming):
            mPrevConsuming(get_consuming())
        {
            set_consuming(consuming);
        }
        OverrideConsuming(const OverrideConsuming&) = delete;
        ~OverrideConsuming()
        {
            set_consuming(mPrevConsuming);
        }
    private:
        bool mPrevConsuming;
    };
    static void setStatus(const std::string& status);
    static std::string getStatus();
    class TempStatus
    {
    public:
        TempStatus(const std::string& status):
            mOldStatus(getStatus())
        {
            setStatus(status);
        }
        TempStatus(const TempStatus&) = delete;
        ~TempStatus()
        {
            setStatus(mOldStatus);
        }
    private:
        std::string mOldStatus;
    };
    struct Stop: public LLContinueError
    {
        Stop(const std::string& what): LLContinueError(what) {}
    };
    struct Stopping: public Stop
    {
        Stopping(const std::string& what): Stop(what) {}
    };
    struct Stopped: public Stop
    {
        Stopped(const std::string& what): Stop(what) {}
    };
    struct Shutdown: public Stop
    {
        Shutdown(const std::string& what): Stop(what) {}
    };
    static void checkStop();
    template <typename T>
    using Promise = boost::fibers::promise<T>;
    template <typename T>
    using Future = boost::fibers::future<T>;
    template <typename T>
    static Future<T> getFuture(Promise<T>& promise) { return promise.get_future(); }
    using Mutex = boost::fibers::mutex;
    using RMutex = boost::fibers::recursive_mutex;
    using LockType = std::unique_lock<Mutex>;
    using cv_status = boost::fibers::cv_status;
    using ConditionVariable = boost::fibers::condition_variable;
    template <typename T>
    using local_ptr = boost::fibers::fiber_specific_ptr<T>;
private:
    void cleanupSingleton();
    std::string generateDistinctName(const std::string& prefix) const;
    void toplevel(std::string name, callable_t callable);
    struct CoroData;
    static CoroData& get_CoroData(const std::string& caller);
    void saveException(const std::string& name, std::exception_ptr exc);
    struct ExceptionData
    {
        ExceptionData(const std::string& nm, std::exception_ptr exc):
            name(nm),
            exception(exc)
        {}
        std::string name;
        std::exception_ptr exception;
    };
    std::queue<ExceptionData> mExceptionQueue;
    S32 mStackSize;
    struct CoroData: public LLInstanceTracker<CoroData, std::string>
    {
        CoroData(const std::string& name);
        CoroData(int n);
        const std::string mName;
        bool mConsuming;
        std::string mStatus;
        F64 mCreationTime;
    };
    local_ptr<CoroData> mCurrent;
};
namespace llcoro
{
inline
std::string logname() { return LLCoros::logname(); }
}
#endif
