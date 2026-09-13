/**
 * @file   lleventdispatcher.cpp
 * @author Nat Goodspeed
 * @date   2009-06-18
 * @brief  Implementation for lleventdispatcher.
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
#if LL_WINDOWS
#pragma warning (disable : 4355)
#endif
#include "linden_common.h"
#include "lleventdispatcher.h"
#include "llevents.h"
#include "llerror.h"
#include "llsdutil.h"
#include "stringize.h"
#include <memory>
class LL_COMMON_API LLSDArgsSource
{
public:
    LLSDArgsSource(const std::string function, const LLSD& args);
    ~LLSDArgsSource();
    LLSD next();
    void done() const;
private:
    std::string _function;
    LLSD _args;
    LLSD::Integer _index;
};
LLSDArgsSource::LLSDArgsSource(const std::string function, const LLSD& args):
    _function(function),
    _args(args),
    _index(0)
{
    if (! (_args.isUndefined() || _args.isArray()))
    {
        LL_ERRS("LLSDArgsSource") << _function << " needs an args array instead of "
                                  << _args << LL_ENDL;
    }
}
LLSDArgsSource::~LLSDArgsSource()
{
    done();
}
LLSD LLSDArgsSource::next()
{
    if (_index >= _args.size())
    {
        LL_ERRS("LLSDArgsSource") << _function << " requires more arguments than the "
                                  << _args.size() << " provided: " << _args << LL_ENDL;
    }
    return _args[_index++];
}
void LLSDArgsSource::done() const
{
    if (_index < _args.size())
    {
        LL_WARNS("LLSDArgsSource") << _function << " only consumed " << _index
                                   << " of the " << _args.size() << " arguments provided: "
                                   << _args << LL_ENDL;
    }
}
class LL_COMMON_API LLSDArgsMapper
{
public:
    LLSDArgsMapper(const std::string& function, const LLSD& names, const LLSD& defaults);
    LLSD map(const LLSD& argsmap) const;
private:
    static std::string formatlist(const LLSD&);
    std::string _function;
    LLSD _names;
    typedef std::map<LLSD::String, LLSD::Integer> IndexMap;
    IndexMap _indexes;
    LLSD _defaults;
    typedef std::vector<char> FilledVector;
    FilledVector _has_dft;
};
LLSDArgsMapper::LLSDArgsMapper(const std::string& function,
                               const LLSD& names, const LLSD& defaults):
    _function(function),
    _names(names),
    _has_dft(names.size())
{
    if (! (_names.isUndefined() || _names.isArray()))
    {
        LL_ERRS("LLSDArgsMapper") << function << " names must be an array, not " << names << LL_ENDL;
    }
    LLSD::Integer nparams(_names.size());
    for (LLSD::Integer ni = 0, nend = _names.size(); ni < nend; ++ni)
    {
        _indexes[_names[ni]] = ni;
    }
    if (nparams)
    {
        _defaults[nparams - 1] = LLSD();
    }
    if (defaults.isUndefined() || defaults.isArray())
    {
        LLSD::Integer ndefaults = defaults.size();
        if (ndefaults > nparams)
        {
            LL_ERRS("LLSDArgsMapper") << function << " names array " << names
                                      << " shorter than defaults array " << defaults << LL_ENDL;
        }
        LLSD::Integer offset = nparams - ndefaults;
        for (LLSD::Integer i = 0, iend = ndefaults; i < iend; ++i)
        {
            _defaults[i + offset] = defaults[i];
            _has_dft[i + offset] = 1;
        }
    }
    else if (defaults.isMap())
    {
        LLSD bogus;
        for (LLSD::map_const_iterator mi(defaults.beginMap()), mend(defaults.endMap());
             mi != mend; ++mi)
        {
            IndexMap::const_iterator ixit(_indexes.find(mi->first));
            if (ixit == _indexes.end())
            {
                bogus.append(mi->first);
                continue;
            }
            LLSD::Integer pos = ixit->second;
            _defaults[pos] = mi->second;
            _has_dft[pos] = 1;
        }
        if (bogus.size())
        {
            LL_ERRS("LLSDArgsMapper") << function << " defaults specified for nonexistent params "
                                      << formatlist(bogus) << LL_ENDL;
        }
    }
    else
    {
        LL_ERRS("LLSDArgsMapper") << function << " defaults must be a map or an array, not "
                                  << defaults << LL_ENDL;
    }
}
LLSD LLSDArgsMapper::map(const LLSD& argsmap) const
{
    if (! (argsmap.isUndefined() || argsmap.isMap() || argsmap.isArray()))
    {
        LL_ERRS("LLSDArgsMapper") << _function << " map() needs a map or array, not "
                                  << argsmap << LL_ENDL;
    }
    LLSD args(LLSD::emptyArray());
    if (_defaults.size() == 0)
    {
        return args;
    }
    args[_defaults.size() - 1] = LLSD();
    FilledVector filled(args.size());
    if (argsmap.isArray())
    {
        LLSD::Integer size(argsmap.size());
        if (size > args.size())
        {
            LL_WARNS("LLSDArgsMapper") << _function << " needs " << args.size()
                                       << " params, ignoring last " << (size - args.size())
                                       << " of passed " << size << ": " << argsmap << LL_ENDL;
            size = args.size();
        }
        for (LLSD::Integer i(0); i < size; ++i)
        {
            args[i] = argsmap[i];
            filled[i] = 1;
        }
    }
    else
    {
        for (LLSD::map_const_iterator mi(argsmap.beginMap()), mend(argsmap.endMap());
             mi != mend; ++mi)
        {
            IndexMap::const_iterator ixit(_indexes.find(mi->first));
            if (ixit == _indexes.end())
            {
                LL_DEBUGS("LLSDArgsMapper") << _function << " ignoring "
                                            << mi->first << "=" << mi->second << LL_ENDL;
                continue;
            }
            LLSD::Integer pos = ixit->second;
            args[pos] = mi->second;
            filled[pos] = 1;
        }
    }
    LLSD unfilled(LLSD::emptyArray());
    for (LLSD::Integer i = 0, iend = args.size(); i < iend; ++i)
    {
        if (! filled[i])
        {
            if (! _has_dft[i])
            {
                unfilled.append(_names[i]);
            }
            else
            {
                args[i] = _defaults[i];
            }
        }
    }
    if (unfilled.size())
    {
        LL_ERRS("LLSDArgsMapper") << _function << " missing required arguments "
                                  << formatlist(unfilled) << " from " << argsmap << LL_ENDL;
    }
    return args;
}
std::string LLSDArgsMapper::formatlist(const LLSD& list)
{
    std::ostringstream out;
    const char* delim = "";
    for (auto const& entry : list.array())
    {
        out << delim << entry.asString();
        delim = ", ";
    }
    return out.str();
}
LLEventDispatcher::LLEventDispatcher(const std::string& desc, const std::string& key):
    mDesc(desc),
    mKey(key)
{
}
LLEventDispatcher::~LLEventDispatcher()
{
}
struct LLEventDispatcher::LLSDDispatchEntry: public LLEventDispatcher::DispatchEntry
{
    LLSDDispatchEntry(const std::string& desc, const Callable& func, const LLSD& required):
        DispatchEntry(desc),
        mFunc(func),
        mRequired(required)
    {}
    Callable mFunc;
    LLSD mRequired;
    virtual void call(const std::string& desc, const LLSD& event) const
    {
        std::string mismatch(llsd_matches(mRequired, event));
        if (! mismatch.empty())
        {
            LL_ERRS("LLEventDispatcher") << desc << ": bad request: " << mismatch << LL_ENDL;
        }
        mFunc(event);
    }
    virtual LLSD addMetadata(LLSD meta) const
    {
        meta["required"] = mRequired;
        return meta;
    }
};
struct LLEventDispatcher::ParamsDispatchEntry: public LLEventDispatcher::DispatchEntry
{
    ParamsDispatchEntry(const std::string& desc, const invoker_function& func):
        DispatchEntry(desc),
        mInvoker(func)
    {}
    invoker_function mInvoker;
    virtual void call(const std::string& desc, const LLSD& event) const
    {
        LLSDArgsSource src(desc, event);
        mInvoker(std::bind(&LLSDArgsSource::next, std::ref(src)));
    }
};
struct LLEventDispatcher::ArrayParamsDispatchEntry: public LLEventDispatcher::ParamsDispatchEntry
{
    ArrayParamsDispatchEntry(const std::string& desc, const invoker_function& func,
                             LLSD::Integer arity):
        ParamsDispatchEntry(desc, func),
        mArity(arity)
    {}
    LLSD::Integer mArity;
    virtual LLSD addMetadata(LLSD meta) const
    {
        LLSD array(LLSD::emptyArray());
        if (mArity)
            array[mArity - 1] = LLSD();
        llassert_always(array.size() == mArity);
        meta["required"] = array;
        return meta;
    }
};
struct LLEventDispatcher::MapParamsDispatchEntry: public LLEventDispatcher::ParamsDispatchEntry
{
    MapParamsDispatchEntry(const std::string& name, const std::string& desc,
                           const invoker_function& func,
                           const LLSD& params, const LLSD& defaults):
        ParamsDispatchEntry(desc, func),
        mMapper(name, params, defaults),
        mRequired(LLSD::emptyMap())
    {
        for (auto const& entry : params.array())
        {
            mRequired[entry.asString()] = LLSD();
        }
        if (defaults.isArray() || defaults.isUndefined())
        {
            LLSD::Integer offset = params.size() - defaults.size();
            for (LLSD::Integer i(0), iend(defaults.size()); i < iend; ++i)
            {
                mRequired.erase(params[i + offset].asString());
                mOptional[params[i + offset].asString()] = defaults[i];
            }
        }
        else if (defaults.isMap())
        {
            mOptional = defaults;
            for (LLSD::map_const_iterator mi(mOptional.beginMap()), mend(mOptional.endMap());
                 mi != mend; ++mi)
            {
                mRequired.erase(mi->first);
            }
        }
    }
    LLSDArgsMapper mMapper;
    LLSD mRequired;
    LLSD mOptional;
    virtual void call(const std::string& desc, const LLSD& event) const
    {
        ParamsDispatchEntry::call(desc, mMapper.map(event));
    }
    virtual LLSD addMetadata(LLSD meta) const
    {
        meta["required"] = mRequired;
        meta["optional"] = mOptional;
        return meta;
    }
};
void LLEventDispatcher::addArrayParamsDispatchEntry(const std::string& name,
                                                    const std::string& desc,
                                                    const invoker_function& invoker,
                                                    LLSD::Integer arity)
{
    mDispatch.insert(
        DispatchMap::value_type(name, DispatchMap::mapped_type(
                                    new ArrayParamsDispatchEntry(desc, invoker, arity))));
}
void LLEventDispatcher::addMapParamsDispatchEntry(const std::string& name,
                                                  const std::string& desc,
                                                  const invoker_function& invoker,
                                                  const LLSD& params,
                                                  const LLSD& defaults)
{
    mDispatch.insert(
        DispatchMap::value_type(name, DispatchMap::mapped_type(
                                    new MapParamsDispatchEntry(name, desc, invoker, params, defaults))));
}
void LLEventDispatcher::add(const std::string& name, const std::string& desc,
                            const Callable& callable, const LLSD& required)
{
    mDispatch.insert(
        DispatchMap::value_type(name, DispatchMap::mapped_type(
                                    new LLSDDispatchEntry(desc, callable, required))));
}
void LLEventDispatcher::addFail(const std::string& name, const std::string& classname) const
{
    LL_ERRS("LLEventDispatcher") << "LLEventDispatcher(" << mDesc << ")::add(" << name
                                 << "): " << classname << " is not a subclass "
                                 << "of LLEventDispatcher" << LL_ENDL;
}
bool LLEventDispatcher::remove(const std::string& name)
{
    DispatchMap::iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        return false;
    }
    mDispatch.erase(found);
    return true;
}
void LLEventDispatcher::operator()(const std::string& name, const LLSD& event) const
{
    if (! try_call(name, event))
    {
        LL_ERRS("LLEventDispatcher") << "LLEventDispatcher(" << mDesc << "): '" << name
                                     << "' not found" << LL_ENDL;
    }
}
void LLEventDispatcher::operator()(const LLSD& event) const
{
    std::string name(event[mKey]);
    if (! try_call(name, event))
    {
        LL_ERRS("LLEventDispatcher") << "LLEventDispatcher(" << mDesc << "): bad " << mKey
                                     << " value '" << name << "'" << LL_ENDL;
    }
}
bool LLEventDispatcher::try_call(const LLSD& event) const
{
    return try_call(event[mKey], event);
}
bool LLEventDispatcher::try_call(const std::string& name, const LLSD& event) const
{
    DispatchMap::const_iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        return false;
    }
    found->second->call(STRINGIZE("LLEventDispatcher(" << mDesc << ") calling '" << name << "'"),
                        event);
    return true;
}
LLSD LLEventDispatcher::getMetadata(const std::string& name) const
{
    DispatchMap::const_iterator found = mDispatch.find(name);
    if (found == mDispatch.end())
    {
        return LLSD();
    }
    LLSD meta;
    meta["name"] = name;
    meta["desc"] = found->second->mDesc;
    return found->second->addMetadata(meta);
}
LLDispatchListener::LLDispatchListener(const std::string& pumpname, const std::string& key):
    LLEventDispatcher(pumpname, key),
    mPump(pumpname, true),
    mBoundListener(mPump.listen("self", boost::bind(&LLDispatchListener::process, this, _1)))
{
}
bool LLDispatchListener::process(const LLSD& event)
{
    (*this)(event);
    return false;
}
LLEventDispatcher::DispatchEntry::DispatchEntry(const std::string& desc):
    mDesc(desc)
{}
