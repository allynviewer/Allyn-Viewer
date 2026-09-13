/**
 * @file   lleventfilter.h
 * @author Nat Goodspeed
 * @date   2009-03-05
 * @brief  Define LLEventFilter: LLEventStream subclass with conditions
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
#if ! defined(LL_LLEVENTFILTER_H)
#define LL_LLEVENTFILTER_H
#include "llevents.h"
#include "stdtypes.h"
#include "lltimer.h"
class LL_COMMON_API LLEventFilter: public LLEventStream
{
public:
    LLEventFilter(const std::string& name="filter", bool tweak=true):
        LLEventStream(name, tweak)
    {}
    LLEventFilter(LLEventPump& source, const std::string& name="filter", bool tweak=true);
    virtual bool post(const LLSD& event) = 0;
private:
    LLTempBoundListener mSource;
};
class LLEventMatching: public LLEventFilter
{
public:
    LLEventMatching(const LLSD& pattern);
    LLEventMatching(LLEventPump& source, const LLSD& pattern);
    virtual bool post(const LLSD& event);
private:
    LLSD mPattern;
};
class LL_COMMON_API LLEventTimeoutBase: public LLEventFilter
{
public:
    LLEventTimeoutBase();
    LLEventTimeoutBase(LLEventPump& source);
    typedef std::function<void()> Action;
    void actionAfter(F32 seconds, const Action& action);
    void errorAfter(F32 seconds, const std::string& message);
    void eventAfter(F32 seconds, const LLSD& event);
    virtual bool post(const LLSD& event);
    void cancel();
protected:
    virtual void setCountdown(F32 seconds) = 0;
    virtual bool countdownElapsed() const = 0;
private:
    bool tick(const LLSD&);
    LLBoundListener mMainloop;
    Action mAction;
};
class LL_COMMON_API LLEventTimeout: public LLEventTimeoutBase
{
public:
    LLEventTimeout();
    LLEventTimeout(LLEventPump& source);
protected:
    virtual void setCountdown(F32 seconds);
    virtual bool countdownElapsed() const;
private:
    LLTimer mTimer;
};
#endif
