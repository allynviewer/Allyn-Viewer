/**
 * @file llavataractions.cpp
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
#include "llviewerprecompiledheaders.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "roles_constants.h"
#include "llagent.h"
#include "llcallingcard.h"
#include "llfloateravatarinfo.h"
#include "llfloateravatarpicker.h"
#include "llfloaterchatterbox.h"
#include "llfloatergroupbulkban.h"
#include "llfloatergroupinvite.h"
#include "llfloatergroups.h"
#include "llfloaterwebprofile.h"
#include "llfloaterworldmap.h"
#include "llgiveinventory.h"
#include "llgivemoney.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llimview.h"
#include "llmutelist.h"
#include "llpanelprofile.h"
#include "lltrans.h"
#include "llvoiceclient.h"
#include "llweb.h"
#include "llslurl.h"
#include "llpanelmaininventory.h"
#include "llavatarname.h"
#include "llagentui.h"
#include "rlvactions.h"
#include "rlvcommon.h"
#include "llviewerwindow.h"
#include "llwindow.h"
extern const S32 TRANS_GIFT;
void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount, BOOL is_group = FALSE, S32 trx_type = TRANS_GIFT, const std::string& desc = LLStringUtil::null);
void handle_lure(const uuid_vec_t& ids);
void send_improved_im(const LLUUID& to_id, const std::string& name, const std::string& message, U8 offline, EInstantMessage dialog, const LLUUID& id, U32 timestamp = NO_TIMESTAMP, const U8* binary_bucket = (U8*)EMPTY_BINARY_BUCKET, S32 binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE);
void LLAvatarActions::requestFriendshipDialog(const LLUUID& id, const std::string& name)
{
	if(id == gAgentID)
	{
		LLNotificationsUtil::add("AddSelfFriend");
		return;
	}
	LLSD args;
	args["NAME"] = name;
	LLSD payload;
	payload["id"] = id;
	payload["name"] = name;
	LLNotificationsUtil::add("AddFriendWithMessage", args, payload, &callbackAddFriendWithMessage);
}
void on_avatar_name_friendship(const LLUUID& id, const LLAvatarName av_name)
{
	LLAvatarActions::requestFriendshipDialog(id, av_name.getCompleteName());
}
void LLAvatarActions::requestFriendshipDialog(const LLUUID& id)
{
	if(id.isNull())
	{
		return;
	}
	LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_friendship, _1, _2));
}
void LLAvatarActions::removeFriendDialog(const LLUUID& id)
{
	if (id.isNull())
		return;
	uuid_vec_t ids;
	ids.push_back(id);
	removeFriendsDialog(ids);
}
void LLAvatarActions::removeFriendsDialog(const uuid_vec_t& ids)
{
	if(ids.size() == 0)
		return;
	LLSD args;
	std::string msgType;
	if(ids.size() == 1)
	{
		LLUUID agent_id = ids[0];
		LLAvatarName av_name;
		if(LLAvatarNameCache::get(agent_id, &av_name))
		{
			args["NAME"] = av_name.getNSName();
		}
		msgType = "RemoveFromFriends";
	}
	else
	{
		msgType = "RemoveMultipleFromFriends";
	}
	LLSD payload;
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		payload["ids"].append(*it);
	}
	LLNotificationsUtil::add(msgType,
		args,
		payload,
		&handleRemove);
}
void LLAvatarActions::offerTeleport(const LLUUID& invitee)
{
	if (invitee.isNull())
		return;
	offerTeleport(uuid_vec_t{invitee});
}
void LLAvatarActions::offerTeleport(const uuid_vec_t& ids)
{
	if (ids.size() == 0)
		return;
	handle_lure(ids);
}
static void on_avatar_name_cache_start_im(const LLUUID& agent_id, const LLAvatarName& av_name)
{
	static LLCachedControl<bool> tear_off("OtherChatsTornOff");
	if (!tear_off) gIMMgr->setFloaterOpen(true);
	gIMMgr->addSession(LLCacheName::cleanFullName(av_name.getLegacyName()), IM_NOTHING_SPECIAL, agent_id);
	make_ui_sound("UISndStartIM");
}
void LLAvatarActions::startIM(const LLUUID& id)
{
	if (id.isNull() || gAgentID == id)
		return;
	if ( (!RlvActions::canStartIM(id)) && (!RlvActions::hasOpenP2PSession(id)) )
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("agent", id, "completename").getSLURLString()));
		return;
	}
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(id, &av_name))
		on_avatar_name_cache_start_im(id, av_name);
	else
		LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_start_im, _1, _2));
}
void LLAvatarActions::endIM(const LLUUID& id)
{
	if (id.isNull())
		return;
	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, id);
	if (session_id.notNull())
	{
		gIMMgr->removeSession(session_id);
	}
}
static void on_avatar_name_cache_start_call(const LLUUID& agent_id,
											const LLAvatarName& av_name)
{
	LLUUID session_id = gIMMgr->addSession(LLCacheName::cleanFullName(av_name.getLegacyName()), IM_NOTHING_SPECIAL, agent_id);
	if (session_id.notNull())
	{
		gIMMgr->startCall(session_id);
	}
	make_ui_sound("UISndStartIM");
}
void LLAvatarActions::startCall(const LLUUID& id)
{
	if (id.isNull())
	{
		return;
	}
	if ( (!RlvActions::canStartIM(id)) && (!RlvActions::hasOpenP2PSession(id)) )
	{
		make_ui_sound("UISndInvalidOp");
		RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTIM, LLSD().with("RECIPIENT", LLSLURL("agent", id, "completename").getSLURLString()));
		return;
	}
	LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_start_call, _1, _2));
}
void LLAvatarActions::startAdhocCall(const uuid_vec_t& ids)
{
	if (ids.size() == 0)
	{
		return;
	}
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		const LLUUID& idAgent = *it;
		if (!RlvActions::canStartIM(idAgent))
		{
			make_ui_sound("UISndInvalidOp");
			RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTCONF);
			return;
		}
	}
	const std::string title = LLTrans::getString("conference-title");
	LLUUID session_id = gIMMgr->addSession(title, IM_SESSION_CONFERENCE_START,
										   ids[0], ids);
	if (session_id.isNull())
	{
		return;
	}
	gIMMgr->autoStartCallOnStartup(session_id);
	make_ui_sound("UISndStartIM");
}
bool LLAvatarActions::canCall()
{
	return LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();
}
void LLAvatarActions::startConference(const uuid_vec_t& ids)
{
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		const LLUUID& idAgent = *it;
		if (!RlvActions::canStartIM(idAgent))
		{
			make_ui_sound("UISndInvalidOp");
			RlvUtil::notifyBlocked(RLV_STRING_BLOCKED_STARTCONF);
			return;
		}
	}
	static LLCachedControl<bool> tear_off("OtherChatsTornOff");
	if (!tear_off) gIMMgr->setFloaterOpen(true);
	const std::string title = LLTrans::getString("conference-title");
	gIMMgr->addSession(title, IM_SESSION_CONFERENCE_START, ids[0], ids);
	make_ui_sound("UISndStartIM");
}
static const char* get_profile_floater_name(const LLUUID& avatar_id)
{
	return avatar_id == gAgentID ? "my_profile" : "profile";
}
static void on_avatar_name_show_profile(const LLUUID& agent_id, const LLAvatarName& av_name, bool web)
{
	if (gSavedSettings.getString("WebProfileURL").empty() || !(web || gSavedSettings.getBOOL("UseWebProfiles")))
	{
		LLFloaterAvatarInfo* floater = LLFloaterAvatarInfo::getInstance(agent_id);
		if(!floater)
		{
			floater = new LLFloaterAvatarInfo(av_name.getCompleteName()+" - "+LLTrans::getString("Command_Profile_Label"), agent_id);
			floater->center();
		}
		floater->open();
	}
	else
	{
		std::string url = getProfileURL(av_name.getAccountName());
		LLFloaterWebContent::Params p;
		p.url(url).id(agent_id.asString());
		LLFloaterWebProfile::showInstance(get_profile_floater_name(agent_id), p);
	}
}
void LLAvatarActions::showProfile(const LLUUID& id, bool web)
{
	if (id.notNull())
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(id, &av_name))
			on_avatar_name_show_profile(id, av_name, web);
		else
			LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_show_profile, _1, _2, web));
	}
}
void LLAvatarActions::showProfiles(const uuid_vec_t& ids, bool web)
{
	for (const auto& id : ids)
		showProfile(id, web);
}
bool LLAvatarActions::profileVisible(const LLUUID& id)
{
	LLFloater* browser = getProfileFloater(id);
	return browser && browser->getVisible();
}
LLFloater* LLAvatarActions::getProfileFloater(const LLUUID& id)
{
	LLFloater* browser;
	if (gSavedSettings.getString("WebProfileURL").empty() || !gSavedSettings.getBOOL("UseWebProfiles"))
		browser = LLFloaterAvatarInfo::getInstance(id);
	else
		browser =
			LLFloaterWebProfile::getInstance(id.asString());
	return browser;
}
void LLAvatarActions::hideProfile(const LLUUID& id)
{
	LLFloater* browser = getProfileFloater(id);
	if (browser)
	{
		browser->close();
	}
}
void LLAvatarActions::showOnMap(const LLUUID& id)
{
	LLAvatarName av_name;
	if (!LLAvatarNameCache::get(id, &av_name))
	{
		LLAvatarNameCache::get(id, boost::bind(&LLAvatarActions::showOnMap, id));
		return;
	}
	gFloaterWorldMap->trackAvatar(id, av_name.getNSName());
	LLFloaterWorldMap::show(true);
}
void LLAvatarActions::pay(const LLUUID& id)
{
	LLNotification::Params params("BusyModePay");
	params.functor(boost::bind(&LLAvatarActions::handlePay, _1, _2, id));
	if (gAgent.isDoNotDisturb())
	{
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 1);
	}
}
void LLAvatarActions::teleport_request_callback(const LLSD& notification, const LLSD& response)
{
	S32 option;
	if (response.isInteger())
	{
		option = response.asInteger();
	}
	else
	{
		option = LLNotificationsUtil::getSelectedOption(notification, response);
	}
	if (0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_MessageBlock);
		msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
		msg->addUUIDFast(_PREHASH_ToAgentID, notification["substitutions"]["uuid"] );
		msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
		msg->addU8Fast(_PREHASH_Dialog, IM_TELEPORT_REQUEST);
		msg->addUUIDFast(_PREHASH_ID, LLUUID::null);
		msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP);
		std::string name;
		LLAgentUI::buildFullname(name);
		msg->addStringFast(_PREHASH_FromAgentName, name);
		msg->addStringFast(_PREHASH_Message, response["message"]);
		msg->addU32Fast(_PREHASH_ParentEstateID, 0);
		msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
		msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
		gMessageSystem->addBinaryDataFast(
				_PREHASH_BinaryBucket,
				EMPTY_BINARY_BUCKET,
				EMPTY_BINARY_BUCKET_SIZE);
		gAgent.sendReliableMessage();
	}
}
void LLAvatarActions::teleportRequest(const LLUUID& id)
{
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(id, &av_name))
		on_avatar_name_cache_teleport_request(id, av_name);
	else
		LLAvatarNameCache::get(id, boost::bind(&on_avatar_name_cache_teleport_request, _1, _2));
}
void LLAvatarActions::on_avatar_name_cache_teleport_request(const LLUUID& id, const LLAvatarName& av_name)
{
	LLSD notification;
	notification["uuid"] = id;
	std::string name;
	if (!RlvActions::canShowName(RlvActions::SNC_TELEPORTREQUEST))
		name = RlvStrings::getAnonym(av_name.getLegacyName());
	else
		name = av_name.getNSName();
	notification["NAME"] = name;
	LLSD payload;
	LLNotificationsUtil::add("TeleportRequestPrompt", notification, payload, teleport_request_callback);
}
void LLAvatarActions::kick(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("KickUser", LLSD(), payload, handleKick);
}
void LLAvatarActions::freeze(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("FreezeUser", LLSD(), payload, handleFreeze);
}
void LLAvatarActions::unfreeze(const LLUUID& id)
{
	LLSD payload;
	payload["avatar_id"] = id;
	LLNotifications::instance().add("UnFreezeUser", LLSD(), payload, handleUnfreeze);
}
void LLAvatarActions::csr(const LLUUID& id)
{
	std::string name;
	if (!gCacheName->getFullName(id, name)) return;
	std::string url = "http://csr.lindenlab.com/agent/";
	if (char* output = curl_easy_escape(nullptr, name.c_str(), name.length()))
	{
		name = output;
		curl_free(output);
	}
	url += name;
	LLWeb::loadURL(url);
}
void LLAvatarActions::share(const LLUUID& id)
{
	LLPanelMainInventory::getActiveInventory()->setVisible(true);
	LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, id);
	if (!gIMMgr->hasSession(session_id))
	{
		startIM(id);
	}
	if (gIMMgr->hasSession(session_id))
	{
		LLIMMgr::getInstance()->addMessage(session_id, LLUUID::null, SYSTEM_FROM, LLTrans::getString("share_alert"));
		LLFloaterChatterBox::showInstance(session_id);
	}
}
namespace action_give_inventory
{
	static LLInventoryPanel* get_active_inventory_panel()
	{
		LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
		return active_panel;
	}
	static bool is_give_inventory_acceptable()
	{
		const auto inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
		if (inventory_selected_uuids.empty()) return false;
		bool acceptable = false;
		for (const auto& id : inventory_selected_uuids)
		{
			if (gInventory.getCategory(id))
			{
				acceptable = true;
				continue;
			}
			if (LLGiveInventory::isInventoryGiveAcceptable(gInventory.getItem(id)))
			{
				acceptable = true;
				continue;
			}
			acceptable = false;
			break;
		}
		return acceptable;
	}
	static void build_items_string(const uuid_set_t& inventory_selected_uuids, std::string& items_string)
	{
		llassert(inventory_selected_uuids.size() > 0);
		const std::string& separator = LLTrans::getString("words_separator");
		for (const auto& id : inventory_selected_uuids)
		{
			if (!items_string.empty()) items_string.append(separator);
			if (LLViewerInventoryCategory* inv_cat = gInventory.getCategory(id))
			{
				items_string = inv_cat->getName();
				break;
			}
			else if (LLViewerInventoryItem* inv_item = gInventory.getItem(id))
			{
				items_string.append(inv_item->getName());
			}
		}
	}
	struct LLShareInfo : public LLSingleton<LLShareInfo>
	{
		std::vector<LLAvatarName> mAvatarNames;
		uuid_vec_t mAvatarUuids;
	};
	static void give_inventory_cb(const LLSD& notification, const LLSD& response)
	{
		S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
		if (option == 1)
		{
			return;
		}
		const auto inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
		if (inventory_selected_uuids.empty())
		{
			return;
		}
		S32 count = LLShareInfo::instance().mAvatarNames.size();
		bool shared = count;
		for(S32 i = 0; i < count; ++i)
		{
			const LLUUID& avatar_uuid = LLShareInfo::instance().mAvatarUuids[i];
			const LLUUID session_id = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL, avatar_uuid);
			const std::string& separator = LLTrans::getString("words_separator");
			std::string noncopy_item_names;
			LLSD noncopy_items = LLSD::emptyArray();
			for (const auto& id : inventory_selected_uuids)
			{
				if (LLViewerInventoryCategory* inv_cat = gInventory.getCategory(id))
				{
					if (!LLGiveInventory::doGiveInventoryCategory(avatar_uuid, inv_cat, session_id, "ItemsShared"))
					{
						shared = false;
					}
					break;
				}
				LLViewerInventoryItem* inv_item = gInventory.getItem(id);
				if (!inv_item->getPermissions().allowCopyBy(gAgentID))
				{
					if (!noncopy_item_names.empty())
					{
						noncopy_item_names.append(separator);
					}
					noncopy_item_names.append(inv_item->getName());
					noncopy_items.append(id);
				}
				else
				{
					if (!LLGiveInventory::doGiveInventoryItem(avatar_uuid, inv_item, session_id))
					{
						shared = false;
					}
				}
			}
			if (noncopy_items.beginArray() != noncopy_items.endArray())
			{
				LLSD substitutions;
				substitutions["ITEMS"] = noncopy_item_names;
				LLSD payload;
				payload["agent_id"] = avatar_uuid;
				payload["items"] = noncopy_items;
				payload["success_notification"] = "ItemsShared";
				LLNotificationsUtil::add("CannotCopyWarning", substitutions, payload,
					&LLGiveInventory::handleCopyProtectedItem);
				shared = false;
				break;
			}
		}
		if (shared)
		{
			if (LLFloaterAvatarPicker::instanceExists()) LLFloaterAvatarPicker::instance().close();
			LLNotificationsUtil::add("ItemsShared");
		}
	}
	static void give_inventory(const uuid_vec_t& avatar_uuids, const std::vector<LLAvatarName> avatar_names)
	{
		llassert(avatar_names.size() == avatar_uuids.size());
		const auto inventory_selected_uuids = LLAvatarActions::getInventorySelectedUUIDs();
		if (inventory_selected_uuids.empty())
		{
			return;
		}
		std::string residents;
		LLAvatarActions::buildResidentsString(avatar_names, residents);
		std::string items;
		build_items_string(inventory_selected_uuids, items);
		auto folders_count = 0;
		for (const auto& id : inventory_selected_uuids)
		{
			if (gInventory.getCategory(id))
			{
				if (++folders_count == 2)
					break;
			}
		}
		std::string notification = folders_count == 2 ? "ShareFolderConfirmation" : "ShareItemsConfirmation";
		LLSD substitutions;
		substitutions["RESIDENTS"] = residents;
		substitutions["ITEMS"] = items;
		LLShareInfo::instance().mAvatarNames = avatar_names;
		LLShareInfo::instance().mAvatarUuids = avatar_uuids;
		LLNotificationsUtil::add(notification, substitutions, LLSD(), &give_inventory_cb);
	}
}
void LLAvatarActions::buildResidentsString(std::vector<LLAvatarName> avatar_names, std::string& residents_string)
{
	llassert(avatar_names.size() > 0);
	std::sort(avatar_names.begin(), avatar_names.end());
	const std::string& separator = LLTrans::getString("words_separator");
	for (std::vector<LLAvatarName>::const_iterator it = avatar_names.begin(); ; )
	{
		residents_string.append((*it).getNSName());
		if	(++it == avatar_names.end())
		{
			break;
		}
		residents_string.append(separator);
	}
}
void LLAvatarActions::buildResidentsString(const uuid_vec_t& avatar_uuids, std::string& residents_string)
{
	std::vector<LLAvatarName> avatar_names;
	uuid_vec_t::const_iterator it = avatar_uuids.begin();
	for (; it != avatar_uuids.end(); ++it)
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(*it, &av_name))
		{
			avatar_names.push_back(av_name);
		}
	}
	if (!avatar_names.empty())
	{
		LLAvatarActions::buildResidentsString(avatar_names, residents_string);
	}
}
uuid_set_t LLAvatarActions::getInventorySelectedUUIDs()
{
	LLInventoryPanel* active_panel = action_give_inventory::get_active_inventory_panel();
	return active_panel ? active_panel->getRootFolder()->getSelectionList() : uuid_set_t();
}
void LLAvatarActions::shareWithAvatars(LLView * panel)
{
	using namespace action_give_inventory;
	LLFloater* root_floater = gFloaterView->getParentFloater(panel);
	LLFloaterAvatarPicker* picker =
		LLFloaterAvatarPicker::show(boost::bind(give_inventory, _1, _2), TRUE, FALSE, FALSE, root_floater->getName());
	if (!picker)
	{
		return;
	}
	picker->setOkBtnEnableCb(boost::bind(is_give_inventory_acceptable));
	picker->openFriendsTab();
	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}
	LLNotificationsUtil::add("ShareNotification");
}
bool LLAvatarActions::canShareSelectedItems(LLInventoryPanel* inv_panel )
{
	using namespace action_give_inventory;
	if (!inv_panel)
	{
		LLInventoryPanel* active_panel = get_active_inventory_panel();
		if (!active_panel) return false;
		inv_panel = active_panel;
	}
	LLFolderView* root_folder = inv_panel->getRootFolder();
    if (!root_folder)
    {
        return false;
    }
	const auto inventory_selected = root_folder->getSelectionList();
	if (inventory_selected.empty()) return false;
	bool can_share = true;
	const LLUUID& trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	for (const auto& id : inventory_selected)
	{
		LLViewerInventoryCategory* inv_cat = gInventory.getCategory(id);
		if (inv_cat && !gInventory.isObjectDescendentOf(inv_cat->getUUID(), trash_id))
		{
			continue;
		}
		else if (!inv_cat && gInventory.isObjectDescendentOf(id, gInventory.getRootFolderID()))
			if (LLViewerInventoryItem* item = gInventory.getItem(id))
				if (LLInventoryCollectFunctor::itemTransferCommonlyAllowed(item) && LLGiveInventory::isInventoryGiveAcceptable(item))
		{
			continue;
		}
		can_share = false;
		break;
	}
	return can_share;
}
void LLAvatarActions::toggleBlock(const LLUUID& id)
{
	std::string name;
	gCacheName->getFullName(id, name);
	LLMute mute(id, name, LLMute::AGENT);
	if (LLMuteList::getInstance()->isMuted(mute.mID, mute.mName))
	{
		LLMuteList::getInstance()->remove(mute);
	}
	else
	{
		LLMuteList::getInstance()->add(mute);
	}
}
void LLAvatarActions::toggleMuteVoice(const LLUUID& id)
{
	std::string name;
	gCacheName->getFullName(id, name);
	LLMuteList* mute_list = LLMuteList::getInstance();
	bool is_muted = mute_list->isMuted(id, LLMute::flagVoiceChat);
	LLMute mute(id, name, LLMute::AGENT);
	if (!is_muted)
	{
		mute_list->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		mute_list->remove(mute, LLMute::flagVoiceChat);
	}
}
bool LLAvatarActions::canOfferTeleport(const LLUUID& id)
{
	if(LLAvatarTracker::instance().isBuddy(id))
	{
		return LLAvatarTracker::instance().isBuddyOnline(id);
	}
	return true;
}
bool LLAvatarActions::canOfferTeleport(const uuid_vec_t& ids)
{
	if(ids.size() > 250) return false;
	bool result = true;
	for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		if(!canOfferTeleport(*it))
		{
			result = false;
			break;
		}
	}
	return result;
}
void LLAvatarActions::inviteToGroup(const LLUUID& id)
{
	inviteToGroup(uuid_vec_t(1, id));
}
void LLAvatarActions::inviteToGroup(const uuid_vec_t& ids)
{
	LLFloaterGroupPicker* widget = LLFloaterGroupPicker::showInstance(LLSD(ids.front()));
	if (widget)
	{
		widget->center();
		widget->setPowersMask(GP_MEMBER_INVITE);
		widget->removeNoneOption();
		widget->setSelectGroupCallback(boost::bind(callback_invite_to_group, _1, ids));
	}
}
bool LLAvatarActions::handleRemove(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	const LLSD& ids = notification["payload"]["ids"];
	for (LLSD::array_const_iterator itr = ids.beginArray(); itr != ids.endArray(); ++itr)
	{
		LLUUID id = itr->asUUID();
		const LLRelationship* ip = LLAvatarTracker::instance().getBuddyInfo(id);
		if (ip)
		{
			switch (option)
			{
			case 0:
				if( ip->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS))
				{
					LLAvatarTracker::instance().empower(id, FALSE);
					LLAvatarTracker::instance().notifyObservers();
				}
				LLAvatarTracker::instance().terminateBuddy(id);
				LLAvatarTracker::instance().notifyObservers();
				gInventory.addChangedMask(LLInventoryObserver::LABEL | LLInventoryObserver::CALLING_CARD, LLUUID::null);
				gInventory.notifyObservers();
				break;
			case 1:
			default:
				LL_INFOS() << "No removal performed." << LL_ENDL;
				break;
			}
		}
	}
	return false;
}
bool LLAvatarActions::handlePay(const LLSD& notification, const LLSD& response, LLUUID avatar_id)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		gAgent.setDoNotDisturb(false);
	}
	LLFloaterPay::payDirectly(&give_money, avatar_id, false);
	return false;
}
void callback_ban_from_group(const LLUUID& group, uuid_vec_t& ids)
{
	LLFloaterGroupBulkBan::showForGroup(group, &ids);
}
void ban_from_group(const uuid_vec_t& ids)
{
	if (LLFloaterGroupPicker* widget = LLFloaterGroupPicker::showInstance(ids.front()))
	{
		widget->center();
		widget->setPowersMask(GP_GROUP_BAN_ACCESS);
		widget->removeNoneOption();
		widget->setSelectGroupCallback(boost::bind(callback_ban_from_group, _1, ids));
	}
}
void LLAvatarActions::callback_invite_to_group(LLUUID group_id, uuid_vec_t& agent_ids)
{
	LLFloaterGroupInvite::showForGroup(group_id, &agent_ids);
}
bool LLAvatarActions::callbackAddFriendWithMessage(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		requestFriendship(notification["payload"]["id"].asUUID(),
		    notification["payload"]["name"].asString(),
		    response["message"].asString());
	}
	return false;
}
bool LLAvatarActions::handleKick(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_DEFAULT );
		msg->addStringFast(_PREHASH_Reason,    response["message"].asString() );
		gAgent.sendReliableMessage();
	}
	return false;
}
bool LLAvatarActions::handleFreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_FREEZE );
		msg->addStringFast(_PREHASH_Reason, response["message"].asString() );
		gAgent.sendReliableMessage();
	}
	return false;
}
bool LLAvatarActions::handleUnfreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string text = response["message"].asString();
	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   avatar_id );
		msg->addU32("KickFlags", KICK_FLAGS_UNFREEZE );
		msg->addStringFast(_PREHASH_Reason,    text );
		gAgent.sendReliableMessage();
	}
	return false;
}
void LLAvatarActions::requestFriendship(const LLUUID& target_id, const std::string& target_name, const std::string& message)
{
	const LLUUID calling_card_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
	send_improved_im(target_id,
					 target_name,
					 message,
					 IM_ONLINE,
					 IM_FRIENDSHIP_OFFERED,
					 calling_card_folder_id);
	LLSD args;
	args["TO_NAME"] = getSLURL(target_id);
	LLSD payload;
	payload["from_id"] = target_id;
	LLNotificationsUtil::add("FriendshipOffered", args, payload);
}
bool LLAvatarActions::isFriend(const LLUUID& id)
{
	return ( NULL != LLAvatarTracker::instance().getBuddyInfo(id) );
}
bool LLAvatarActions::isBlocked(const LLUUID& id)
{
	return LLMuteList::getInstance()->isMuted(id);
}
bool LLAvatarActions::isVoiceMuted(const LLUUID& id)
{
	return LLMuteList::getInstance()->isMuted(id, LLMute::flagVoiceChat);
}
bool LLAvatarActions::canBlock(const LLUUID& id)
{
	bool is_linden = LLMuteList::getInstance()->isLinden(id);
	bool is_self = id == gAgentID;
	return !is_self && !is_linden;
}
bool LLAvatarActions::isAgentMappable(const LLUUID& agent_id)
{
	const LLRelationship* buddy_info = NULL;
	bool is_friend = LLAvatarActions::isFriend(agent_id);
	if (is_friend)
		buddy_info = LLAvatarTracker::instance().getBuddyInfo(agent_id);
	return (buddy_info &&
			buddy_info->isOnline() &&
			buddy_info->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION)
			);
}
void copy_from_ids(const uuid_vec_t& ids, std::function<std::string(const LLUUID&)> func)
{
	std::string ids_string;
	const std::string& separator = LLTrans::getString("words_separator");
	for (const auto& id : ids)
	{
		if (id.isNull())
			continue;
		if (!ids_string.empty())
			ids_string.append(separator);
		ids_string.append(func(id));
	}
	if (!ids_string.empty())
		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(ids_string));
}
void LLAvatarActions::copyUUIDs(const uuid_vec_t& ids)
{
	copy_from_ids(ids, [](const LLUUID& id) { return id.asString(); });
}
std::string LLAvatarActions::getSLURL(const LLUUID& id)
{
	return llformat("secondlife:///app/agent/%s/about", id.asString().c_str());
}
