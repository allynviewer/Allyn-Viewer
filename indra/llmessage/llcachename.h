/** 
 * @file llcachename.h
 * @brief A cache of names from UUIDs.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LL_LLCACHENAME_H
#define LL_LLCACHENAME_H
#include <boost/bind.hpp>
#include <boost/signals2.hpp>
class LLMessageSystem;
class LLHost;
class LLUUID;
typedef boost::signals2::signal<void (const LLUUID& id,
                                      const std::string& name,
                                      bool is_group)> LLCacheNameSignal;
typedef LLCacheNameSignal::slot_type LLCacheNameCallback;
typedef void (*old_callback_t)(const LLUUID&, const std::string&, bool, void*);
class LLCacheName
{
public:
	LLCacheName(LLMessageSystem* msg);
	LLCacheName(LLMessageSystem* msg, const LLHost& upstream_host);
	~LLCacheName();
	const std::map<std::string, LLUUID>& getReverseMap() const;
	void setUpstream(const LLHost& upstream_host);
	boost::signals2::connection addObserver(const LLCacheNameCallback& callback);
	bool importFile(std::istream& istr);
	void exportFile(std::ostream& ostr);
	BOOL getFullName(const LLUUID& id, std::string& full_name);
	BOOL getUUID(const std::string& first, const std::string& last, LLUUID& id);
	BOOL getUUID(const std::string& fullname, LLUUID& id);
	static std::string buildFullName(const std::string& first, const std::string& last);
	static std::string cleanFullName(const std::string& full_name);
	static std::string buildUsername(const std::string& name);
	static std::string buildLegacyName(const std::string& name);
	BOOL getGroupName(const LLUUID& id, std::string& group);
	boost::signals2::connection get(const LLUUID& id, bool is_group, const LLCacheNameCallback& callback);
	bool getIfThere(const LLUUID& id, std::string& fullname, BOOL& is_group);
	boost::signals2::connection getGroup(const LLUUID& group_id, const LLCacheNameCallback& callback);
	boost::signals2::connection get(const LLUUID& id, bool is_group, old_callback_t callback, void* user_data);
	void processPending();
	void deleteEntriesOlderThan(S32 secs);
	void dump();
	void dumpStats();
	void clear();
	static std::string getDefaultName();
	static std::string getDefaultLastName();
	static void localizeCacheName(std::string key, std::string value);
	static std::map<std::string, std::string> sCacheName;
private:
	class Impl;
	Impl& impl;
};
extern LLCacheName* gCacheName;
#endif
