/**
 * @file llgroupactions.h
 * @brief Group-related actions (join, leave, new, delete, etc)
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
#ifndef LL_LLGROUPACTIONS_H
#define LL_LLGROUPACTIONS_H
class LLFloaterGroupInfo;
class LLOfferInfo;
class LLGroupActions
{
public:
	static void search();
	static void join(const LLUUID& group_id);
	static void leave(const LLUUID& group_id);
	static void activate(const LLUUID& group_id);
	static void show(const LLUUID& group_id);
	static void showProfiles(const uuid_vec_t& group_ids);
	static void showTab(const LLUUID& group_id, const std::string& tab_name);
	static void showNotice(const std::string& subj, const std::string& mes, const LLUUID& group_id, const bool& has_inventory, const std::string& item_name, LLOfferInfo* info);
	static void inspect(const LLUUID& group_id);
	static void refresh(const LLUUID& group_id);
	static void refresh_notices();
	static void createGroup();
	static void closeGroup(const LLUUID& group_id);
	static LLUUID startIM(const LLUUID& group_id);
	static void endIM(const LLUUID& group_id);
	static bool isInGroup(const LLUUID& group_id);
	static void startCall(const LLUUID& group_id);
	static bool isAvatarMemberOfGroup(const LLUUID& group_id, const LLUUID& avatar_id);
	static std::string getSLURL(const LLUUID& id);
private:
	static bool onJoinGroup(const LLSD& notification, const LLSD& response);
	static bool onLeaveGroup(const LLSD& notification, const LLSD& response);
	static void processLeaveGroupDataResponse(const LLUUID group_id);
	static LLFloaterGroupInfo* openGroupProfile(const LLUUID& group_id);
	friend class LLFetchLeaveGroupData;
};
#endif
