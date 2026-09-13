/** 
 * @file llevent.h
 * @author Tom Yedwab
 * @brief LLEvent and LLEventListener base classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_EVENT_H
#define LL_EVENT_H
#include "llsd.h"
#include "llpointer.h"
#include "llthread.h"
namespace LLOldEvents
{
class LLEventListener;
class LLEvent;
class LLEventDispatcher;
class LLObservable;
class LL_COMMON_API LLEvent : public LLThreadSafeRefCount
{
protected:
	virtual ~LLEvent();
public:
	LLEvent(LLObservable* source, const std::string& desc = "") : mSource(source), mDesc(desc) { }
	LLObservable* getSource() { return mSource; }
	virtual LLSD		getValue() { return LLSD(); }
	virtual bool accept(LLEventListener* listener);
	virtual const std::string& desc();
private:
	LLObservable* mSource;
	std::string mDesc;
};
class LL_COMMON_API LLEventListener : public LLThreadSafeRefCount
{
protected:
	virtual ~LLEventListener();
public:
	virtual bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata) = 0;
	virtual bool handleAttach(LLEventDispatcher *dispatcher) = 0;
	virtual bool handleDetach(LLEventDispatcher *dispatcher) = 0;
};
class LL_COMMON_API LLSimpleListener : public LLEventListener
{
public:
	void clearDispatchers();
	virtual bool handleAttach(LLEventDispatcher *dispatcher);
	virtual bool handleDetach(LLEventDispatcher *dispatcher);
protected:
	~LLSimpleListener();
	std::vector<LLEventDispatcher *> mDispatchers;
};
class LLObservable;
struct LLListenerEntry
{
	LLEventListener* listener;
	LLSD filter;
	LLSD userdata;
};
class LL_COMMON_API LLEventDispatcher : public LLThreadSafeRefCount
{
protected:
	virtual ~LLEventDispatcher();
public:
	LLEventDispatcher();
	bool engage(LLObservable* observable);
	void disengage(LLObservable* observable);
	void addListener(LLEventListener *listener, LLSD filter, const LLSD& userdata);
	void removeListener(LLEventListener *listener);
	std::vector<LLListenerEntry> getListeners() const;
	bool fireEvent(LLPointer<LLEvent> event, LLSD filter);
public:
	class Impl;
private:
	Impl* impl;
};
class LL_COMMON_API LLObservable
{
public:
	LLObservable();
	virtual ~LLObservable();
	virtual bool setDispatcher(LLPointer<LLEventDispatcher> dispatcher);
	virtual LLEventDispatcher* getDispatcher();
	void addListener(LLEventListener *listener, LLSD filter = "", const LLSD& userdata = "")
	{
		if (mDispatcher.notNull()) mDispatcher->addListener(listener, filter, userdata);
	}
	void removeListener(LLEventListener *listener)
	{
		if (mDispatcher.notNull()) mDispatcher->removeListener(listener);
	}
	void fireEvent(LLPointer<LLEvent> event, LLSD filter = LLSD());
protected:
	LLPointer<LLEventDispatcher> mDispatcher;
};
class LLValueChangedEvent : public LLEvent
{
public:
	LLValueChangedEvent(LLObservable* source, LLSD value) : LLEvent(source, "value_changed"), mValue(value) { }
	LLSD getValue() { return mValue; }
	LLSD mValue;
};
}
#endif
