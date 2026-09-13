/** 
 * @file llavatarnamecache.h
 * @brief Provides lookup of avatar SLIDs ("bobsmith123") and display names
 * ("James Cook") from avatar UUIDs.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#ifndef LLAVATARNAMECACHE_H
#define LLAVATARNAMECACHE_H
#include "llavatarname.h"
#include <boost/signals2.hpp>
class AIHTTPReceivedHeaders;
class LLUUID;
namespace LLAvatarNameCache
{
	typedef boost::signals2::signal<void (void)> use_display_name_signal_t;
	void initClass(bool running, bool usePeopleAPI);
	void cleanupClass();
	bool importFile(std::istream& istr);
	void exportFile(std::ostream& ostr);
	void setNameLookupURL(const std::string& name_lookup_url);
	bool hasNameLookupURL();
	bool usePeopleAPI();
	void idle();
	bool get(const LLUUID& agent_id, LLAvatarName *av_name);
	bool getNSName(const LLUUID& agent_id, std::string& name, const S32& name_system = main_name_system());
	typedef boost::signals2::signal<
		void (const LLUUID& agent_id, const LLAvatarName& av_name)>
			callback_signal_t;
	typedef callback_signal_t::slot_type callback_slot_t;
	typedef boost::signals2::connection callback_connection_t;
	callback_connection_t get(const LLUUID& agent_id, callback_slot_t slot);
	void setUseDisplayNames(bool use);
	bool getForceDisplayNames();
	void setForceDisplayNames(bool force);
	void setUseUsernames(bool use);
	void insert(const LLUUID& agent_id, const LLAvatarName& av_name);
	void erase(const LLUUID& agent_id);
	void handleAgentError(const LLUUID& agent_id);
	F64 nameExpirationFromHeaders(const AIHTTPReceivedHeaders& headers);
	void addUseDisplayNamesCallback(const use_display_name_signal_t::slot_type& cb);
}
bool max_age_from_cache_control(const std::string& cache_control, S32 *max_age);
#endif
