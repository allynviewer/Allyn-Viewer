/** 
 * @file lldispatcher.h
 * @brief LLDispatcher class header file.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#ifndef LL_LLDISPATCHER_H
#define LL_LLDISPATCHER_H
#include <map>
#include <vector>
#include <string>
class LLDispatcher;
class LLMessageSystem;
class LLUUID;
class LLDispatchHandler
{
public:
	typedef std::vector<std::string> sparam_t;
	LLDispatchHandler() {}
	virtual ~LLDispatchHandler() {}
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& string) = 0;
};
class LLDispatcher
{
public:
	typedef std::string key_t;
	typedef std::vector<std::string> keys_t;
	typedef std::vector<std::string> sparam_t;
	LLDispatcher();
	virtual ~LLDispatcher();
	bool isHandlerPresent(const key_t& name) const;
	void copyAllHandlerNames(keys_t& names) const;
	bool dispatch(
		const key_t& name,
		const LLUUID& invoice,
		const sparam_t& strings) const;
	LLDispatchHandler* addHandler(const key_t& name, LLDispatchHandler* func);
	static bool unpackMessage(
		LLMessageSystem* msg,
		key_t& method,
		LLUUID& invoice,
		sparam_t& parameters);
    static bool unpackLargeMessage(
        LLMessageSystem* msg,
        key_t& method,
        LLUUID& invoice,
        sparam_t& parameters);
protected:
	typedef std::map<key_t, LLDispatchHandler*> dispatch_map_t;
	dispatch_map_t mHandlers;
};
#endif
