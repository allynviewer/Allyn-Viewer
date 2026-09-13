/**
 * @file llavataractions.h
 * @brief Friend-related actions (add, remove, offer teleport, etc)
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
#ifndef LL_LLAVATARACTIONS_H
#define LL_LLAVATARACTIONS_H
#include <boost/unordered_set.hpp>
class LLAvatarName;
class LLInventoryPanel;
class LLFloater;
class LLView;
class LLAvatarActions
{
public:
	static void requestFriendshipDialog(const LLUUID& id, const std::string& name);
	static void requestFriendshipDialog(const LLUUID& id);
	static void removeFriendDialog(const LLUUID& id);
	static void removeFriendsDialog(const uuid_vec_t& ids);
	static void offerTeleport(const LLUUID& invitee);
	static void offerTeleport(const uuid_vec_t& ids);
	static void startIM(const LLUUID& id);
	static void endIM(const LLUUID& id);
	static void startCall(const LLUUID& id);
	static void startAdhocCall(const uuid_vec_t& ids);
	static void startConference(const uuid_vec_t& ids);
	static void showProfile(const LLUUID& id, bool web = false);
	static void showProfiles(const uuid_vec_t& ids, bool web = false);
	static void hideProfile(const LLUUID& id);
	static bool profileVisible(const LLUUID& id);
	static LLFloater* getProfileFloater(const LLUUID& id);
	static void showOnMap(const LLUUID& id);
	static void pay(const LLUUID& id);
	static void teleportRequest(const LLUUID& id);
	static void teleport_request_callback(const LLSD& notification, const LLSD& response);
	static void share(const LLUUID& id);
	static void shareWithAvatars(LLView * panel);
	static void toggleBlock(const LLUUID& id);
	static void toggleMuteVoice(const LLUUID& id);
	static bool isFriend(const LLUUID& id);
	static bool isBlocked(const LLUUID& id);
	static bool isVoiceMuted(const LLUUID& id);
	static bool canBlock(const LLUUID& id);
	static bool canCall();
	static void inviteToGroup(const LLUUID& id);
	static void inviteToGroup(const uuid_vec_t& ids);
	static void kick(const LLUUID& id);
	static void freeze(const LLUUID& id);
	static void unfreeze(const LLUUID& id);
	static void csr(const LLUUID& id);
	static bool canOfferTeleport(const LLUUID& id);
	static bool canOfferTeleport(const uuid_vec_t& ids);
	static bool canShareSelectedItems(LLInventoryPanel* inv_panel = NULL);
	static bool isAgentMappable(const LLUUID& agent_id);
	static void buildResidentsString(std::vector<LLAvatarName> avatar_names, std::string& residents_string);
	static void buildResidentsString(const uuid_vec_t& avatar_uuids, std::string& residents_string);
	static uuid_set_t getInventorySelectedUUIDs();
	static void copyUUIDs(const uuid_vec_t& id);
	static std::string getSLURL(const LLUUID& id);
private:
	static bool callbackAddFriendWithMessage(const LLSD& notification, const LLSD& response);
	static bool handleRemove(const LLSD& notification, const LLSD& response);
	static bool handlePay(const LLSD& notification, const LLSD& response, LLUUID avatar_id);
	static bool handleKick(const LLSD& notification, const LLSD& response);
	static bool handleFreeze(const LLSD& notification, const LLSD& response);
	static bool handleUnfreeze(const LLSD& notification, const LLSD& response);
	static void callback_invite_to_group(LLUUID group_id, uuid_vec_t& ids);
	static void on_avatar_name_cache_teleport_request(const LLUUID& id, const LLAvatarName& av_name);
public:
	static void requestFriendship(const LLUUID& target_id, const std::string& target_name, const std::string& message);
};
#endif
