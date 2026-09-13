/**
 * @file   llevents.cpp
 * @author Nat Goodspeed
 * @date   2008-09-12
 * @brief  Implementation for llevents.
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
#include "linden_common.h"
#if LL_WINDOWS
#pragma warning (disable : 4675)
#endif
#include "llevents.h"
#include <set>
#include <sstream>
#include <algorithm>
#include <typeinfo>
#include <cassert>
#include <cmath>
#include <cctype>
#include <boost/range/iterator_range.hpp>
#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable : 4701)
#endif
#include <boost/lexical_cast.hpp>
#if LL_WINDOWS
#pragma warning (pop)
#endif
#include "stringize.h"
#include "llerror.h"
#include "llsdutil.h"
#if LL_MSVC
#pragma warning (disable : 4702)
#endif
const char* queue_names[] =
{
    "placeholder - replace with first real name string"
};
struct RegisterFlush : public LLEventTrackable
{
    RegisterFlush():
        pumps(LLEventPumps::instance())
    {
        pumps.obtain("mainloop").listen("flushLLEventQueues", boost::bind(&RegisterFlush::flush, this, _1));
    }
    bool flush(const LLSD&)
    {
        pumps.flush();
        return false;
    }
    ~RegisterFlush()
    {
    }
    LLEventPumps& pumps;
};
static RegisterFlush registerFlush;
LLEventPumps::LLEventPumps():
    mQueueNames(boost::begin(queue_names), boost::end(queue_names))
{
}
LLEventPump& LLEventPumps::obtain(const std::string& name)
{
    PumpMap::iterator found = mPumpMap.find(name);
    if (found != mPumpMap.end())
    {
        return *found->second;
    }
    LLEventPump* newInstance;
    PumpNames::const_iterator nfound = mQueueNames.find(name);
    if (nfound != mQueueNames.end())
        newInstance = new LLEventQueue(name);
    else
        newInstance = new LLEventStream(name);
    mOurPumps.insert(newInstance);
    return *newInstance;
}
void LLEventPumps::flush()
{
    for (PumpMap::iterator pmi = mPumpMap.begin(), pmend = mPumpMap.end(); pmi != pmend; ++pmi)
    {
        pmi->second->flush();
    }
}
void LLEventPumps::reset()
{
    for (PumpMap::iterator pmi = mPumpMap.begin(), pmend = mPumpMap.end(); pmi != pmend; ++pmi)
    {
        pmi->second->reset();
    }
}
std::string LLEventPumps::registerNew(const LLEventPump& pump, const std::string& name, bool tweak)
{
    std::pair<PumpMap::iterator, bool> inserted =
        mPumpMap.insert(PumpMap::value_type(name, const_cast<LLEventPump*>(&pump)));
    if (inserted.second)
        return name;
    if (! tweak)
    {
        throw LLEventPump::DupPumpName(std::string("Duplicate LLEventPump name '") + name + "'");
    }
    std::set<int> suffixes;
    PumpMap::iterator pmi(inserted.first), pmend(mPumpMap.end());
    while (++pmi != pmend)
    {
        if (pmi->first.substr(0, name.length()) != name)
        {
            break;
        }
        if (pmi->first[name.length()] > '9')
            break;
        if (! std::isdigit(pmi->first[name.length()]))
            continue;
        try
        {
            suffixes.insert(boost::lexical_cast<int>(pmi->first.substr(name.length())));
        }
        catch (const boost::bad_lexical_cast&)
        {
        }
    }
    int suffix = 1;
    for ( ; suffixes.find(suffix) != suffixes.end(); ++suffix)
        ;
    std::ostringstream out;
    out << name << suffix;
    return registerNew(pump, out.str(), tweak);
}
void LLEventPumps::unregister(const LLEventPump& pump)
{
    PumpMap::iterator found = mPumpMap.find(pump.getName());
    if (found != mPumpMap.end())
    {
        mPumpMap.erase(found);
    }
    PumpSet::iterator psfound = mOurPumps.find(const_cast<LLEventPump*>(&pump));
    if (psfound != mOurPumps.end())
    {
        mOurPumps.erase(psfound);
    }
}
bool LLEventPumps::sDeleted;
void LLEventPumps::maybe_unregister(const LLEventPump& pump)
{
	if (!sDeleted)
	{
		LLEventPumps::instance().unregister(pump);
	}
}
LLEventPumps::~LLEventPumps()
{
    sDeleted = true;
    for (LLEventPumps::PumpSet::iterator pump = mOurPumps.begin(); pump != mOurPumps.end(); ++pump)
    {
        delete *pump;
    }
}
#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable : 4355)
#endif
LLEventPump::LLEventPump(const std::string& name, bool tweak):
    mName(LLEventPumps::instance().registerNew(*this, name, tweak)),
    mSignal(new LLStandardSignal()),
    mEnabled(true)
{}
#if LL_WINDOWS
#pragma warning (pop)
#endif
LLEventPump::~LLEventPump()
{
    LLEventPumps::maybe_unregister(*this);
}
const LLEventPump::NameList LLEventPump::empty;
std::string LLEventPump::inventName(const std::string& pfx)
{
    static long suffix = 0;
    return STRINGIZE(pfx << suffix++);
}
void LLEventPump::reset()
{
    mSignal.reset();
    mConnections.clear();
}
LLBoundListener LLEventPump::listen_impl(const std::string& name, const LLEventListener& listener,
                                         const NameList& after,
                                         const NameList& before)
{
    ConnectionMap::const_iterator found = mConnections.find(name);
    if (found != mConnections.end() && found->second.connected())
    {
        throw DupListenerName(std::string("Attempt to register duplicate listener name '") + name +
                              "' on " + typeid(*this).name() + " '" + getName() + "'");
    }
    DependencyMap::node_type& newNode = mDeps.add(name, -1.0, after, before);
    DependencyMap::sorted_range sorted_range;
    try
    {
        sorted_range = mDeps.sort();
    }
    catch (const DependencyMap::Cycle& e)
    {
        mDeps.remove(name);
        throw Cycle(std::string("New listener '") + name + "' on " + typeid(*this).name() +
                    " '" + getName() + "' would cause cycle: " + e.what());
    }
    float previous = 0.0, myprev = 0.0;
    DependencyMap::sorted_iterator mydmi = sorted_range.end();
    for (DependencyMap::sorted_iterator dmi = sorted_range.begin();
         dmi != sorted_range.end(); ++dmi)
    {
        if (dmi->first == name)
        {
            mydmi = dmi;
            myprev = previous;
            continue;
        }
        if (dmi->second < previous)
        {
            typedef std::vector< std::pair<float, std::string> > SortNameList;
            SortNameList sortnames;
            for (DependencyMap::sorted_iterator cdmi(sorted_range.begin()), cdmend(sorted_range.end());
                 cdmi != cdmend; ++cdmi)
            {
                if (cdmi->first != name)
                {
                    sortnames.push_back(SortNameList::value_type(cdmi->second, cdmi->first));
                }
            }
            std::sort(sortnames.begin(), sortnames.end());
            std::ostringstream out;
            out << "New listener '" << name << "' on " << typeid(*this).name() << " '" << getName()
                << "' would move previous listener '" << dmi->first << "'\nwas: ";
            SortNameList::const_iterator sni(sortnames.begin()), snend(sortnames.end());
            if (sni != snend)
            {
                out << sni->second;
                while (++sni != snend)
                {
                    out << ", " << sni->second;
                }
            }
            out << "\nnow: ";
            DependencyMap::sorted_iterator ddmi(sorted_range.begin()), ddmend(sorted_range.end());
            if (ddmi != ddmend)
            {
                out << ddmi->first;
                while (++ddmi != ddmend)
                {
                    out << ", " << ddmi->first;
                }
            }
            mDeps.remove(name);
            throw OrderChange(out.str());
        }
        previous = dmi->second;
    }
    assert(mydmi != sorted_range.end());
    if (++mydmi != sorted_range.end())
    {
        newNode = (myprev + mydmi->second)/2.f;
    }
    else
    {
        newNode = std::ceil(myprev) + 1.f;
    }
    LLBoundListener bound = mSignal->connect(newNode, listener);
    mConnections[name] = bound;
    return bound;
}
LLBoundListener LLEventPump::getListener(const std::string& name) const
{
    ConnectionMap::const_iterator found = mConnections.find(name);
    if (found != mConnections.end())
    {
        return found->second;
    }
    return LLBoundListener();
}
void LLEventPump::stopListening(const std::string& name)
{
    ConnectionMap::iterator found = mConnections.find(name);
    if (found != mConnections.end())
    {
        found->second.disconnect();
        mConnections.erase(found);
    }
}
bool LLEventStream::post(const LLSD& event)
{
    if (! mEnabled || !mSignal)
    {
        return false;
    }
    boost::shared_ptr<LLStandardSignal> signal(mSignal);
    return (*signal)(event);
}
bool LLEventQueue::post(const LLSD& event)
{
    if (mEnabled)
    {
        mEventQueue.push_back(event);
    }
    return false;
}
void LLEventQueue::flush()
{
	if(!mSignal) return;
    EventQueue queue(mEventQueue);
    mEventQueue.clear();
    boost::shared_ptr<LLStandardSignal> signal(mSignal);
    for ( ; ! queue.empty(); queue.pop_front())
    {
        (*signal)(queue.front());
    }
}
LLListenerOrPumpName::LLListenerOrPumpName(const std::string& pumpname):
    mListener(boost::bind(&LLEventPump::post,
                          boost::ref(LLEventPumps::instance().obtain(pumpname)),
                          _1))
{
}
LLListenerOrPumpName::LLListenerOrPumpName(const char* pumpname):
    mListener(boost::bind(&LLEventPump::post,
                          boost::ref(LLEventPumps::instance().obtain(pumpname)),
                          _1))
{
}
bool LLListenerOrPumpName::operator()(const LLSD& event) const
{
    if (! mListener)
    {
        throw Empty("attempting to call uninitialized");
    }
    return (*mListener)(event);
}
void LLReqID::stamp(LLSD& response) const
{
    if (! (response.isUndefined() || response.isMap()))
    {
        LL_INFOS("LLReqID") << "stamp(" << mReqid << ") leaving non-map response unmodified: "
                            << response << LL_ENDL;
        return;
    }
    LLSD oldReqid(response["reqid"]);
    if (! (oldReqid.isUndefined() || llsd_equals(oldReqid, mReqid)))
    {
        LL_INFOS("LLReqID") << "stamp(" << mReqid << ") preserving existing [\"reqid\"] value "
                            << oldReqid << " in response: " << response << LL_ENDL;
        return;
    }
    response["reqid"] = mReqid;
}
bool sendReply(const LLSD& reply, const LLSD& request, const std::string& replyKey)
{
    if (! request.has(replyKey))
    {
        return false;
    }
    LLSD newreply(reply);
    LLReqID reqID(request);
    reqID.stamp(newreply);
    return LLEventPumps::instance().obtain(request[replyKey]).post(newreply);
}
