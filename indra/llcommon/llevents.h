/**
 * @file   llevents.h
 * @author Kent Quirk, Nat Goodspeed
 * @date   2008-09-11
 * @brief  This is an implementation of the event system described at
 *         https://wiki.lindenlab.com/wiki/Viewer:Messaging/Event_System,
 *         originally introduced in llnotifications.h. It has nothing
 *         whatsoever to do with the older system in llevent.h.
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
#if ! defined(LL_LLEVENTS_H)
#define LL_LLEVENTS_H
#include <string>
#include <map>
#include <set>
#include <vector>
#include <deque>
#include <stdexcept>
#if LL_WINDOWS
	#pragma warning (push)
	#pragma warning (disable : 4263)
	#pragma warning (disable : 4264)
#endif
#include <boost/signals2.hpp>
#if LL_WINDOWS
	#pragma warning (pop)
#endif
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility.hpp>
#include <boost/optional/optional.hpp>
#include <boost/visit_each.hpp>
#include <boost/ref.hpp>
#include <boost/type_traits/is_pointer.hpp>
#ifndef BOOST_FUNCTION_HPP_INCLUDED
#include <boost/function.hpp>
#define BOOST_FUNCTION_HPP_INCLUDED
#endif
#include <boost/static_assert.hpp>
#include "llsd.h"
#include "llsingleton.h"
#include "lldependencies.h"
#include "llstl.h"
#ifndef testable
#define testable private
#endif
struct LLStopWhenHandled
{
    typedef bool result_type;
    template<typename InputIterator>
    result_type operator()(InputIterator first, InputIterator last) const
    {
        for (InputIterator si = first; si != last; ++si)
		{
            if (*si)
			{
                return true;
			}
		}
        return false;
    }
};
typedef boost::signals2::signal<bool(const LLSD&), LLStopWhenHandled, float>  LLStandardSignal;
typedef LLStandardSignal::slot_type LLEventListener;
typedef boost::signals2::connection LLBoundListener;
typedef boost::signals2::scoped_connection LLTempBoundListener;
class LL_COMMON_API LLListenerOrPumpName
{
public:
    LLListenerOrPumpName(const std::string& pumpname);
    LLListenerOrPumpName(const char* pumpname);
    template<typename T>
    LLListenerOrPumpName(const T& listener): mListener(listener) {}
    LLListenerOrPumpName() {}
    operator bool() const { return bool(mListener); }
    bool operator! () const { return ! mListener; }
    const ::LLEventListener& getListener() const { return *mListener; }
    operator ::LLEventListener() const { return *mListener; }
    bool operator()(const LLSD& event) const;
    struct Empty: public std::runtime_error
    {
        Empty(const std::string& what):
            std::runtime_error(std::string("LLListenerOrPumpName::Empty: ") + what) {}
    };
private:
    boost::optional<LLEventListener> mListener;
};
class LLEventPump;
class LL_COMMON_API LLEventPumps: public LLSingleton<LLEventPumps>
{
    friend class LLSingleton<LLEventPumps>;
public:
    LLEventPump& obtain(const std::string& name);
    void flush();
    void reset();
private:
    friend class LLEventPump;
    std::string registerNew(const LLEventPump&, const std::string& name, bool tweak);
    void unregister(const LLEventPump&);
    static bool sDeleted;
    static void maybe_unregister(const LLEventPump&);
private:
    LLEventPumps();
    ~LLEventPumps();
testable:
    typedef std::map<std::string, LLEventPump*> PumpMap;
    PumpMap mPumpMap;
    typedef std::set<LLEventPump*> PumpSet;
    PumpSet mOurPumps;
    typedef std::set<std::string> PumpNames;
    PumpNames mQueueNames;
};
namespace LLEventDetail
{
    typedef boost::function<LLBoundListener(const ::LLEventListener&)> ConnectFunc;
    template <typename LISTENER>
    LLBoundListener visit_and_connect(const std::string& name,
                                      const LISTENER& listener,
                                      const ConnectFunc& connect_func);
    template <typename LISTENER>
    LLBoundListener visit_and_connect(const LISTENER& listener,
                                      const ConnectFunc& connect_func)
    {
        return visit_and_connect("", listener, connect_func);
    }
}
typedef boost::signals2::trackable LLEventTrackable;
class LL_COMMON_API LLEventPump: public LLEventTrackable
{
public:
    struct DupPumpName: public std::runtime_error
    {
        DupPumpName(const std::string& what):
            std::runtime_error(std::string("DupPumpName: ") + what) {}
    };
    LLEventPump(const std::string& name, bool tweak=false);
    virtual ~LLEventPump();
    struct ListenError: public std::runtime_error
    {
        ListenError(const std::string& what): std::runtime_error(what) {}
    };
    struct DupListenerName: public ListenError
    {
        DupListenerName(const std::string& what):
            ListenError(std::string("DupListenerName: ") + what)
        {}
    };
    struct Cycle: public ListenError
    {
        Cycle(const std::string& what): ListenError(std::string("Cycle: ") + what) {}
    };
    struct OrderChange: public ListenError
    {
        OrderChange(const std::string& what): ListenError(std::string("OrderChange: ") + what) {}
    };
    typedef std::vector<std::string> NameList;
    const static NameList empty;
    std::string getName() const { return mName; }
    template <typename LISTENER>
    LLBoundListener listen(const std::string& name, const LISTENER& listener,
                           const NameList& after=NameList(),
                           const NameList& before=NameList())
    {
        return LLEventDetail::visit_and_connect(name,
                                                listener,
                                                boost::bind(&LLEventPump::listen_impl,
                                                            this,
                                                            name,
                                                            _1,
                                                            after,
                                                            before));
    }
    virtual LLBoundListener getListener(const std::string& name) const;
    typedef boost::signals2::shared_connection_block Blocker;
    virtual void stopListening(const std::string& name);
    virtual bool post(const LLSD&) = 0;
    virtual void enable(bool enabled=true) { mEnabled = enabled; }
    virtual bool enabled() const { return mEnabled; }
    static std::string inventName(const std::string& pfx="listener");
private:
    friend class LLEventPumps;
    virtual void flush() {}
    virtual void reset();
private:
    virtual LLBoundListener listen_impl(const std::string& name, const ::LLEventListener&,
                                        const NameList& after,
                                        const NameList& before);
    std::string mName;
protected:
    boost::shared_ptr<LLStandardSignal> mSignal;
    bool mEnabled;
    typedef std::map<std::string, boost::signals2::connection> ConnectionMap;
    ConnectionMap mConnections;
    typedef LLDependencies<std::string, float> DependencyMap;
    DependencyMap mDeps;
};
class LL_COMMON_API LLEventStream: public LLEventPump
{
public:
    LLEventStream(const std::string& name, bool tweak=false): LLEventPump(name, tweak) {}
    virtual ~LLEventStream() {}
    virtual bool post(const LLSD& event);
};
class LL_COMMON_API LLEventQueue: public LLEventPump
{
public:
    LLEventQueue(const std::string& name, bool tweak=false): LLEventPump(name, tweak) {}
    virtual ~LLEventQueue() {}
    virtual bool post(const LLSD& event);
private:
    virtual void flush();
private:
    typedef std::deque<LLSD> EventQueue;
    EventQueue mEventQueue;
};
class LL_COMMON_API LLReqID
{
public:
    LLReqID(const LLSD& request):
        mReqid(request["reqid"])
    {}
    LLReqID() {}
    void setFrom(const LLSD& request)
    {
        mReqid = request["reqid"];
    }
    void stamp(LLSD& response) const;
    LLSD makeResponse() const
    {
        LLSD response;
        stamp(response);
        return response;
    }
    LLSD getReqID() const { return mReqid; }
private:
    LLSD mReqid;
};
LL_COMMON_API bool sendReply(const LLSD& reply, const LLSD& request,
                             const std::string& replyKey="reply");
class LL_COMMON_API LLListenerWrapperBase
{
public:
    LLListenerWrapperBase():
        mName(new std::string),
        mConnection(new LLBoundListener)
    {
    }
    LLListenerWrapperBase(const LLListenerWrapperBase& that):
        mName(that.mName),
        mConnection(that.mConnection)
    {
    }
	virtual ~LLListenerWrapperBase() {}
    virtual void accept_name(const std::string& name) const
    {
        *mName = name;
    }
    virtual void accept_connection(const LLBoundListener& connection) const
    {
        *mConnection = connection;
    }
protected:
    boost::shared_ptr<std::string> mName;
    boost::shared_ptr<LLBoundListener> mConnection;
};
namespace LLEventDetail
{
    template <typename F>
    const F& unwrap(const F& f) { return f; }
    template <typename F>
    const F& unwrap(const boost::reference_wrapper<F>& f) { return f.get(); }
    template<bool Cond> struct truth {};
    class Visitor
    {
    public:
        Visitor(::LLEventListener& listener):
            mListener(listener)
        {
        }
        template <typename T>
        void operator()(const T& t) const
        {
            decode(t, 0);
        }
    private:
        template<typename T>
        void decode(const boost::reference_wrapper<T>& t, int) const
        {
        }
        template<typename T>
        void decode(const T& t, long) const
        {
            typedef truth<(boost::is_pointer<T>::value)> is_a_pointer;
            maybe_get_pointer(t, is_a_pointer());
        }
        template<typename T>
        void maybe_get_pointer(const T& t, truth<true>) const
        {
        }
        template<typename T>
        void maybe_get_pointer(const boost::shared_ptr<T>& t, truth<false>) const
        {
            BOOST_STATIC_ASSERT(sizeof(T) == 0);
        }
        template<typename T>
        void maybe_get_pointer(const boost::weak_ptr<T>& t, truth<false>) const
        {
            mListener.track(t);
        }
#if 0
        template <typename T>
        inline void maybe_get_pointer(const boost::enable_shared_from_this<T>& ct,
                                      truth<false>) const
        {
            boost::enable_shared_from_this<T>&
                t(const_cast<boost::enable_shared_from_this<T>&>(ct));
            std::cout << "Capturing shared_from_this()" << std::endl;
            boost::shared_ptr<T> sp(t.shared_from_this());
            std::cout << "Tracking shared__ptr" << std::endl;
            mListener.track(sp);
        }
#endif
        template<typename T>
        void maybe_get_pointer(const T& t, truth<false>) const
        {
        }
        ::LLEventListener& mListener;
    };
    template <typename LISTENER>
    LLBoundListener visit_and_connect(const std::string& name,
                                      const LISTENER& raw_listener,
                                      const ConnectFunc& connect_func)
    {
        ::LLEventListener listener(raw_listener);
        LLEventDetail::Visitor visitor(listener);
        using boost::visit_each;
        visit_each(visitor, LLEventDetail::unwrap(raw_listener));
        LLBoundListener connection(connect_func(listener));
        const LLListenerWrapperBase* lwb =
            ll_template_cast<const LLListenerWrapperBase*>(&raw_listener);
        if (lwb)
        {
            lwb->accept_name(name);
            lwb->accept_connection(connection);
        }
        return connection;
    }
}
namespace boost
{
    template <typename T>
    T* get_pointer(const weak_ptr<T>& ptr) { return shared_ptr<T>(ptr).get(); }
}
template <typename T>
boost::weak_ptr<T> weaken(const boost::shared_ptr<T>& ptr)
{
    return boost::weak_ptr<T>(ptr);
}
#endif
