/** 
 * @file llviewermessage.cpp
 * @brief Dumping ground for viewer-side message system callbacks.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
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
#include "llviewerprecompiledheaders.h"
#include "llviewermessage.h"
#include <boost/algorithm/string/replace.hpp>
#include "llanimationstates.h"
#include "llaudioengine.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llcororesponder.h"
#include "../lscript/lscript_byteformat.h"
#include "llfocusmgr.h"
#include "llfollowcamparams.h"
#include "llinventorydefines.h"
#include "llregionhandle.h"
#include "llsdserialize.h"
#include "llteleportflags.h"
#include "lltransactionflags.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llxfermanager.h"
#include "mean_collision_data.h"
#include "llagent.h"
#include "llagentbenefits.h"
#include "llagentcamera.h"
#include "llcallingcard.h"
#include "llcontrolavatar.h"
#include "llfirstuse.h"
#include "llfloaterbump.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuyland.h"
#include "llfloaterchat.h"
#include "llchataitranslate.h"
#include "llfloaterexperienceprofile.h"
#include "llfloaterland.h"
#include "llfloaterregioninfo.h"
#include "llfloaterlandholdings.h"
#include "llfloatermute.h"
#include "llfloaterpostcard.h"
#include "llfloaterpreference.h"
#include "llfloaterregionrestarting.h"
#include "llfloaterteleporthistory.h"
#include "llgroupactions.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llimprocessing.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "lllslconstants.h"
#include "llmarketplacefunctions.h"
#include "llmutelist.h"
#include "llnotify.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanelgrouplandmoney.h"
#include "llpanelmaininventory.h"
#include "llselectmgr.h"
#include "llstartup.h"
#include "llsky.h"
#include "llslurl.h"
#include "llstatenums.h"
#include "llstatusbar.h"
#include "llimview.h"
#include "llspeakers.h"
#include "lltrans.h"
#include "llviewerfoldertype.h"
#include "llviewermenu.h"
#include "llviewerinventory.h"
#include "llviewerjoystick.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvlmanager.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llattachmentsmgr.h"
#include "llworld.h"
#include "pipeline.h"
#include "llfloaterworldmap.h"
#include "llviewerdisplay.h"
#include "llkeythrottle.h"
#include "llagentui.h"
#include "llviewerregion.h"
#include  "llexperiencecache.h"
#include "rlvactions.h"
#include "rlvhandler.h"
#include "rlvinventory.h"
#include "rlvui.h"
#include <boost/regex.hpp>
#include "llexperiencecache.h"
#if SHY_MOD
# include "shcommandhandler.h"
#endif
#include "hippogridmanager.h"
#include "hippolimits.h"
#include "m7wlinterface.h"
#include "llgiveinventory.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>
#if LL_WINDOWS
#include "llwindebug.h"
#endif
#include "NACLantispam.h"
bool can_block(const LLUUID& id);
static const boost::regex NEWLINES("\\n{1}");
const F32 BIRD_AUDIBLE_RADIUS = 32.0f;
const F32 SIT_DISTANCE_FROM_TARGET = 0.25f;
const F32 CAMERA_POSITION_THRESHOLD_SQUARED = 0.001f * 0.001f;
static const F32 LOGOUT_REPLY_TIME = 3.f;
static const U32 LLREQUEST_PERMISSION_THROTTLE_LIMIT	= 5;
static const F32 LLREQUEST_PERMISSION_THROTTLE_INTERVAL	= 10.0f;
extern BOOL gDebugClicks;
extern bool gShiftFrame;
bool check_offer_throttle(const std::string& from_name, bool check_only);
bool check_asset_previewable(const LLAssetType::EType asset_type);
static void process_money_balance_reply_extended(LLMessageSystem* msg);
bool handle_trusted_experiences_notification(const LLSD&);
LLFrameTimer gThrottleTimer;
const U32 OFFER_THROTTLE_MAX_COUNT=5;
const F32 OFFER_THROTTLE_TIME=10.f;
const std::string SCRIPT_QUESTIONS[SCRIPT_PERMISSION_EOF] =
{
	"ScriptTakeMoney",
	"ActOnControlInputs",
	"RemapControlInputs",
	"AnimateYourAvatar",
	"AttachToYourAvatar",
	"ReleaseOwnership",
	"LinkAndDelink",
	"AddAndRemoveJoints",
	"ChangePermissions",
	"TrackYourCamera",
	"ControlYourCamera",
	"TeleportYourAgent",
	"JoinAnExperience",
	"SilentlyManageEstateAccess",
	"OverrideYourAnimations",
	"ScriptReturnObjects"
};
constexpr bool SCRIPT_QUESTION_IS_CAUTION[SCRIPT_PERMISSION_EOF] =
{
	true,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
};
void accept_friendship_coro(const LLCoroResponder& responder, const LLSD& notification)
{
    const auto& status = responder.getStatus();
    if (!responder.isGoodStatus(status))
    {
        LL_WARNS("Friendship") << "HTTP status " << status << ": " << responder.getReason() <<
            ". friendship offer accept failed." << LL_ENDL;
    }
    else
    {
        {
            LLSD payload = notification["payload"];
            LL_DEBUGS("Friendship") << "Adding friend to list" << responder.getContent() << LL_ENDL;
            LLNotificationsUtil::add("FriendshipAcceptedByMe",
                notification["substitutions"], payload);
        }
    }
}
void decline_friendship_coro(const LLCoroResponder& responder, const LLSD& notification, S32 option)
{
    const auto& status = responder.getStatus();
    if (!responder.isGoodStatus(status))
    {
        LL_WARNS("Friendship") << "HTTP status " << status << ": " << responder.getReason() <<
            ". friendship offer decline failed." << LL_ENDL;
    }
    else
    {
        {
			const auto& payload = notification["payload"];
            LL_DEBUGS("Friendship") << "Friendship declined" << responder.getContent() << LL_ENDL;
            if (option == 1)
            {
                LLNotificationsUtil::add("FriendshipDeclinedByMe",
                    notification["substitutions"], payload);
            }
            else if (option == 2)
            {
                LLAvatarActions::startIM(payload["from_id"].asUUID());
            }
        }
    }
}
bool friendship_offer_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLMessageSystem* msg = gMessageSystem;
	const LLSD& payload = notification["payload"];
	switch(option)
	{
	case 0:
	{
		LLAvatarTracker::formFriendship(payload["from_id"]);
		const LLUUID fid = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
            std::string url = gAgent.getRegionCapability("AcceptFriendship");
            LL_DEBUGS("Friendship") << "Cap string: " << url << LL_ENDL;
            if (!url.empty() && payload.has("online") && payload["online"].asBoolean() == false)
            {
                LL_DEBUGS("Friendship") << "Accepting friendship via capability" << LL_ENDL;
			    url += "?from=" + payload["from_id"].asString();
				url += "&agent_name=\"" + LLURI::escape(gAgentAvatarp->getFullname()) + '"';
                LLHTTPClient::post(url, LLSD(), new LLCoroResponder(
                    boost::bind(accept_friendship_coro, _1, notification)));
            }
            else if (payload.has("session_id") && payload["session_id"].asUUID().notNull())
            {
                LL_DEBUGS("Friendship") << "Accepting friendship via viewer message" << LL_ENDL;
				msg->newMessageFast(_PREHASH_AcceptFriendship);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_TransactionBlock);
				msg->addUUIDFast(_PREHASH_TransactionID, payload["session_id"]);
				msg->nextBlockFast(_PREHASH_FolderData);
				msg->addUUIDFast(_PREHASH_FolderID, fid);
				msg->sendReliable(LLHost(payload["sender"].asString()));
                LLNotificationsUtil::add("FriendshipAcceptedByMe",
                    notification["substitutions"], payload);
            }
            else
            {
                LL_WARNS("Friendship") << "Failed to accept friendship offer, neither capability nor transaction id are accessible" << LL_ENDL;
            }
		break;
	}
        case 1:
		    {
                std::string url = gAgent.getRegionCapability("DeclineFriendship");
                LL_DEBUGS("Friendship") << "Cap string: " << url << LL_ENDL;
                if (!url.empty() && payload.has("online") && payload["online"].asBoolean() == false)
                {
                    LL_DEBUGS("Friendship") << "Declining friendship via capability" << LL_ENDL;
					url += "?from=" + payload["from_id"].asString();
                    LLHTTPClient::del(url, new LLCoroResponder(
                        boost::bind(decline_friendship_coro, _1, notification, option)));
                }
                else if (payload.has("session_id") && payload["session_id"].asUUID().notNull())
                {
                    LL_DEBUGS("Friendship") << "Declining friendship via viewer message" << LL_ENDL;
					msg->newMessageFast(_PREHASH_DeclineFriendship);
					msg->nextBlockFast(_PREHASH_AgentData);
					msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
					msg->nextBlockFast(_PREHASH_TransactionBlock);
					msg->addUUIDFast(_PREHASH_TransactionID, payload["session_id"]);
					msg->sendReliable(LLHost(payload["sender"].asString()));
                    if (option == 1)
                    {
                        LLNotificationsUtil::add("FriendshipDeclinedByMe",
                            notification["substitutions"], payload);
                    }
                }
                else
                {
                    LL_WARNS("Friendship") << "Failed to decline friendship offer, neither capability nor transaction id are accessible" << LL_ENDL;
                }
	    }
	default:
		break;
	}
	return false;
}
static LLNotificationFunctorRegistration friendship_offer_callback_reg("OfferFriendship", friendship_offer_callback);
static LLNotificationFunctorRegistration friendship_offer_callback_reg_nm("OfferFriendshipNoMessage", friendship_offer_callback);
void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount, BOOL is_group,
				S32 trx_type, const std::string& desc)
{
	if (0 == amount || !region) return;
	amount = abs(amount);
	LL_INFOS("Messaging") << "give_money(" << uuid << "," << amount << ")" << LL_ENDL;
	if (can_afford_transaction(amount))
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_MoneyTransferRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_MoneyData);
		msg->addUUIDFast(_PREHASH_SourceID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_DestID, uuid);
		msg->addU8Fast(_PREHASH_Flags, pack_transaction_flags(FALSE, is_group));
		msg->addS32Fast(_PREHASH_Amount, amount);
		msg->addU8Fast(_PREHASH_AggregatePermNextOwner, (U8)LLAggregatePermissions::AP_EMPTY);
		msg->addU8Fast(_PREHASH_AggregatePermInventory, (U8)LLAggregatePermissions::AP_EMPTY);
		msg->addS32Fast(_PREHASH_TransactionType, trx_type);
		msg->addStringFast(_PREHASH_Description, desc);
		msg->sendReliable(region->getHost());
	}
	else
	{
		LLStringUtil::format_map_t args;
		args["CURRENCY"] = gHippoGridManager->getConnectedGrid()->getCurrencySymbol();
		LLFloaterBuyCurrency::buyCurrency( LLTrans::getString("giving", args)+" ", amount );
	}
}
void send_complete_agent_movement(const LLHost& sim_host)
{
	LL_INFOS("Teleport", "Messaging") << "Sending CompleteAgentMovement to " << sim_host << LL_ENDL;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_CompleteAgentMovement);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_CircuitCode, msg->mOurCircuitCode);
	msg->sendReliable(sim_host);
}

void process_logout_reply(LLMessageSystem* msg, void**)
{
	LL_DEBUGS("Messaging") << "process_logout_reply" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	LLUUID session_id;
	msg->getUUID("AgentData", "SessionID", session_id);
	if ((agent_id != gAgent.getID()) || (session_id != gAgent.getSessionID()))
	{
		LL_WARNS("Messaging") << "Bogus Logout Reply" << LL_ENDL;
	}
	LLInventoryModel::update_map_t parents;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for (S32 i = 0; i < count; ++i)
	{
		LLUUID item_id;
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id, i);
		if ((1 == count) && item_id.isNull())
		{
			break;
		}
		LL_INFOS("Messaging") << "process_logout_reply itemID=" << item_id << LL_ENDL;
		LLInventoryItem* item = gInventory.getItem(item_id);
		if (item)
		{
			parents[item->getParentUUID()] = 0;
			gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		}
		else
		{
			LL_INFOS("Messaging") << "process_logout_reply item not found: " << item_id << LL_ENDL;
		}
	}
    LLAppViewer::instance()->forceQuit();
}
void process_layer_data(LLMessageSystem* mesgsys, void** user_data)
{
	LLViewerRegion* regionp = LLWorld::getInstance()->getRegion(mesgsys->getSender());
	LL_DEBUGS_ONCE("SceneLoadTiming") << "Received layer data" << LL_ENDL;
	if (!regionp || gNoRender)
	{
		LL_WARNS() << "Invalid region for layer data." << LL_ENDL;
		return;
	}
	S32 size;
	S8 type;
	mesgsys->getS8Fast(_PREHASH_LayerID, _PREHASH_Type, type);
	size = mesgsys->getSizeFast(_PREHASH_LayerData, _PREHASH_Data);
	if (0 == size)
	{
		LL_WARNS("Messaging") << "Layer data has zero size." << LL_ENDL;
		return;
	}
	if (size < 0)
	{
		LL_WARNS("Messaging") << "getSizeFast() returned negative result: "
			<< size
			<< LL_ENDL;
		return;
	}
	U8* datap = new U8[size];
	mesgsys->getBinaryDataFast(_PREHASH_LayerData, _PREHASH_Data, datap, size);
	LLVLData* vl_datap = new LLVLData(regionp, type, datap, size);
	if (mesgsys->getReceiveCompressedSize())
	{
		gVLManager.addLayerData(vl_datap, mesgsys->getReceiveCompressedSize());
	}
	else
	{
		gVLManager.addLayerData(vl_datap, mesgsys->getReceiveSize());
	}
}
void process_derez_ack(LLMessageSystem*, void**)
{
	if (gViewerWindow) gViewerWindow->getWindow()->decBusyCount();
}
void process_places_reply(LLMessageSystem* msg, void** data)
{
	LLUUID query_id;
	msg->getUUID("AgentData", "QueryID", query_id);
	if (query_id.isNull())
	{
		LLFloaterLandHoldings::processPlacesReply(msg, data);
	}
	else if (gAgent.isInGroup(query_id))
	{
		LLPanelGroupLandMoney::processPlacesReply(msg, data);
	}
	else
	{
		LL_WARNS("Messaging") << "Got invalid PlacesReply message" << LL_ENDL;
	}
}
void send_sound_trigger(const LLUUID& sound_id, F32 gain)
{
	if (sound_id.isNull() || gAgent.getRegion() == nullptr)
	{
		return;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_SoundTrigger);
	msg->nextBlockFast(_PREHASH_SoundData);
	msg->addUUIDFast(_PREHASH_SoundID, sound_id);
	msg->addUUIDFast(_PREHASH_OwnerID, LLUUID::null);
	msg->addUUIDFast(_PREHASH_ObjectID, LLUUID::null);
	msg->addUUIDFast(_PREHASH_ParentID, LLUUID::null);
	msg->addU64Fast(_PREHASH_Handle, gAgent.getRegion()->getHandle());
	LLVector3 position = gAgent.getPositionAgent();
	msg->addVector3Fast(_PREHASH_Position, position);
	msg->addF32Fast(_PREHASH_Gain, gain);
	gAgent.sendMessage();
}
static LLSD sSavedGroupInvite;
static LLSD sSavedResponse;
void response_group_invitation_coro(const LLCoroResponder& responder, const LLUUID& group_id, bool notify_and_update)
{
    const auto& status = responder.getStatus();
    if (!responder.isGoodStatus(status))
    {
        LL_WARNS("GroupInvite") << "HTTP status " << status << ": " << responder.getReason() <<
            ". Group " << group_id << " invitation response processing failed." << LL_ENDL;
	}
	else
	{
        {
            LL_DEBUGS("GroupInvite") << "Successfully sent response to group " << group_id << " invitation" << LL_ENDL;
            if (notify_and_update)
            {
                LLNotificationsUtil::add("JoinGroupSuccess");
                gAgent.sendAgentDataUpdateRequest();
                LLGroupMgr::getInstance()->clearGroupData(group_id);
                LLGroupActions::refresh(group_id);
            }
        }
	}
}
void send_join_group_response(const LLUUID& group_id, const LLUUID& transaction_id, bool accept_invite, S32 fee, bool use_offline_cap, LLSD& payload)
{
    if (accept_invite && fee > 0)
	{
			LLSD args;
			args["COST"] = llformat("%d", fee);
            LLSD next_payload = payload;
			next_payload["fee"] = 0;
			LLNotificationsUtil::add("JoinGroupCanAfford",
				args,
                next_payload);
    }
    else if (use_offline_cap)
    {
        std::string url;
        if (accept_invite)
        {
            url = gAgent.getRegionCapability("AcceptGroupInvite");
		}
		else
		{
            url = gAgent.getRegionCapability("DeclineGroupInvite");
        }
        if (!url.empty())
        {
            LL_DEBUGS("GroupInvite") << "Capability url: " << url << LL_ENDL;
		    LLSD payload;
		    payload["group"] = group_id;
            LLHTTPClient::post(url, payload, new LLCoroResponder(
                boost::bind(response_group_invitation_coro, _1, group_id, accept_invite)));
        }
        else
        {
            LL_WARNS("GroupInvite") << "No capability, can't reply to offline invitation!" << LL_ENDL;
        }
    }
    else
    {
        LL_DEBUGS("GroupInvite") << "Replying to group invite via IM message" << LL_ENDL;
        EInstantMessage type = accept_invite ? IM_GROUP_INVITATION_ACCEPT : IM_GROUP_INVITATION_DECLINE;
		send_improved_im(group_id,
			std::string("name"),
			std::string("message"),
			IM_ONLINE,
            type,
			transaction_id);
	}
}
void send_join_group_response(const LLUUID& group_id, const LLUUID& transaction_id, bool accept_invite, S32 fee, bool use_offline_cap)
{
    LLSD payload;
    if (accept_invite)
    {
        payload["group_id"] = group_id;
        payload["transaction_id"] =  transaction_id;
        payload["fee"] =  fee;
        payload["use_offline_cap"] = use_offline_cap;
    }
    send_join_group_response(group_id, transaction_id, accept_invite, fee, use_offline_cap, payload);
}
bool join_group_response(const LLSD& notification, const LLSD& response)
{
	LLSD notification_adjusted = notification;
	LLSD response_adjusted = response;
	std::string action = notification["name"];
	std::string id = notification_adjusted["payload"]["group_id"].asString();
	if ("JoinGroup" == action || "JoinGroupCanAfford" == action)
	{
		sSavedGroupInvite[id] = notification;
		sSavedResponse[id] = response;
	}
	else if ("JoinedTooManyGroupsMember" == action)
	{
		S32 opt = LLNotificationsUtil::getSelectedOption(notification, response);
		if (0 == opt)
		{
			notification_adjusted = sSavedGroupInvite[id];
			response_adjusted = sSavedResponse[id];
		}
		sSavedGroupInvite.erase(id);
		sSavedResponse.erase(id);
	}
	S32 option = LLNotificationsUtil::getSelectedOption(notification_adjusted, response_adjusted);
	bool accept_invite = false;
	LLUUID group_id = notification_adjusted["payload"]["group_id"].asUUID();
	LLUUID transaction_id = notification_adjusted["payload"]["transaction_id"].asUUID();
	std::string name = notification_adjusted["payload"]["name"].asString();
	std::string message = notification_adjusted["payload"]["message"].asString();
	S32 fee = notification_adjusted["payload"]["fee"].asInteger();
	U8 use_offline_cap = notification_adjusted["payload"]["use_offline_cap"].asInteger();
	if (option == 2 && !group_id.isNull())
	{
		LLGroupActions::show(group_id);
		LLSD args;
		args["MESSAGE"] = message;
		LLNotificationsUtil::add("JoinGroup", args, notification_adjusted["payload"]);
		return false;
	}
	if (option == 0 && !group_id.isNull())
	{
		S32 max_groups = LLAgentBenefitsMgr::current().getGroupMembershipLimit();
		if (gAgent.isInGroup(group_id)) ++max_groups;
		if ((S32)gAgent.mGroups.size() < max_groups)
		{
			accept_invite = true;
		}
		else
		{
			LLSD args;
			args["NAME"] = name;
			args["INVITE"] = message;
			LLNotificationsUtil::add("JoinedTooManyGroupsMember", args, notification["payload"]);
			return false;
		}
	}
	send_join_group_response(group_id, transaction_id, accept_invite, fee, use_offline_cap, notification_adjusted["payload"]);
	sSavedGroupInvite[id] = LLSD::emptyMap();
	sSavedResponse[id] = LLSD::emptyMap();
	return false;
}
static void highlight_inventory_objects_in_panel(const uuid_vec_t& items, LLInventoryPanel *inventory_panel)
{
	if (nullptr == inventory_panel) return;
	for (auto item_id : items)
	{
        if (!highlight_offered_object(item_id))
		{
			continue;
		}
		LLInventoryObject* item = gInventory.getObject(item_id);
		llassert(item);
		if (!item)
		{
			continue;
		}
		LL_DEBUGS("Inventory_Move") << "Highlighting inventory item: " << item->getName() << ", " << item_id  << LL_ENDL;
		LLFolderView* fv = inventory_panel->getRootFolder();
		if (fv)
		{
			LLFolderViewItem* fv_item = fv->getItemByID(item_id);
			if (fv_item)
			{
				LLFolderViewItem* fv_folder = fv_item->getParentFolder();
				if (fv_folder)
				{
					LL_DEBUGS("Inventory_Move") << "Open folder: " << fv_folder->getName() << LL_ENDL;
					fv_folder->setOpen(TRUE);
					if (fv_folder->isSelected())
					{
						fv->changeSelection(fv_folder, FALSE);
					}
				}
				fv->changeSelection(fv_item, TRUE);
			}
		}
	}
}
static LLNotificationFunctorRegistration jgr_1("JoinGroup", join_group_response);
static LLNotificationFunctorRegistration jgr_2("JoinedTooManyGroupsMember", join_group_response);
static LLNotificationFunctorRegistration jgr_3("JoinGroupCanAfford", join_group_response);
class LLOpenAgentOffer final : public LLInventoryFetchItemsObserver
{
public:
	LLOpenAgentOffer(const LLUUID& object_id,
					 const std::string& from_name) :
		LLInventoryFetchItemsObserver(object_id),
		mFromName(from_name)
	{
	}
	void startFetch() override
	{
		for (uuid_vec_t::const_iterator it = mIDs.begin(); it < mIDs.end(); ++it)
		{
			LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
			if (cat)
			{
				mComplete.push_back((*it));
			}
		}
		LLInventoryFetchItemsObserver::startFetch();
	}
	void done() override
	{
		open_inventory_offer(mComplete, mFromName);
		gInventory.removeObserver(this);
		delete this;
	}
private:
	std::string mFromName;
};
class LLViewerInventoryMoveFromWorldObserver final : public LLInventoryAddItemByAssetObserver
{
public:
	LLViewerInventoryMoveFromWorldObserver()
		: LLInventoryAddItemByAssetObserver()
	{
	}
	void setMoveIntoFolderID(const LLUUID& into_folder_uuid) { mMoveIntoFolderID = into_folder_uuid; }
private:
	void onAssetAdded(const LLUUID& asset_id) override
	{
		if (LLInventoryPanel::getActiveInventoryPanel())
		{
			mActivePanel = LLInventoryPanel::getActiveInventoryPanel()->getHandle();
		}
		mSelectedItems.clear();
		if (LLInventoryPanel::getActiveInventoryPanel())
		{
			mSelectedItems = LLInventoryPanel::getActiveInventoryPanel()->getRootFolder()->getSelectionList();
		}
		mSelectedItems.erase(mMoveIntoFolderID);
	}
	void done() override
	{
		LLInventoryPanel* active_panel = dynamic_cast<LLInventoryPanel*>(mActivePanel.get());
		if (active_panel && !isSelectionChanged())
		{
			LL_DEBUGS("Inventory_Move") << "Selecting new items..." << LL_ENDL;
			active_panel->clearSelection();
			highlight_inventory_objects_in_panel(mAddedItems, active_panel);
		}
	}
	bool isSelectionChanged()
	{
		LLInventoryPanel* active_panel = dynamic_cast<LLInventoryPanel*>(mActivePanel.get());
		if (nullptr == active_panel)
		{
			return true;
		}
		selected_items_t selected_items = active_panel->getRootFolder()->getSelectionList();
		selected_items.erase(mMoveIntoFolderID);
		selected_items_t different_items;
		std::set_symmetric_difference(mSelectedItems.begin(), mSelectedItems.end(),
			selected_items.begin(), selected_items.end(), std::inserter(different_items, different_items.begin()));
		LL_DEBUGS("Inventory_Move") << "Selected firstly: " << mSelectedItems.size()
			<< ", now: " << selected_items.size() << ", difference: " << different_items.size() << LL_ENDL;
		return !different_items.empty();
	}
	LLHandle<LLPanel> mActivePanel;
	typedef uuid_set_t selected_items_t;
	selected_items_t mSelectedItems;
	LLUUID mMoveIntoFolderID;
};
LLViewerInventoryMoveFromWorldObserver* gInventoryMoveObserver = nullptr;
void set_dad_inventory_item(LLInventoryItem* inv_item, const LLUUID& into_folder_uuid)
{
	start_new_inventory_observer();
	gInventoryMoveObserver->setMoveIntoFolderID(into_folder_uuid);
	gInventoryMoveObserver->watchAsset(inv_item->getAssetUUID());
}
class LLViewerInventoryMoveObserver final : public LLInventoryObserver
{
public:
	LLViewerInventoryMoveObserver(const LLUUID& object_id)
		: LLInventoryObserver()
		, mObjectID(object_id)
	{
		if (LLInventoryPanel::getActiveInventoryPanel())
		{
			mActivePanel = LLInventoryPanel::getActiveInventoryPanel()->getHandle();
		}
	}
	virtual ~LLViewerInventoryMoveObserver() = default;
	void changed(U32 mask) override;
private:
	LLUUID mObjectID;
	LLHandle<LLPanel> mActivePanel;
};
void LLViewerInventoryMoveObserver::changed(U32 mask)
{
	LLInventoryPanel* active_panel = dynamic_cast<LLInventoryPanel*>(mActivePanel.get());
	if (nullptr == active_panel)
	{
		gInventory.removeObserver(this);
		return;
	}
	if ((mask & (STRUCTURE)) != 0)
	{
		const uuid_set_t& changed_items = gInventory.getChangedIDs();
		auto id_it = changed_items.begin();
		auto id_end = changed_items.end();
		for (;id_it != id_end; ++id_it)
		{
			if ((*id_it) == mObjectID)
			{
				active_panel->clearSelection();
				highlight_inventory_objects_in_panel({mObjectID}, active_panel);
				active_panel->getRootFolder()->scrollToShowSelection();
				gInventory.removeObserver(this);
				break;
			}
		}
	}
}
void set_dad_inbox_object(const LLUUID& object_id)
{
	LLViewerInventoryMoveObserver* move_observer = new LLViewerInventoryMoveObserver(object_id);
	gInventory.addObserver(move_observer);
}
class LLOpenTaskOffer final : public LLInventoryAddedObserver
{
protected:
	void done() override
	{
		uuid_vec_t added;
		for (auto it : gInventory.getAddedIDs())
		{
			added.push_back(it);
		}
		for (uuid_vec_t::iterator it = added.begin(); it != added.end();)
		{
			const LLUUID& item_uuid = *it;
			bool was_moved = false;
			LLInventoryObject* added_object = gInventory.getObject(item_uuid);
			if (added_object)
			{
				LLInventoryItem* added_item = dynamic_cast<LLInventoryItem*>(added_object);
				if (added_item)
				{
					const LLUUID& asset_uuid = added_item->getAssetUUID();
					if (gInventoryMoveObserver->isAssetWatched(asset_uuid))
					{
						LL_DEBUGS("Inventory_Move") << "Found asset UUID: " << asset_uuid << LL_ENDL;
						was_moved = true;
					}
				}
			}
			if (was_moved)
			{
				it = added.erase(it);
			}
			else ++it;
		}
		open_inventory_offer(added, "");
	}
};
class LLOpenTaskGroupOffer final : public LLInventoryAddedObserver
{
protected:
	void done() override
	{
		uuid_vec_t added;
		for (auto it : gInventory.getAddedIDs())
		{
			added.push_back(it);
		}
		open_inventory_offer(added, "group_offer");
		gInventory.removeObserver(this);
		delete this;
	}
};
LLOpenTaskOffer* gNewInventoryObserver = nullptr;
class LLNewInventoryHintObserver final : public LLInventoryAddedObserver
{
protected:
	void done() override
	{
	}
};
LLNewInventoryHintObserver* gNewInventoryHintObserver = nullptr;
void start_new_inventory_observer()
{
	if (!gNewInventoryObserver)
	{
		gNewInventoryObserver = new LLOpenTaskOffer;
		gInventory.addObserver(gNewInventoryObserver);
	}
	if (!gInventoryMoveObserver)
	{
		gInventoryMoveObserver = new LLViewerInventoryMoveFromWorldObserver;
		gInventory.addObserver(gInventoryMoveObserver);
	}
	if (!gNewInventoryHintObserver)
	{
		gNewInventoryHintObserver = new LLNewInventoryHintObserver();
		gInventory.addObserver(gNewInventoryHintObserver);
	}
}
class LLDiscardAgentOffer final : public LLInventoryFetchItemsObserver
{
	LOG_CLASS(LLDiscardAgentOffer);
public:
	LLDiscardAgentOffer(const LLUUID& folder_id, const LLUUID& object_id) :
		LLInventoryFetchItemsObserver(object_id),
		mFolderID(folder_id),
		mObjectID(object_id)
	{
	}
	void done() override
	{
		LL_DEBUGS("Messaging") << "LLDiscardAgentOffer::done()" << LL_ENDL;
		LLAppViewer::instance()->addOnIdleCallback(std::bind(&LLInventoryModel::removeObject, &gInventory, mObjectID));
		gInventory.removeObserver(this);
		delete this;
	}
protected:
	LLUUID mFolderID;
	LLUUID mObjectID;
};
bool check_offer_throttle(const std::string& from_name, bool check_only)
{
	static U32 throttle_count;
	static bool throttle_logged;
	LLChat chat;
	if (!gSavedSettings.getBOOL("ShowNewInventory"))
		return false;
	if (check_only)
	{
		return gThrottleTimer.hasExpired();
	}
	if (gThrottleTimer.checkExpirationAndReset(OFFER_THROTTLE_TIME))
	{
		LL_DEBUGS("Messaging") << "Throttle Expired" << LL_ENDL;
		throttle_count = 1;
		throttle_logged = false;
		return true;
	}
	LL_DEBUGS("Messaging") << "Throttle Not Expired, Count: " << throttle_count << LL_ENDL;
	if (LLStartUp::getStartupState() >= STATE_STARTED
		&& throttle_count >= OFFER_THROTTLE_MAX_COUNT)
	{
		if (!throttle_logged)
		{
			LLStringUtil::format_map_t arg;
			std::string log_msg;
			std::ostringstream time;
			time << OFFER_THROTTLE_TIME;
			arg["APP_NAME"] = LLAppViewer::instance()->getSecondLifeTitle();
			arg["TIME"] = time.str();
			if (!from_name.empty())
			{
				arg["FROM_NAME"] = from_name;
				log_msg = LLTrans::getString("ItemsComingInTooFastFrom", arg);
			}
			else
			{
				log_msg = LLTrans::getString("ItemsComingInTooFast", arg);
			}
			chat.mText = log_msg;
			LLFloaterChat::addChat(chat, FALSE, FALSE);
			throttle_logged = true;
		}
		return false;
	}
	throttle_count++;
	return true;
}
bool check_asset_previewable(const LLAssetType::EType asset_type)
{
	return	(asset_type == LLAssetType::AT_NOTECARD)  ||
			(asset_type == LLAssetType::AT_LANDMARK)  ||
			(asset_type == LLAssetType::AT_TEXTURE)   ||
			(asset_type == LLAssetType::AT_ANIMATION) ||
			(asset_type == LLAssetType::AT_SCRIPT)    ||
			(asset_type == LLAssetType::AT_SOUND);
}
void open_inventory_offer(const uuid_vec_t& objects, const std::string& from_name)
{
	if (gAgent.isDoNotDisturb()) return;
	for (auto obj_id : objects)
	{
        if (!highlight_offered_object(obj_id))
		{
			continue;
		}
		const LLInventoryObject* obj = gInventory.getObject(obj_id);
		if (!obj)
		{
			LL_WARNS() << "Cannot find object [ itemID:" << obj_id << " ] to open." << LL_ENDL;
			continue;
		}
		const LLAssetType::EType asset_type = obj->getActualType();
		const LLInventoryItem* item = dynamic_cast<const LLInventoryItem*>(obj);
		if (item && check_asset_previewable(asset_type))
		{
			if (check_offer_throttle(from_name, false))
			{
				bool show_keep_discard = true;
				switch(asset_type)
				{
					case LLAssetType::AT_NOTECARD:
					{
						open_notecard((LLViewerInventoryItem*)item, std::string("Note: ") + item->getName(), LLUUID::null, show_keep_discard, LLUUID::null, FALSE);
						break;
					}
					case LLAssetType::AT_LANDMARK:
					{
						open_landmark((LLViewerInventoryItem*)item, std::string("Landmark: ") + item->getName(), show_keep_discard, LLUUID::null, FALSE);
					}
					break;
					case LLAssetType::AT_TEXTURE:
					{
						open_texture(obj_id, std::string("Texture: ") + item->getName(), show_keep_discard, LLUUID::null, FALSE);
						break;
					}
					case LLAssetType::AT_ANIMATION:
					case LLAssetType::AT_SCRIPT:
					case LLAssetType::AT_SOUND:
						LLInvFVBridgeAction::doAction(asset_type, obj_id, &gInventory);
						break;
					default:
						LL_DEBUGS("Messaging") << "No preview method for previewable asset type : " << LLAssetType::lookupHumanReadable(asset_type)  << LL_ENDL;
						break;
				}
			}
		}
		LLPanelMainInventory* view = LLPanelMainInventory::getActiveInventory();
		if(!view)
		{
			return;
		}
		if ((gInventory.isObjectDescendentOf(obj_id, gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH)))
		|| (gAwayTimer.getStarted() && gInventory.isObjectDescendentOf(obj_id, gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND))))
		{
			return;
		}
		if (gSavedSettings.getBOOL("ShowInInventory") &&
		   objects.size() == 1 && item != NULL &&
		   asset_type != LLAssetType::AT_CALLINGCARD &&
		   item->getInventoryType() != LLInventoryType::EType::IT_ATTACHMENT &&
		   !from_name.empty())
		{
			LLPanelMainInventory::showAgentInventory(TRUE);
		}
		if (!gSavedSettings.getBOOL("LiruHighlightNewInventory")) return;
		LL_DEBUGS("Messaging") << "Highlighting" << obj_id  << LL_ENDL;
		LLFocusableElement* focus_ctrl = gFocusMgr.getKeyboardFocus();
		view->getPanel()->setSelection(obj_id, TAKE_FOCUS_NO);
		gFocusMgr.setKeyboardFocus(focus_ctrl);
	}
}
bool highlight_offered_object(const LLUUID& obj_id)
{
	const LLInventoryObject* obj = gInventory.getObject(obj_id);
	if (!obj)
	{
		LL_WARNS("Messaging") << "Unable to show inventory item: " << obj_id << LL_ENDL;
		return false;
	}
	if (!gAgent.getAFK())
	{
		const LLViewerInventoryCategory* parent = gInventory.getFirstNondefaultParent(obj_id);
		if (parent)
		{
			const LLFolderType::EType parent_type = parent->getPreferredType();
			if (LLViewerFolderType::lookupIsQuietType(parent_type))
			{
				return false;
			}
		}
	}
	return true;
}
void inventory_offer_mute_callback(const LLUUID& blocked_id,
								   const std::string& full_name,
								   bool is_group)
{
	LLMute::EType mute_type = is_group ? LLMute::GROUP : LLMute::AGENT;
	LLMute mute(blocked_id, full_name, mute_type);
	if (LLMuteList::getInstance()->add(mute))
	{
		LLFloaterMute::showInstance()->selectMute(blocked_id);
	}
	class OfferMatcher final : public LLNotifyBoxView::Matcher
	{
	public:
		OfferMatcher(const LLUUID& to_block) : blocked_id(to_block)
		{
		}
		bool matches(const LLNotificationPtr notification) const override
		{
			if(notification->getName() == "ObjectGiveItem"
				|| notification->getName() == "ObjectGiveItemUnknownUser"
				|| notification->getName() == "UserGiveItem")
			{
				return (notification->getPayload()["from_id"].asUUID() == blocked_id);
			}
			return FALSE;
		}
	private:
		const LLUUID& blocked_id;
	};
	gNotifyBoxView->purgeMessagesMatching(OfferMatcher(blocked_id));
}
LLOfferInfo::LLOfferInfo()
 : mFromGroup(FALSE)
 , mFromObject(FALSE)
 , mIM(IM_NOTHING_SPECIAL)
 , mType(LLAssetType::AT_NONE)
{
}
LLOfferInfo::LLOfferInfo(const LLSD& sd)
{
	mIM = (EInstantMessage)sd["im_type"].asInteger();
	mFromID = sd["from_id"].asUUID();
	mFromGroup = sd["from_group"].asBoolean();
	mFromObject = sd["from_object"].asBoolean();
	mTransactionID = sd["transaction_id"].asUUID();
	mFolderID = sd["folder_id"].asUUID();
	mObjectID = sd["object_id"].asUUID();
	mType = LLAssetType::lookup(sd["type"].asString().c_str());
	mFromName = sd["from_name"].asString();
	mDesc = sd["description"].asString();
	mHost = LLHost(sd["sender"].asString());
}
LLOfferInfo::LLOfferInfo(const LLOfferInfo& info)
{
	mIM = info.mIM;
	mFromID = info.mFromID;
	mFromGroup = info.mFromGroup;
	mFromObject = info.mFromObject;
	mTransactionID = info.mTransactionID;
	mFolderID = info.mFolderID;
	mObjectID = info.mObjectID;
	mType = info.mType;
	mFromName = info.mFromName;
	mDesc = info.mDesc;
	mHost = info.mHost;
}
LLSD LLOfferInfo::asLLSD()
{
	LLSD sd;
	sd["im_type"] = mIM;
	sd["from_id"] = mFromID;
	sd["from_group"] = mFromGroup;
	sd["from_object"] = mFromObject;
	sd["transaction_id"] = mTransactionID;
	sd["folder_id"] = mFolderID;
	sd["object_id"] = mObjectID;
	sd["type"] = LLAssetType::lookup(mType);
	sd["from_name"] = mFromName;
	sd["description"] = mDesc;
	sd["sender"] = mHost.getIPandPort();
	return sd;
}
bool LLOfferInfo::inventory_offer_callback(const LLSD& notification, const LLSD& response)
{
	LLChat chat;
	std::string log_message;
	S32 button = LLNotificationsUtil::getSelectedOption(notification, response);
	if (button == 4)
	{
		LLAvatarActions::showProfile(mFromID);
		LLNotification::Params p(notification["name"]);
		p.substitutions(notification["substitutions"]).payload(notification["payload"]).functor(boost::bind(&LLOfferInfo::inventory_offer_callback, this, _1, _2));
		LLNotifications::instance().add(p);
		return false;
	}
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	LLViewerInventoryCategory* catp = NULL;
	catp = gInventory.getCategory(mObjectID);
	LLViewerInventoryItem* itemp = NULL;
	if(!catp)
	{
		itemp = (LLViewerInventoryItem*)gInventory.getItem(mObjectID);
	}
	if (2 == button)
	{
		gCacheName->get(mFromID, mFromGroup, boost::bind(&inventory_offer_mute_callback,_1,_2,_3));
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, mFromID);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addUUIDFast(_PREHASH_ID, mTransactionID);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP);
	std::string name;
	LLAgentUI::buildFullname(name);
	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, "");
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
	LLInventoryObserver* opener = NULL;
	std::string from_string;
	std::string chatHistory_string;
	if (mFromObject == TRUE)
	{
		if (mFromGroup)
		{
			std::string group_name = LLGroupActions::getSLURL(mFromID);
			{
				from_string = LLTrans::getString("InvOfferAnObjectNamed") + " " + LLTrans::getString("'")
				+ mFromName + LLTrans::getString("'") + " " + LLTrans::getString("InvOfferOwnedByGroup")
				+ " " + LLTrans::getString("'") + group_name + LLTrans::getString("'");
				chatHistory_string = mFromName + " " + LLTrans::getString("InvOfferOwnedByGroup")
				+ " " + group_name + LLTrans::getString("'") + LLTrans::getString(".");
			}
		}
		else
		{
			std::string full_name = LLAvatarActions::getSLURL(mFromID);
			if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) && (RlvUtil::isNearbyAgent(mFromID)) )
			{
				if (gCacheName->getFullName(mFromID, full_name))
				{
					full_name = RlvStrings::getAnonym(full_name);
				}
			}
			from_string = LLTrans::getString("InvOfferAnObjectNamed") + " " + LLTrans::getString("'") + mFromName + LLTrans::getString("'") + ' ';
			chatHistory_string = mFromName + ' ';
			if (full_name.empty())
			{
				from_string += LLTrans::getString("InvOfferOwnedByUnknownUser");
				chatHistory_string += LLTrans::getString("InvOfferOwnedByUnknownUser") + LLTrans::getString(".");
			}
			else
			{
				from_string += LLTrans::getString("InvOfferOwnedBy") + full_name;
				chatHistory_string += LLTrans::getString("InvOfferOwnedBy") + " " + full_name + LLTrans::getString(".");
			}
		}
	}
	else
	{
		from_string = chatHistory_string = mFromName;
	}
	bool busy = gAgent.isDoNotDisturb();
	bool fRlvNotifyAccepted = false;
	switch(button)
	{
	case IOR_ACCEPT:
		if ( (rlv_handler_t::isEnabled()) && (IM_TASK_INVENTORY_OFFERED == mIM) && (LLAssetType::AT_CATEGORY == mType) && (mDesc.find(RLV_PUTINV_PREFIX) == 1) )
		{
			fRlvNotifyAccepted = true;
			if (!RlvSettings::getForbidGiveToRLV())
			{
				const LLUUID& idRlvRoot = RlvInventory::instance().getSharedRootID();
				if (idRlvRoot.notNull())
					mFolderID = idRlvRoot;
				fRlvNotifyAccepted = false;
				RlvGiveToRLVTaskOffer* pOfferObserver = new RlvGiveToRLVTaskOffer(mTransactionID);
				gInventory.addObserver(pOfferObserver);
			}
		}
		msg->addU8Fast(_PREHASH_Dialog, (U8)(mIM + 1));
		msg->addBinaryDataFast(_PREHASH_BinaryBucket, &(mFolderID.mData),
					 sizeof(mFolderID.mData));
		msg->sendReliable(mHost);
			if (fRlvNotifyAccepted)
			{
				std::string::size_type idxToken = mDesc.find("'  ( http://");
				if (std::string::npos != idxToken)
					RlvBehaviourNotifyHandler::sendNotification("accepted_in_inv inv_offer " + mDesc.substr(1, idxToken - 1));
			}
		if (check_offer_throttle(mFromName, true))
		{
			log_message = chatHistory_string + ' ' + LLTrans::getString("InvOfferGaveYou") + ' ' + mDesc + LLTrans::getString(".");
			chat.mText = log_message;
			if (mFromObject || !mFromGroup)
				chat.mURL = mFromGroup ? LLGroupActions::getSLURL(mFromID) : LLAvatarActions::getSLURL(mFromID);
			chat.mFromName = mFromName;
			LLFloaterChat::addChatHistory(chat);
		}
		LL_DEBUGS("Messaging") << "Initializing an opener for tid: " << mTransactionID
				 << LL_ENDL;
		switch (mIM)
		{
		case IM_INVENTORY_OFFERED:
		{
			if ( (rlv_handler_t::isEnabled()) && (!RlvSettings::getForbidGiveToRLV()) && (LLAssetType::AT_CATEGORY == mType) && (mDesc.find(RLV_PUTINV_PREFIX) == 0) )
			{
				RlvGiveToRLVAgentOffer* pOfferObserver = new RlvGiveToRLVAgentOffer(mObjectID);
				pOfferObserver->startFetch();
				if (pOfferObserver->isFinished())
					pOfferObserver->done();
				else
					gInventory.addObserver(pOfferObserver);
			}
			LLOpenAgentOffer* open_agent_offer = new LLOpenAgentOffer(mObjectID, from_string);
			open_agent_offer->startFetch();
			if(catp || (itemp && itemp->isFinished()))
			{
				open_agent_offer->done();
			}
			else
			{
				opener = open_agent_offer;
			}
		}
			break;
		case IM_TASK_INVENTORY_OFFERED:
		case IM_GROUP_NOTICE:
		case IM_GROUP_NOTICE_REQUESTED:
		{
		}
		break;
		default:
			LL_WARNS("Messaging") << "inventory_offer_callback: unknown offer type" << LL_ENDL;
			break;
		}
		break;
	case -2:
	{
		LLStringUtil::format_map_t args;
		args["[DESC]"] = mDesc;
		args["[NAME]"] = mFromName;
		LLFloaterChat::addChatHistory(LLTrans::getString("InvOfferDeclineSilent", args));
	}
	break;
	case -1:
	{
		LLOpenAgentOffer* open_agent_offer = new LLOpenAgentOffer(mObjectID, from_string);
		open_agent_offer->startFetch();
		if(catp || (itemp && itemp->isFinished()))
		{
			open_agent_offer->done();
		}
		else
		{
			opener = open_agent_offer;
		}
		LLStringUtil::format_map_t args;
		args["[DESC]"] = mDesc;
		args["[NAME]"] = mFromName;
		LLFloaterChat::addChatHistory(LLTrans::getString("InvOfferAcceptSilent", args));
	}
	break;
	case IOR_BUSY:
		busy=TRUE;
	case IOR_MUTE:
	case IOR_DECLINE:
	default:
		msg->addU8Fast(_PREHASH_Dialog, (U8)(mIM + 2));
		msg->addBinaryDataFast(_PREHASH_BinaryBucket, EMPTY_BINARY_BUCKET, EMPTY_BINARY_BUCKET_SIZE);
		msg->sendReliable(mHost);
		if (!mFromGroup && gSavedSettings.getBOOL("LogInventoryDecline"))
		{
			if ( (rlv_handler_t::isEnabled()) &&
				 (IM_TASK_INVENTORY_OFFERED == mIM) && (LLAssetType::AT_CATEGORY == mType) && (mDesc.find(RLV_PUTINV_PREFIX) == 1) )
			{
				std::string::size_type idxToken = mDesc.find("'  ( http://");
				if (std::string::npos != idxToken)
					RlvBehaviourNotifyHandler::instance().sendNotification("declined inv_offer " + mDesc.substr(1, idxToken - 1));
			}
			LLStringUtil::format_map_t log_message_args;
			log_message_args["[DESC]"] = mDesc;
			log_message_args["[NAME]"] = mFromName;
			log_message = LLTrans::getString("InvOfferDecline", log_message_args);
			chat.mText = log_message;
			if( LLMuteList::getInstance()->isMuted(mFromID ) && ! LLMuteList::getInstance()->isLinden(mFromName) )
			{
				chat.mMuted = TRUE;
			}
			LLFloaterChat::addChatHistory(chat);
		}
		if(IM_INVENTORY_OFFERED == mIM)
		{
			LLDiscardAgentOffer* discard_agent_offer = new LLDiscardAgentOffer(mFolderID, mObjectID);
			discard_agent_offer->startFetch();
			if (catp || (itemp && itemp->isFinished()))
			{
				discard_agent_offer->done();
			}
			else
			{
				opener = discard_agent_offer;
			}
		}
		if (busy &&	(!mFromGroup && !mFromObject))
		{
			send_do_not_disturb_message(msg,mFromID);
		}
		break;
	}
	if(opener)
	{
		gInventory.addObserver(opener);
	}
	gFloaterView->resetStartingFloaterPosition();
	delete this;
	return false;
}
bool has_spam_bypass(bool is_friend, bool is_owned_by_me)
{
	static LLCachedControl<bool> antispam_not_mine(gSavedSettings,"AntiSpamNotMine");
	static LLCachedControl<bool> antispam_not_friend(gSavedSettings,"AntiSpamNotFriend");
	return (antispam_not_mine && is_owned_by_me) || (antispam_not_friend && is_friend);
}
bool is_spam_filtered(const EInstantMessage& dialog, bool is_friend, bool is_owned_by_me)
{
	if (has_spam_bypass(is_friend, is_owned_by_me)) return false;
	static LLCachedControl<bool> antispam(gSavedSettings,"_NACL_Antispam");
	if (antispam) return true;
	switch(dialog)
	{
	case IM_GROUP_NOTICE:
	case IM_GROUP_NOTICE_REQUESTED:
	{
		static const LLCachedControl<bool> filter("AntiSpamGroupNotices");
		return filter;
	}
	case IM_GROUP_INVITATION:
	{
		static const LLCachedControl<bool> filter("AntiSpamGroupInvites");
		return filter;
	}
	case IM_INVENTORY_OFFERED:
	case IM_TASK_INVENTORY_OFFERED:
	{
		static const LLCachedControl<bool> filter("AntiSpamItemOffers");
		return filter;
	}
	case IM_FROM_TASK_AS_ALERT:
	{
		static const LLCachedControl<bool> filter("AntiSpamAlerts");
		return filter;
	}
	case IM_LURE_USER:
	{
		static const LLCachedControl<bool> filter("AntiSpamTeleports");
		return filter;
	}
	case IM_TELEPORT_REQUEST:
	{
		static const LLCachedControl<bool> filter("AntiSpamTeleportRequests");
		return filter;
	}
	case IM_FRIENDSHIP_OFFERED:
	{
		static const LLCachedControl<bool> filter("AntiSpamFriendshipOffers");
		return filter;
	}
	case IM_COUNT:
	{
		static const LLCachedControl<bool> filter( "AntiSpamScripts");
		return filter;
	}
	default:
		return false;
	}
}
bool lure_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = 0;
	if (response.isInteger())
	{
		option = response.asInteger();
	}
	else
	{
		option = LLNotificationsUtil::getSelectedOption(notification, response);
	}
	LLUUID from_id = notification["payload"]["from_id"].asUUID();
	LLUUID lure_id = notification["payload"]["lure_id"].asUUID();
	BOOL godlike = notification["payload"]["godlike"].asBoolean();
	switch (option)
	{
	case 0:
		{
			gAgent.teleportViaLure(lure_id, godlike);
		}
		break;
	case 1:
	default:
		send_simple_im(from_id,
					   LLStringUtil::null,
					   IM_LURE_DECLINED,
					   lure_id);
		break;
	}
	return false;
}
static LLNotificationFunctorRegistration lure_callback_reg("TeleportOffered", lure_callback);
bool goto_url_callback(const LLSD& notification, const LLSD& response)
{
	std::string url = notification["payload"]["url"].asString();
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(1 == option)
	{
		LLWeb::loadURL(url);
	}
	return false;
}
static LLNotificationFunctorRegistration goto_url_callback_reg("GotoURL", goto_url_callback);
void process_improved_im(LLMessageSystem *msg, void **user_data)
{
	if (gNoRender)
	{
		return;
	}
	LLUUID from_id;
	BOOL from_group;
	LLUUID to_id;
	U8 offline;
	U8 d = 0;
	LLUUID session_id;
	U32 timestamp;
    std::string agentName;
	std::string message;
	U32 parent_estate_id = 0;
	LLUUID region_id;
	LLVector3 position;
	U8 binary_bucket[MTUBYTES];
	S32 binary_bucket_size;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, from_id);
	msg->getBOOLFast(_PREHASH_MessageBlock, _PREHASH_FromGroup, from_group);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ToAgentID, to_id);
	msg->getU8Fast(_PREHASH_MessageBlock, _PREHASH_Offline, offline);
	msg->getU8Fast(_PREHASH_MessageBlock, _PREHASH_Dialog, d);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ID, session_id);
	msg->getU32Fast(_PREHASH_MessageBlock, _PREHASH_Timestamp, timestamp);
    msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_FromAgentName, agentName);
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_Message,		message);
	auto antispam = NACLAntiSpamRegistry::getIfExists();
	if (antispam && can_block(from_id))
	{
		static const LLCachedControl<U32> SpamNewlines("_NACL_AntiSpamNewlines");
		boost::sregex_iterator iter(message.begin(), message.end(), NEWLINES);
		if((U32)std::abs(std::distance(iter, boost::sregex_iterator())) > SpamNewlines)
		{
			antispam->blockOnQueue(NACLAntiSpamRegistry::QUEUE_IM, from_id);
			LL_INFOS() << "[antispam] blocked owner due to too many newlines: " << from_id << LL_ENDL;
			if (gSavedSettings.getBOOL("AntiSpamNotify"))
			{
				LLSD args;
				args["SOURCE"] = from_id;
				args["AMOUNT"] = fmt::to_string(SpamNewlines);
				LLNotificationsUtil::add("AntiSpamNewlineFlood", args);
			}
			return;
		}
	}
	msg->getU32Fast(_PREHASH_MessageBlock, _PREHASH_ParentEstateID, parent_estate_id);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_RegionID, region_id);
	msg->getVector3Fast(_PREHASH_MessageBlock, _PREHASH_Position, position);
	msg->getBinaryDataFast(_PREHASH_MessageBlock, _PREHASH_BinaryBucket, binary_bucket, 0, 0, MTUBYTES);
	binary_bucket_size = msg->getSizeFast(_PREHASH_MessageBlock, _PREHASH_BinaryBucket);
	EInstantMessage dialog = (EInstantMessage)d;
	if (antispam && (dialog != IM_TYPING_START && dialog != IM_TYPING_STOP)
	&& antispam->checkQueue(NACLAntiSpamRegistry::QUEUE_IM, from_id))
		return;
    LLHost sender = msg->getSender();
    LLIMProcessing::processNewMessage(from_id,
        from_group,
        to_id,
        offline,
        dialog,
        session_id,
        timestamp,
        agentName,
        message,
        parent_estate_id,
        region_id,
        position,
        binary_bucket,
        binary_bucket_size,
        sender);
}
void send_do_not_disturb_message(LLMessageSystem* msg, const LLUUID& from_id, const LLUUID& session_id)
{
	if (gAgent.isDoNotDisturb())
	{
		std::string my_name;
		LLAgentUI::buildFullname(my_name);
		std::string name;
		gCacheName->getFullName(from_id, name);
		std::string response = gSavedPerAccountSettings.getString("BusyModeResponse");
		pack_instant_message(
			msg,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			from_id,
			my_name,
			replace_wildcards(response, from_id, name),
			IM_ONLINE,
			IM_BUSY_AUTO_RESPONSE,
			session_id);
		gAgent.sendReliableMessage();
		LLAvatarName av_name;
		std::string ns_name = LLAvatarNameCache::get(from_id, &av_name) ? av_name.getNSName() : name;
		const auto show = gSavedPerAccountSettings.getBOOL("BusyModeResponseShow");
		if (show) gIMMgr->addMessage(session_id, from_id, name, LLTrans::getString("IM_autoresponded_to") + ' ' + ns_name);
		if (!gSavedPerAccountSettings.getBOOL("BusyModeResponseItem")) return;
		if (LLViewerInventoryItem* item = gInventory.getItem(static_cast<LLUUID>(gSavedPerAccountSettings.getString("BusyModeResponseItemID"))))
		{
			LLGiveInventory::doGiveInventoryItem(from_id, item, session_id);
			if (show)
				gIMMgr->addMessage(session_id, from_id, name, llformat("%s %s \"%s\"", ns_name.c_str(), LLTrans::getString("IM_autoresponse_sent_item").c_str(), item->getName().c_str()));
		}
	}
}
bool callingcard_offer_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLUUID fid;
	LLMessageSystem* msg = gMessageSystem;
	switch (option)
	{
	case 0:
		msg->newMessageFast(_PREHASH_AcceptCallingCard);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, notification["payload"]["transaction_id"].asUUID());
		fid = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
		msg->nextBlockFast(_PREHASH_FolderData);
		msg->addUUIDFast(_PREHASH_FolderID, fid);
		msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));
		break;
	case 1:
		msg->newMessageFast(_PREHASH_DeclineCallingCard);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, notification["payload"]["transaction_id"].asUUID());
		msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));
		send_do_not_disturb_message(msg, notification["payload"]["source_id"].asUUID());
		break;
	default:
		break;
	}
	return false;
}
static LLNotificationFunctorRegistration callingcard_offer_cb_reg("OfferCallingCard", callingcard_offer_callback);
void process_offer_callingcard(LLMessageSystem* msg, void**)
{
	if (is_spam_filtered(IM_FRIENDSHIP_OFFERED, false, false))
		return;
	LL_DEBUGS("Messaging") << "callingcard offer" << LL_ENDL;
	LLUUID source_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, source_id);
	if (auto antispam = NACLAntiSpamRegistry::getIfExists())
		if (antispam->checkQueue(NACLAntiSpamRegistry::QUEUE_CALLING_CARD, source_id))
			return;
	LLUUID tid;
	msg->getUUIDFast(_PREHASH_AgentBlock, _PREHASH_TransactionID, tid);
	LLSD payload;
	payload["transaction_id"] = tid;
	payload["source_id"] = source_id;
	payload["sender"] = msg->getSender().getIPandPort();
	LLViewerObject* source = gObjectList.findObject(source_id);
	LLSD args;
	std::string source_name;
	if (source && source->isAvatar())
	{
		LLNameValue* nvfirst = source->getNVPair("FirstName");
		LLNameValue* nvlast  = source->getNVPair("LastName");
		if (nvfirst && nvlast)
		{
			source_name = LLCacheName::buildFullName(
				nvfirst->getString(), nvlast->getString());
		}
	}
	if (!source_name.empty())
	{
		if (gAgent.isDoNotDisturb()
			|| LLMuteList::getInstance()->isMuted(source_id, source_name, LLMute::flagTextChat))
		{
			LLNotifications::instance().forceResponse(LLNotification::Params("OfferCallingCard").payload(payload), 1);
		}
		else
		{
			args["NAME"] = source_name;
			LLNotificationsUtil::add("OfferCallingCard", args, payload);
		}
	}
	else
	{
		LL_WARNS("Messaging") << "Calling card offer from an unknown source." << LL_ENDL;
	}
}
void process_accept_callingcard(LLMessageSystem* msg, void**)
{
	LLNotificationsUtil::add("CallingCardAccepted");
}
void process_decline_callingcard(LLMessageSystem* msg, void**)
{
	LLNotificationsUtil::add("CallingCardDeclined");
}
#if 0
class ChatTranslationReceiver final : public LLTranslate::TranslationReceiver
{
public :
	ChatTranslationReceiver(const std::string &fromLang, const std::string &toLang, LLChat *chat,
		const BOOL history)
		: LLTranslate::TranslationReceiver(fromLang, toLang),
		m_chat(chat),
		m_history(history)
	{
	}
	static boost::intrusive_ptr<ChatTranslationReceiver> build(const std::string &fromLang, const std::string &toLang, LLChat *chat, const BOOL history)
	{
		return boost::intrusive_ptr<ChatTranslationReceiver>(new ChatTranslationReceiver(fromLang, toLang, chat, history));
	}
protected:
	void handleResponse(const std::string &translation, const std::string &detectedLanguage)
	{
		if (m_toLang != detectedLanguage)
			m_chat->mText += " (" + translation + ")";
		add_floater_chat(*m_chat, m_history);
		delete m_chat;
	}
	void handleFailure()
	{
		LLTranslate::TranslationReceiver::handleFailure();
		m_chat->mText += " (?)";
		add_floater_chat(*m_chat, m_history);
		delete m_chat;
	}
	char const* getName() const override { return "ChatTranslationReceiver"; }
private:
	LLChat *m_chat;
	const BOOL m_history;
};
#endif
void add_floater_chat(LLChat &chat, const bool history)
{
	if (history)
	{
		LLFloaterChat::addChatHistory(chat);
	}
	else
	{
		LLFloaterChat::addChat(chat, FALSE, FALSE);
	}
}
#if 0
void check_translate_chat(const std::string &mesg, LLChat &chat, const bool history)
{
	const bool translate = LLUI::sConfigGroup->getBOOL("TranslateChat");
	if (translate && chat.mSourceType != CHAT_SOURCE_SYSTEM)
	{
		const std::string &fromLang = "";
		const std::string &toLang = LLTranslate::getTranslateLanguage();
		LLChat *newChat = new LLChat(chat);
		LLHTTPClient::ResponderPtr result = ChatTranslationReceiver::build(fromLang, toLang, newChat, history);
		LLTranslate::translateMessage(result, fromLang, toLang, mesg);
	}
	else
	{
		add_floater_chat(chat, history);
	}
}
#endif
void process_chat_from_simulator(LLMessageSystem* msg, void** user_data)
{
	LLChat	chat;
	std::string		mesg;
	std::string		from_name;
	U8			source_temp;
	U8			type_temp;
	U8			audible_temp;
	LLColor4	color(1.0f, 1.0f, 1.0f, 1.0f);
	LLUUID		from_id;
	LLUUID		owner_id;
	BOOL		is_owned_by_me = FALSE;
	LLViewerObject*	chatter;
	msg->getString("ChatData", "FromName", from_name);
	LLStringUtil::trim(from_name);
	if (from_name.empty())
	{
		from_name = LLTrans::getString("Unnamed");
	}
	msg->getUUID("ChatData", "SourceID", from_id);
	chat.mFromID = from_id;
	chatter = gObjectList.findObject(from_id);
	if(chatter && chatter->isAvatar())
	{
		((LLVOAvatar*)chatter)->mIdleTimer.reset();
	}
	msg->getUUID("ChatData", "OwnerID", owner_id);
	bool has_owner = owner_id.notNull();
	if (chatter && has_owner) chatter->mOwnerID = owner_id;
	msg->getU8Fast(_PREHASH_ChatData, _PREHASH_SourceType, source_temp);
	chat.mSourceType = (EChatSourceType)source_temp;
	msg->getU8("ChatData", "ChatType", type_temp);
	chat.mChatType = (EChatType)type_temp;
	auto antispam = NACLAntiSpamRegistry::getIfExists();
	if (antispam && chat.mChatType != CHAT_TYPE_START && chat.mChatType != CHAT_TYPE_STOP
	&& (antispam->checkQueue(NACLAntiSpamRegistry::QUEUE_CHAT, from_id, !has_owner ? LFIDBearer::AVATAR : LFIDBearer::OBJECT)
	|| (has_owner && (antispam->checkQueue(NACLAntiSpamRegistry::QUEUE_CHAT, owner_id)))))
		return;
	msg->getU8Fast(_PREHASH_ChatData, _PREHASH_Audible, audible_temp);
	chat.mAudible = (EChatAudible)audible_temp;
	chat.mTime = LLFrameTimer::getElapsedSeconds();
	if (chat.mSourceType == CHAT_SOURCE_AGENT)
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(from_id, &av_name))
		{
			chat.mFromName = av_name.getNSName();
		}
		else
		{
			chat.mFromName = LLCacheName::cleanFullName(from_name);
		}
	}
	else
	{
		LLStringUtil::trim(from_name);
		if (from_name.empty())
		{
			from_name = LLTrans::getString("Unnamed");
		}
		chat.mFromName = from_name;
	}
	bool is_do_not_disturb = gAgent.isDoNotDisturb();
	bool is_muted = false;
	bool is_linden = false;
	is_muted = LLMuteList::getInstance()->isMuted(
		from_id,
		from_name,
		LLMute::flagTextChat)
		|| LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagTextChat);
	is_linden = chat.mSourceType != CHAT_SOURCE_OBJECT &&
		LLMuteList::getInstance()->isLinden(from_name);
	msg->getStringFast(_PREHASH_ChatData, _PREHASH_Message, mesg);
	bool handle_obj_auth(const LLUUID& from_id, const std::string& mesg);
	if (source_temp == CHAT_SOURCE_OBJECT && type_temp == CHAT_TYPE_OWNER && handle_obj_auth(from_id, mesg)) return;
	if (is_muted && (chat.mSourceType == CHAT_SOURCE_OBJECT))
	{
		return;
	}
	BOOL is_audible = (CHAT_AUDIBLE_FULLY == chat.mAudible);
	if (chatter)
	{
		chat.mPosAgent = chatter->getPositionAgent();
		if ( ((chat.mSourceType == CHAT_SOURCE_OBJECT) && (chat.mChatType != CHAT_TYPE_DEBUG_MSG)) &&
			 (gSavedSettings.getBOOL("EffectScriptChatParticles")) &&
			 ((!rlv_handler_t::isEnabled()) || (CHAT_TYPE_OWNER != chat.mChatType)) )
		{
			LLPointer<LLViewerPartSourceChat> psc = new LLViewerPartSourceChat(chatter->getPositionAgent());
			psc->setSourceObject(chatter);
			psc->setColor(color);
			psc->setOwnerUUID(owner_id);
			LLViewerPartSim::getInstance()->addPartSource(psc);
		}
		if (is_audible
			&& (is_linden || (!is_muted && !is_do_not_disturb)))
		{
			if (chat.mChatType != CHAT_TYPE_START
				&& chat.mChatType != CHAT_TYPE_STOP)
			{
				gAgent.heardChat(chat.mFromID);
			}
		}
		is_owned_by_me = chatter->permYouOwner();
	}
	else is_owned_by_me = owner_id == gAgentID;
	U32 links_for_chatting_objects = gSavedSettings.getU32("LinksForChattingObjects");
	if (links_for_chatting_objects != 0 && chat.mSourceType == CHAT_SOURCE_OBJECT &&
		(!is_owned_by_me || links_for_chatting_objects == 2)
		&& !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)
		)
	{
		LLSD query_string;
		query_string["name"]  = from_name;
		query_string["owner"] = owner_id;
		if( !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC) )
		{
			const LLViewerObject* obj(chatter ? chatter : gObjectList.findObject(owner_id));
			LLVector3 pos = obj ? obj->getPositionRegion() : LLVector3::zero;
			S32 x = ll_round((F32)fmod((F64)pos.mV[VX], (F64)REGION_WIDTH_METERS));
			S32 y = ll_round((F32)fmod((F64)pos.mV[VY], (F64)REGION_WIDTH_METERS));
			S32 z = ll_round((F32)pos.mV[VZ]);
			std::ostringstream location;
			location << (obj ? obj->getRegion() : gAgent.getRegion())->getName() << "/" << x << "/" << y << "/" << z;
			if (chatter != obj) location << "?owner_not_object";
			query_string["slurl"] = location.str();
		}
		std::ostringstream link;
		link << "secondlife:///app/objectim/" << from_id << LLURI::mapToQueryString(query_string);
		chat.mURL = link.str();
	}
	if (is_audible)
	{
		if (antispam && can_block(from_id))
		{
			static const LLCachedControl<U32> SpamNewlines("_NACL_AntiSpamNewlines");
			boost::sregex_iterator iter(mesg.begin(), mesg.end(), NEWLINES);
			if((U32)std::abs(std::distance(iter, boost::sregex_iterator())) > SpamNewlines)
			{
				antispam->blockOnQueue(NACLAntiSpamRegistry::QUEUE_CHAT, owner_id);
				if(gSavedSettings.getBOOL("AntiSpamNotify"))
				{
					LLSD args;
					args["MESSAGE"] = "Chat: Blocked newline flood from " + LLAvatarActions::getSLURL(owner_id);
					LLNotificationsUtil::add("SystemMessageTip", args);
				}
				return;
			}
		}
		if (chatter && chatter->isAvatar())
		{
			if (LLAvatarNameCache::getNSName(from_id, from_name))
				chat.mFromName = from_name;
		}
		BOOL visible_in_chat_bubble = FALSE;
		std::string verb;
		color.setVec(1.f, 1.f, 1.f, 1.f);
		if ( (rlv_handler_t::isEnabled()) && (CHAT_TYPE_START != chat.mChatType) && (CHAT_TYPE_STOP != chat.mChatType) )
		{
			BOOL is_attachment = (chatter) ? chatter->isAttachment() : FALSE;
			if ( ( (CHAT_SOURCE_AGENT == chat.mSourceType) && (from_id != gAgent.getID()) ) ||
				 ( (CHAT_SOURCE_OBJECT == chat.mSourceType) && ((!is_owned_by_me) || (!is_attachment)) &&
				   (CHAT_TYPE_OWNER != chat.mChatType) && (CHAT_TYPE_DIRECT != chat.mChatType) ) )
			{
				bool fIsEmote = RlvUtil::isEmote(mesg);
				if ((!fIsEmote) &&
					(((gRlvHandler.hasBehaviour(RLV_BHVR_RECVCHAT)) && (!gRlvHandler.isException(RLV_BHVR_RECVCHAT, from_id))) ||
					 ((gRlvHandler.hasBehaviour(RLV_BHVR_RECVCHATFROM)) && (gRlvHandler.isException(RLV_BHVR_RECVCHATFROM, from_id))) ))
				{
					if ( (gRlvHandler.filterChat(mesg, false)) && (!gSavedSettings.getBOOL("RestrainedLoveShowEllipsis")) )
						return;
				}
				else if ((fIsEmote) &&
					     (((gRlvHandler.hasBehaviour(RLV_BHVR_RECVEMOTE)) && (!gRlvHandler.isException(RLV_BHVR_RECVEMOTE, from_id))) ||
					      ((gRlvHandler.hasBehaviour(RLV_BHVR_RECVEMOTEFROM)) && (gRlvHandler.isException(RLV_BHVR_RECVEMOTEFROM, from_id))) ))
				{
					if (!gSavedSettings.getBOOL("RestrainedLoveShowEllipsis"))
						return;
					mesg = "/me ...";
				}
			}
			if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
			{
				if (CHAT_SOURCE_AGENT != chat.mSourceType)
				{
					RlvUtil::filterNames(chat.mFromName);
				}
				else if (chat.mFromID != gAgentID)
				{
					chat.mFromName = RlvStrings::getAnonym(chat.mFromName);
					chat.mRlvNamesFiltered = TRUE;
				}
			}
			if ( (CHAT_SOURCE_OBJECT == chat.mSourceType) &&
				 ((gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) || (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))) )
			{
				LLSD sdQuery;
				sdQuery["name"] = chat.mFromName;
				sdQuery["owner"] = owner_id;
				if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) && (!is_owned_by_me) )
					sdQuery["rlv_shownames"] = true;
				const LLViewerRegion* pRegion = LLWorld::getInstance()->getRegionFromPosAgent(chat.mPosAgent);
				if (pRegion)
					sdQuery["slurl"] = LLSLURL(pRegion->getName(), chat.mPosAgent).getLocationString();
				chat.mURL = LLSLURL("objectim", from_id, LLURI::mapToQueryString(sdQuery)).getSLURLString();
			}
		}
		BOOL ircstyle = FALSE;
		std::string prefix = mesg.substr(0, 4);
		if (boost::iequals(prefix, "/me ") || boost::iequals(prefix, "/me'"))
		{
			chat.mText = chat.mFromName;
			mesg = mesg.substr(3);
			ircstyle = TRUE;
			chat.mChatStyle = CHAT_STYLE_IRC;
		}
		chat.mText += mesg;
		if (CHAT_TYPE_START == chat.mChatType)
		{
			LLLocalSpeakerMgr::getInstance()->setSpeakerTyping(from_id, TRUE);
			if (chatter && chatter->isAvatar())
			{
				((LLVOAvatar*)chatter)->startTyping();
			}
			return;
		}
		if (CHAT_TYPE_STOP == chat.mChatType)
		{
			LLLocalSpeakerMgr::getInstance()->setSpeakerTyping(from_id, FALSE);
			if (chatter && chatter->isAvatar())
			{
				((LLVOAvatar*)chatter)->stopTyping();
			}
			return;
		}
		if (chatter && chatter->isAvatar())
		{
			LLLocalSpeakerMgr::getInstance()->setSpeakerTyping(from_id, FALSE);
			static_cast<LLVOAvatar*>(chatter)->stopTyping();
			if (!is_muted )
			{
				static const LLCachedControl<bool> use_chat_bubbles("UseChatBubbles",false);
				visible_in_chat_bubble = use_chat_bubbles;
				static_cast<LLVOAvatar*>(chatter)->addChat(chat);
			}
		}
		if (ircstyle)
		{
		}
		else
		{
			switch (chat.mChatType)
			{
			case CHAT_TYPE_WHISPER:
				verb = " " + LLTrans::getString("whisper") + " ";
				break;
			case CHAT_TYPE_OWNER:
				if ( (rlv_handler_t::isEnabled()) && (mesg.length() > 3) && (RLV_CMD_PREFIX == mesg[0]) && (CHAT_TYPE_OWNER == chat.mChatType) )
				{
					mesg.erase(0, 1);
					LLStringUtil::toLower(mesg);
					std::string strExecuted, strFailed, strRetained, *pstr;
					boost_tokenizer tokens(mesg, boost::char_separator<char>(",", "", boost::drop_empty_tokens));
					for (auto itToken = tokens.begin(); itToken != tokens.end(); ++itToken)
					{
						std::string strCmd = *itToken;
						ERlvCmdRet eRet = gRlvHandler.processCommand(from_id, strCmd, true);
						if ( (RlvSettings::getDebug()) &&
							 ( (!RlvSettings::getDebugHideUnsetDup()) ||
							   ((RLV_RET_SUCCESS_UNSET != eRet) && (RLV_RET_SUCCESS_DUPLICATE != eRet)) ) )
						{
							if ( RLV_RET_SUCCESS == (eRet & RLV_RET_SUCCESS) )
								pstr = &strExecuted;
							else if ( RLV_RET_FAILED == (eRet & RLV_RET_FAILED) )
								pstr = &strFailed;
							else if (RLV_RET_RETAINED == eRet)
								pstr = &strRetained;
							else
							{
								RLV_ASSERT(false);
								pstr = &strFailed;
							}
							const char* pstrSuffix = RlvStrings::getStringFromReturnCode(eRet);
							if (pstrSuffix)
								strCmd.append(" (").append(pstrSuffix).append(")");
							if (!pstr->empty())
								pstr->push_back(',');
							pstr->append(strCmd);
						}
					}
					if (RlvForceWear::instanceExists())
						RlvForceWear::instance().done();
					if ( (!RlvSettings::getDebug()) || ((strExecuted.empty()) && (strFailed.empty()) && (strRetained.empty())) )
						return;
					if ( (!strExecuted.empty()) && (strFailed.empty()) && (strRetained.empty()) )
					{
						verb = " executes: @";
						mesg = strExecuted;
					}
					else if ( (strExecuted.empty()) && (!strFailed.empty()) && (strRetained.empty()) )
					{
						verb = " failed: @";
						mesg = strFailed;
					}
					else if ( (strExecuted.empty()) && (strFailed.empty()) && (!strRetained.empty()) )
					{
						verb = " retained: @";
						mesg = strRetained;
					}
					else
					{
						verb = ": @";
						if (!strExecuted.empty())
							mesg += "\n    - executed: @" + strExecuted;
						if (!strFailed.empty())
							mesg += "\n    - failed: @" + strFailed;
						if (!strRetained.empty())
							mesg += "\n    - retained: @" + strRetained;
					}
					break;
				}
#if SHY_MOD
				if(SHCommandHandler::handleCommand(false,mesg,from_id,chatter))
					return;
#endif
				if  ( (rlv_handler_t::isEnabled()) && (chatter) && (chat.mSourceType == CHAT_SOURCE_OBJECT) &&
					  (gSavedSettings.getBOOL("EffectScriptChatParticles")) )
				{
					LLPointer<LLViewerPartSourceChat> psc = new LLViewerPartSourceChat(chatter->getPositionAgent());
					psc->setSourceObject(chatter);
					psc->setColor(color);
					psc->setOwnerUUID(owner_id);
					LLViewerPartSim::getInstance()->addPartSource(psc);
				}
			case CHAT_TYPE_DEBUG_MSG:
			case CHAT_TYPE_NORMAL:
			case CHAT_TYPE_DIRECT:
				verb = ": ";
				break;
			case CHAT_TYPE_SHOUT:
				verb = " " + LLTrans::getString("shout") + " ";
				break;
			case CHAT_TYPE_START:
			case CHAT_TYPE_STOP:
				LL_WARNS("Messaging") << "Got chat type start/stop in main chat processing." << LL_ENDL;
				break;
			default:
				LL_WARNS("Messaging") << "Unknown type " << chat.mChatType << " in chat!" << LL_ENDL;
				verb = ": ";
				break;
			}
			chat.mText = chat.mFromName + verb + mesg;
		}
		if (chatter)
		{
			chat.mPosAgent = chatter->getPositionAgent();
		}
		chat.mMuted = is_muted && !is_linden;
		bool only_history = visible_in_chat_bubble || (!is_linden && !is_owned_by_me && is_do_not_disturb);
#if 0
		if (!chat.mMuted)
		{
			check_translate_chat(mesg, chat, only_history);
		}
#else
		LLChatAITranslate::instance().handleIncomingNearby(chat, only_history, mesg);
#endif
	}
}
void process_teleport_start(LLMessageSystem* msg, void**)
{
	U32 teleport_flags = 0x0;
	msg->getU32("Info", "TeleportFlags", teleport_flags);
	if (gAgent.getTeleportState() == LLAgent::TELEPORT_MOVING)
	{
		LL_WARNS("Messaging") << "Got TeleportStart, but teleport already in progress. TeleportFlags=" << teleport_flags << LL_ENDL;
	}
	LL_DEBUGS("Messaging") << "Got TeleportStart with TeleportFlags=" << teleport_flags << ". gTeleportDisplay: " << gTeleportDisplay << ", gAgent.mTeleportState: " << gAgent.getTeleportState() << LL_ENDL;
	if ( (teleport_flags & TELEPORT_FLAGS_DISABLE_CANCEL) || (!gRlvHandler.getCanCancelTp()) )
	{
		gViewerWindow->setProgressCancelButtonVisible(FALSE);
	}
	else
	{
		gViewerWindow->setProgressCancelButtonVisible(TRUE, LLTrans::getString("Cancel"));
	}
	if (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE)
	{
		gTeleportDisplay = TRUE;
		gAgent.setTeleportState(LLAgent::TELEPORT_START);
		make_ui_sound("UISndTeleportOut");
		LL_INFOS("Messaging") << "Teleport initiated by remote TeleportStart message with TeleportFlags: " <<  teleport_flags << LL_ENDL;
	}
}
void process_teleport_progress(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	if ((gAgent.getID() != agent_id)
	   || (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE))
	{
		LL_WARNS("Messaging") << "Unexpected teleport progress message." << LL_ENDL;
		return;
	}
	U32 teleport_flags = 0x0;
	msg->getU32("Info", "TeleportFlags", teleport_flags);
	if ( (teleport_flags & TELEPORT_FLAGS_DISABLE_CANCEL) || (!gRlvHandler.getCanCancelTp()) )
	{
		gViewerWindow->setProgressCancelButtonVisible(FALSE);
	}
	else
	{
		gViewerWindow->setProgressCancelButtonVisible(TRUE, LLTrans::getString("Cancel"));
	}
	std::string buffer;
	msg->getString("Info", "Message", buffer);
	LL_DEBUGS("Messaging") << "teleport progress: " << buffer << " flags: " << teleport_flags << LL_ENDL;
	std::string message = buffer;
	if (LLAgent::sTeleportProgressMessages.find(buffer) !=
		LLAgent::sTeleportProgressMessages.end())
	{
		message = LLAgent::sTeleportProgressMessages[buffer];
	}
	gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages[message]);
}
class LLFetchInWelcomeArea final : public LLInventoryFetchDescendentsObserver
{
public:
	LLFetchInWelcomeArea(const uuid_vec_t& ids) :
		LLInventoryFetchDescendentsObserver(ids)
	{
	}
	void done() override
	{
		LLIsType is_landmark(LLAssetType::AT_LANDMARK);
		LLIsType is_card(LLAssetType::AT_CALLINGCARD);
		LLInventoryModel::cat_array_t	card_cats;
		LLInventoryModel::item_array_t	card_items;
		LLInventoryModel::cat_array_t	land_cats;
		LLInventoryModel::item_array_t	land_items;
		uuid_vec_t::iterator it = mComplete.begin();
		uuid_vec_t::iterator end = mComplete.end();
		for (; it != end; ++it)
		{
			gInventory.collectDescendentsIf(
				(*it),
				land_cats,
				land_items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_landmark);
			gInventory.collectDescendentsIf(
				(*it),
				card_cats,
				card_items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_card);
		}
		LLSD args;
		if (!land_items.empty())
		{
			S32 random_land = ll_rand(land_items.size() - 1);
			args["NAME"] = land_items[random_land]->getName();
			LLNotificationsUtil::add("TeleportToLandmark",args);
		}
		if (!card_items.empty())
		{
			S32 random_card = ll_rand(card_items.size() - 1);
			args["NAME"] = card_items[random_card]->getName();
			LLNotificationsUtil::add("TeleportToPerson",args);
		}
		gInventory.removeObserver(this);
		delete this;
	}
};
class LLPostTeleportNotifiers final : public LLEventTimer
{
public:
	LLPostTeleportNotifiers();
	virtual ~LLPostTeleportNotifiers();
	BOOL tick() override;
};
LLPostTeleportNotifiers::LLPostTeleportNotifiers() : LLEventTimer(2.0)
{
};
LLPostTeleportNotifiers::~LLPostTeleportNotifiers()
{
}
BOOL LLPostTeleportNotifiers::tick()
{
	BOOL all_done = FALSE;
	if (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE)
	{
		uuid_vec_t folders;
		const LLUUID callingcard_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
		if (callingcard_id.notNull())
			folders.push_back(callingcard_id);
		const LLUUID folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
		if (folder_id.notNull())
			folders.push_back(folder_id);
		if (!folders.empty())
		{
			LLFetchInWelcomeArea* fetcher = new LLFetchInWelcomeArea(folders);
			fetcher->startFetch();
			if (fetcher->isFinished())
			{
				fetcher->done();
			}
			else
			{
				gInventory.addObserver(fetcher);
			}
		}
		all_done = TRUE;
	}
	return all_done;
}
void process_teleport_finish(LLMessageSystem* msg, void**)
{
	LL_INFOS("Teleport", "Messaging") << "Received TeleportFinish" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_Info, _PREHASH_AgentID, agent_id);
	if (agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "Got teleport notification for wrong agent!" << LL_ENDL;
		return;
	}
	gViewerWindow->setProgressCancelButtonVisible(FALSE);
	LLHUDEffectSpiral* effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
	effectp->setPositionGlobal(gAgent.getPositionGlobal());
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	LLHUDManager::getInstance()->sendEffects();
	U32 location_id;
	U32 sim_ip;
	U16 sim_port;
	LLVector3 pos, look_at;
	U64 region_handle;
	msg->getU32Fast(_PREHASH_Info, _PREHASH_LocationID, location_id);
	msg->getIPAddrFast(_PREHASH_Info, _PREHASH_SimIP, sim_ip);
	msg->getIPPortFast(_PREHASH_Info, _PREHASH_SimPort, sim_port);
	msg->getU64Fast(_PREHASH_Info, _PREHASH_RegionHandle, region_handle);
	U32 teleport_flags;
	msg->getU32Fast(_PREHASH_Info, _PREHASH_TeleportFlags, teleport_flags);
	std::string seedCap;
	msg->getStringFast(_PREHASH_Info, _PREHASH_SeedCapability, seedCap);
	if ((teleport_flags & TELEPORT_FLAGS_SET_HOME_TO_TARGET)
	   && (!gAgent.isGodlike()))
	{
		gAgent.setHomePosRegion(region_handle, pos);
		new LLPostTeleportNotifiers();
	}
	LLHost sim_host(sim_ip, sim_port);
	gMessageSystem->enableCircuit(sim_host, TRUE);
	if (!gHippoGridManager->getConnectedGrid()->isSecondLife())
	{
		U32 region_size_x = 256;
		msg->getU32Fast(_PREHASH_Info, _PREHASH_RegionSizeX, region_size_x);
		U32 region_size_y = 256;
		msg->getU32Fast(_PREHASH_Info, _PREHASH_RegionSizeY, region_size_y);
		LLWorld::getInstance()->setRegionSize(region_size_x, region_size_y);
	}
	LLViewerRegion* regionp =  LLWorld::getInstance()->addRegion(region_handle, sim_host);
	M7WindlightInterface::getInstance()->receiveReset();
	gAgent.standUp();
	LL_INFOS("Teleport", "Messaging") << "process_teleport_finish() Enabling "
			<< sim_host << " with code " << msg->mOurCircuitCode
			<< " seedCap " << seedCap << LL_ENDL;
	msg->newMessageFast(_PREHASH_UseCircuitCode);
	msg->nextBlockFast(_PREHASH_CircuitCode);
	msg->addU32Fast(_PREHASH_Code, msg->getOurCircuitCode());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_ID, gAgent.getID());
	msg->sendReliable(sim_host);
	LL_INFOS("Teleport") << "Calling send_complete_agent_movement() and setting state to TELEPORT_MOVING" << LL_ENDL;
	send_complete_agent_movement(sim_host);
	gAgent.setTeleportState(LLAgent::TELEPORT_MOVING);
	gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["contacting"]);
	LL_INFOS("CrossingCaps") << "Calling setSeedCapability from process_teleport_finish(). Seed cap == "
		<< seedCap << LL_ENDL;
	regionp->setSeedCapability(seedCap);
	effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
	effectp->setPositionGlobal(gAgent.getPositionGlobal());
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	LLHUDManager::getInstance()->sendEffects();
}
void process_agent_movement_complete(LLMessageSystem* msg, void**)
{
	gShiftFrame = true;
	gAgentMovementCompleted = true;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if ((gAgent.getID() != agent_id) || (gAgent.getSessionID() != session_id))
	{
		LL_WARNS("Messaging") << "Incorrect id in process_agent_movement_complete()"
				<< LL_ENDL;
		return;
	}
	LL_INFOS("Teleport", "Messaging") << "Received AgentMovementComplete" << LL_ENDL;
	LLVector3 agent_pos;
	msg->getVector3Fast(_PREHASH_Data, _PREHASH_Position, agent_pos);
	LLVector3 look_at;
	msg->getVector3Fast(_PREHASH_Data, _PREHASH_LookAt, look_at);
	U64 region_handle;
	msg->getU64Fast(_PREHASH_Data, _PREHASH_RegionHandle, region_handle);
	std::string version_channel;
	msg->getString("SimData", "ChannelVersion", version_channel);
	if (!isAgentAvatarValid())
	{
		LL_WARNS("Messaging") << "agent_movement_complete() with NULL avatarp." << LL_ENDL;
	}
	F32 x, y;
	from_region_handle(region_handle, &x, &y);
	LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromHandle(region_handle);
	if (!regionp)
	{
		if (gAgent.getRegion())
		{
			LL_WARNS("Messaging") << "current region " << gAgent.getRegion()->getOriginGlobal() << LL_ENDL;
		}
		LL_WARNS("Messaging") << "Agent being sent to invalid home region: "
			<< x << ":" << y
			<< " current pos " << gAgent.getPositionGlobal()
			<< LL_ENDL;
		LLAppViewer::instance()->forceDisconnect(LLTrans::getString("SentToInvalidRegion"));
		return;
	}
	LL_INFOS("Messaging") << "Changing home region to " << x << ":" << y << LL_ENDL;
	dump_hud_teleport_state("movement_complete_before_shift");
	LLVector3 shift_vector = regionp->getPosRegionFromGlobal(
		gAgent.getRegion()->getOriginGlobal());
	LL_INFOS("HUDTeleport") << "shift_vector=" << shift_vector
		<< " len=" << shift_vector.length()
		<< " dest_region=" << regionp->getName()
		<< LL_ENDL;
	gAgent.setRegion(regionp);
	gObjectList.shiftObjects(shift_vector);
	dump_hud_teleport_state("movement_complete_after_shift");
	// Firestorm only rebuilds spatial partitions on OpenSim long jumps
	// (FIRE-11593). Doing this on Second Life destroys HUD spatial groups
	// and can make HUDs vanish until a refresh.
	if (shift_vector.length() > 2048.f * 256.f &&
		!gHippoGridManager->getConnectedGrid()->isSecondLife())
	{
		regionp->reInitPartitions();
		gAgent.setRegion(regionp);
		for (auto r : LLWorld::getInstance()->getRegionList())
		{
			if (r != regionp)
			{
				gObjectList.killObjects(r);
			}
		}
		dump_hud_teleport_state("movement_complete_after_long_jump_cleanup");
	}
	gAssetStorage->setUpstream(msg->getSender());
	gCacheName->setUpstream(msg->getSender());
	gViewerThrottle.sendToSim();
	gViewerWindow->sendShapeToSim();
	bool is_teleport = gAgent.getTeleportState() == LLAgent::TELEPORT_MOVING;
	if (is_teleport)
	{
		if (!gSavedPerAccountSettings.getBOOL("AllynRenderFriendsOnlyPersistsTP"))
		{
			gSavedPerAccountSettings.setBOOL("AllynRenderFriendsOnly", false);
		}
		if (gAgent.getTeleportKeepsLookAt())
		{
			look_at = LLViewerCamera::getInstance()->getAtAxis();
		}
		gAgentCamera.setFocusOnAvatar(TRUE, FALSE);
		gAgentCamera.slamLookAt(look_at);
		gAgentCamera.updateCamera();
		gAgent.setTeleportState(LLAgent::TELEPORT_START_ARRIVAL);
		gAgent.sendAgentSetAppearance();
		if (isAgentAvatarValid())
		{
			LLSLURL slurl;
			gAgent.getTeleportSourceSLURL(slurl);
			LLChat chat(LLTrans::getString("completed_from") + " " + slurl.getSLURLString());
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChatHistory(chat);
			gAgentAvatarp->setPositionAgent(agent_pos);
			gAgentAvatarp->clearChat();
			gAgentAvatarp->slamPosition();
			gAgentAvatarp->resetHUDAttachments();
			dump_hud_teleport_state("movement_complete_after_resetHUD");
		}
		LLFloaterTeleportHistory::getInstance()->addPendingEntry(regionp->getName(), (S16)agent_pos.mV[VX], (S16)agent_pos.mV[VY], (S16)agent_pos.mV[VZ]);
	}
	else
	{
		gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
		if (LLStartUp::getStartupState() < STATE_STARTED)
		{
			LLVector3 look_at_point = look_at;
			look_at_point = agent_pos + look_at_point.rotVec(gAgent.getQuat());
			static LLVector3 up_direction(0.0f, 0.0f, 1.0f);
			LLViewerCamera::getInstance()->lookAt(agent_pos, look_at_point, up_direction);
		}
	}
	if (LLTracker::isTracking())
	{
		LLVector3d beacon_pos = LLTracker::getTrackedPositionGlobal();
		LLVector3 beacon_dir(agent_pos.mV[VX] - (F32)fmod(beacon_pos.mdV[VX], 256.0), agent_pos.mV[VY] - (F32)fmod(beacon_pos.mdV[VY], 256.0), 0);
		if (beacon_dir.magVecSquared() < 25.f)
		{
			LLTracker::stopTracking(false);
		}
		else if (is_teleport)
		{
			if (!gAgent.getTeleportKeepsLookAt() && look_at.isExactlyZero())
			{
				LLVector3 global_agent_pos = agent_pos;
				global_agent_pos[0] += x;
				global_agent_pos[1] += y;
				look_at = (LLVector3)beacon_pos - global_agent_pos;
				look_at.normVec();
				gAgentCamera.slamLookAt(look_at);
			}
			if (gSavedSettings.getBOOL("ClearBeaconAfterTeleport"))
			{
				LLTracker::stopTracking(false);
			}
		}
	}
	send_agent_update(TRUE, TRUE);
	if (gAgent.getRegion()->getBlockFly())
	{
		gAgent.setFlying(false);
	}
	gAgent.setDoNotDisturb(gAgent.isDoNotDisturb());
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->mFootPlane.clearVec();
	}
	gAgent.sendWalkRun();
	if (gLastVersionChannel == version_channel)
	{
		return;
	}
	if (!gLastVersionChannel.empty() && gSavedSettings.getBOOL("SGServerVersionChangedNotification"))
	{
		LLSD args;
		args["OLD_VERSION"] = gLastVersionChannel;
		args["NEW_VERSION"] = version_channel;
		LLNotificationsUtil::add("ServerVersionChanged", args);
	}
	gLastVersionChannel = version_channel;
}
void process_crossed_region(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if ((gAgent.getID() != agent_id) || (gAgent.getSessionID() != session_id))
	{
		LL_WARNS("Messaging") << "Incorrect id in process_crossed_region()"
				<< LL_ENDL;
		return;
	}
	LL_INFOS("Messaging") << "process_crossed_region()" << LL_ENDL;
	gAgentAvatarp->resetRegionCrossingTimer();
	gAgent.setIsCrossingRegion(false);
	U32 sim_ip;
	msg->getIPAddrFast(_PREHASH_RegionData, _PREHASH_SimIP, sim_ip);
	U16 sim_port;
	msg->getIPPortFast(_PREHASH_RegionData, _PREHASH_SimPort, sim_port);
	LLHost sim_host(sim_ip, sim_port);
	U64 region_handle;
	msg->getU64Fast(_PREHASH_RegionData, _PREHASH_RegionHandle, region_handle);
	std::string seedCap;
	msg->getStringFast(_PREHASH_RegionData, _PREHASH_SeedCapability, seedCap);
	send_complete_agent_movement(sim_host);
	if (!gHippoGridManager->getConnectedGrid()->isSecondLife())
	{
		U32 region_size_x = 256;
		msg->getU32(_PREHASH_RegionData, _PREHASH_RegionSizeX, region_size_x);
		U32 region_size_y = 256;
		msg->getU32(_PREHASH_RegionData, _PREHASH_RegionSizeY, region_size_y);
		LLWorld::getInstance()->setRegionSize(region_size_x, region_size_y);
	}
	LLViewerRegion* regionp = LLWorld::getInstance()->addRegion(region_handle, sim_host);
	LL_DEBUGS("CrossingCaps") << "Calling setSeedCapability from process_crossed_region(). Seed cap == "
		<< seedCap << LL_ENDL;
	regionp->setSeedCapability(seedCap);
}
const F32 THRESHOLD_HEAD_ROT_QDOT = 0.9997f;
const F32 MAX_HEAD_ROT_QDOT = 0.99999f;
static LLTrace::BlockTimerStatHandle FTM_AGENT_UPDATE_SEND("Send Message");
void send_agent_update(BOOL force_send, BOOL send_reliable)
{
	if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
	{
		return;
	}
	if (LLAppViewer::instance()->logoutRequestSent())
	{
		return;
	}
	if (gAgent.getRegion() == nullptr)
	{
		return;
	}
	const F32 TRANSLATE_THRESHOLD = 0.01f;
	const F32 ROTATION_THRESHOLD = 0.1f * 2.f * F_PI / 360.f;
	const U8 DUP_MSGS = 1;
	static LLVector3 last_camera_pos_agent,
					 last_camera_at,
					 last_camera_left,
					 last_camera_up;
	static LLVector3 cam_center_chg,
					 cam_rot_chg;
	static LLQuaternion last_head_rot;
	static U32 last_control_flags = 0;
	static U8 last_render_state;
	static U8 duplicate_count = 0;
	static F32 head_rot_chg = 1.0;
	static U8 last_flags;
	LLMessageSystem* msg = gMessageSystem;
	LLVector3		camera_pos_agent;
	U8				render_state;
	LLQuaternion body_rotation = gAgent.getFrameAgent().getQuaternion();
	LLQuaternion head_rotation = gAgent.getHeadRotation();
	camera_pos_agent = gAgentCamera.getCameraPositionAgent();
	render_state = gAgent.getRenderState();
	U32		control_flag_change = 0;
	U8		flag_change = 0;
	cam_center_chg = last_camera_pos_agent - camera_pos_agent;
	cam_rot_chg = last_camera_at - LLViewerCamera::getInstance()->getAtAxis();
	U32 control_flags = gAgent.getControlFlags();
	static LLCachedControl<bool> alchemyPrejump(gSavedSettings, "Nimble", false);
	if (alchemyPrejump)
	{
		control_flags |= AGENT_CONTROL_FINISH_ANIM;
	}
	MASK	key_mask = gKeyboard->currentMask(TRUE);
	if (key_mask & MASK_ALT || key_mask & MASK_CONTROL)
	{
		control_flags &= ~(AGENT_CONTROL_LBUTTON_DOWN |
			AGENT_CONTROL_ML_LBUTTON_DOWN);
		control_flags |= 	AGENT_CONTROL_LBUTTON_UP |
			AGENT_CONTROL_ML_LBUTTON_UP;
	}
	control_flag_change = last_control_flags ^ control_flags;
	U8 flags = AU_FLAGS_NONE;
	if (gAgent.isGroupTitleHidden())
	{
		flags |= AU_FLAGS_HIDETITLE;
	}
	flag_change = last_flags ^ flags;
	head_rot_chg = dot(last_head_rot, head_rotation);
	if (force_send ||
		(cam_center_chg.magVec() > TRANSLATE_THRESHOLD) ||
		(head_rot_chg < THRESHOLD_HEAD_ROT_QDOT) ||
		(last_render_state != render_state) ||
		(cam_rot_chg.magVec() > ROTATION_THRESHOLD) ||
		control_flag_change != 0 ||
		flag_change != 0)
	{
		duplicate_count = 0;
	}
	else
	{
		duplicate_count++;
		if (head_rot_chg < MAX_HEAD_ROT_QDOT  &&  duplicate_count < AGENT_UPDATES_PER_SECOND)
		{
			if (head_rot_chg < THRESHOLD_HEAD_ROT_QDOT + (MAX_HEAD_ROT_QDOT - THRESHOLD_HEAD_ROT_QDOT) * duplicate_count / AGENT_UPDATES_PER_SECOND)
			{
				duplicate_count = 0;
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
	}
	if (duplicate_count < DUP_MSGS && !gDisconnected)
	{
		LL_RECORD_BLOCK_TIME(FTM_AGENT_UPDATE_SEND);
		msg->newMessageFast(_PREHASH_AgentUpdate);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addQuatFast(_PREHASH_BodyRotation, body_rotation);
		msg->addQuatFast(_PREHASH_HeadRotation, head_rotation);
		msg->addU8Fast(_PREHASH_State, render_state);
		msg->addU8Fast(_PREHASH_Flags, flags);
		msg->addVector3Fast(_PREHASH_CameraCenter, camera_pos_agent);
		msg->addVector3Fast(_PREHASH_CameraAtAxis, LLViewerCamera::getInstance()->getAtAxis());
		msg->addVector3Fast(_PREHASH_CameraLeftAxis, LLViewerCamera::getInstance()->getLeftAxis());
		msg->addVector3Fast(_PREHASH_CameraUpAxis, LLViewerCamera::getInstance()->getUpAxis());
		msg->addF32Fast(_PREHASH_Far, gAgentCamera.mDrawDistance);
		msg->addU32Fast(_PREHASH_ControlFlags, control_flags);
		if (gDebugClicks)
		{
			if (control_flags & AGENT_CONTROL_LBUTTON_DOWN)
			{
				LL_INFOS("Messaging") << "AgentUpdate left button down" << LL_ENDL;
			}
			if (control_flags & AGENT_CONTROL_LBUTTON_UP)
			{
				LL_INFOS("Messaging") << "AgentUpdate left button up" << LL_ENDL;
			}
		}
		gAgent.enableControlFlagReset();
		if (!send_reliable)
		{
			gAgent.sendMessage();
		}
		else
		{
			gAgent.sendReliableMessage();
		}
		last_head_rot = head_rotation;
		last_render_state = render_state;
		last_camera_pos_agent = camera_pos_agent;
		last_camera_at = LLViewerCamera::getInstance()->getAtAxis();
		last_camera_left = LLViewerCamera::getInstance()->getLeftAxis();
		last_camera_up = LLViewerCamera::getInstance()->getUpAxis();
		last_control_flags = control_flags;
		last_flags = flags;
	}
}
class PostponedSoundData
{
public:
    PostponedSoundData() :
        mExpirationTime(0)
    {}
    PostponedSoundData(const LLUUID &object_id, const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain, const U8 flags);
    bool hasExpired() { return LLFrameTimer::getTotalSeconds() > mExpirationTime; }
    LLUUID mObjectId;
    LLUUID mSoundId;
    LLUUID mOwnerId;
    F32 mGain;
    U8 mFlags;
    static const F64 MAXIMUM_PLAY_DELAY;
private:
    F64 mExpirationTime;
};
const F64 PostponedSoundData::MAXIMUM_PLAY_DELAY = 15.0;
static F64 postponed_sounds_update_expiration = 0.0;
static std::map<LLUUID, PostponedSoundData> postponed_sounds;
void set_attached_sound(LLViewerObject *objectp, const LLUUID &object_id, const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain, const U8 flags)
{
    if (LLMuteList::getInstance()->isMuted(object_id)) return;
    if (LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagObjectSounds)) return;
    LLVector3d pos = objectp->getPositionGlobal();
    if (!gAgent.canAccessMaturityAtGlobal(pos))
    {
        return;
    }
    objectp->setAttachedSound(sound_id, owner_id, gain, flags);
}
PostponedSoundData::PostponedSoundData(const LLUUID &object_id, const LLUUID &sound_id, const LLUUID& owner_id, const F32 gain, const U8 flags)
    :
    mObjectId(object_id),
    mSoundId(sound_id),
    mOwnerId(owner_id),
    mGain(gain),
    mFlags(flags),
    mExpirationTime(LLFrameTimer::getTotalSeconds() + MAXIMUM_PLAY_DELAY)
{
}
void update_attached_sounds()
{
    if (postponed_sounds.empty())
    {
        return;
    }
    std::map<LLUUID, PostponedSoundData>::iterator iter = postponed_sounds.begin();
    std::map<LLUUID, PostponedSoundData>::iterator end = postponed_sounds.end();
    while (iter != end)
    {
        std::map<LLUUID, PostponedSoundData>::iterator cur_iter = iter++;
        PostponedSoundData* data = &cur_iter->second;
        if (data->hasExpired())
        {
            postponed_sounds.erase(cur_iter);
        }
        else
        {
            LLViewerObject *objectp = gObjectList.findObject(data->mObjectId);
            if (objectp)
            {
                set_attached_sound(objectp, data->mObjectId, data->mSoundId, data->mOwnerId, data->mGain, data->mFlags);
                postponed_sounds.erase(cur_iter);
            }
        }
    }
    postponed_sounds_update_expiration = LLFrameTimer::getTotalSeconds() + 2 * PostponedSoundData::MAXIMUM_PLAY_DELAY;
}
void clear_expired_postponed_sounds()
{
    if (postponed_sounds_update_expiration > LLFrameTimer::getTotalSeconds())
    {
        return;
    }
    std::map<LLUUID, PostponedSoundData>::iterator iter = postponed_sounds.begin();
    std::map<LLUUID, PostponedSoundData>::iterator end = postponed_sounds.end();
    while (iter != end)
    {
        std::map<LLUUID, PostponedSoundData>::iterator cur_iter = iter++;
        PostponedSoundData* data = &cur_iter->second;
        if (data->hasExpired())
        {
            postponed_sounds.erase(cur_iter);
        }
    }
    postponed_sounds_update_expiration = LLFrameTimer::getTotalSeconds() + 2 * PostponedSoundData::MAXIMUM_PLAY_DELAY;
}
extern U32Bits gObjectData;
void process_object_update(LLMessageSystem* mesgsys, void** user_data)
{
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectData += U32Bytes(mesgsys->getReceiveCompressedSize());
	}
	else
	{
		gObjectData += U32Bytes(mesgsys->getReceiveSize());
	}
	S32 old_num_objects = gObjectList.mNumNewObjects;
	gObjectList.processObjectUpdate(mesgsys, user_data, OUT_FULL);
	if (old_num_objects != gObjectList.mNumNewObjects)
	{
		update_attached_sounds();
	}
}
void process_compressed_object_update(LLMessageSystem* mesgsys, void** user_data)
{
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectData += U32Bytes(mesgsys->getReceiveCompressedSize());
	}
	else
	{
		gObjectData += U32Bytes(mesgsys->getReceiveSize());
	}
	S32 old_num_objects = gObjectList.mNumNewObjects;
	gObjectList.processCompressedObjectUpdate(mesgsys, user_data, OUT_FULL_COMPRESSED);
	if (old_num_objects != gObjectList.mNumNewObjects)
	{
		update_attached_sounds();
	}
}
void process_cached_object_update(LLMessageSystem* mesgsys, void** user_data)
{
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectData += U32Bytes(mesgsys->getReceiveCompressedSize());
	}
	else
	{
		gObjectData += U32Bytes(mesgsys->getReceiveSize());
	}
	gObjectList.processCachedObjectUpdate(mesgsys, user_data, OUT_FULL_CACHED);
}
void process_terse_object_update_improved(LLMessageSystem* mesgsys, void** user_data)
{
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectData += U32Bytes(mesgsys->getReceiveCompressedSize());
	}
	else
	{
		gObjectData += U32Bytes(mesgsys->getReceiveSize());
	}
	S32 old_num_objects = gObjectList.mNumNewObjects;
	gObjectList.processCompressedObjectUpdate(mesgsys, user_data, OUT_TERSE_IMPROVED);
	if (old_num_objects != gObjectList.mNumNewObjects)
	{
		update_attached_sounds();
	}
}
static LLTrace::BlockTimerStatHandle FTM_PROCESS_OBJECTS("Process Kill Objects");
class LLRefreshAttachmentsAfterTPTimer : public LLEventTimer
{
public:
	LLRefreshAttachmentsAfterTPTimer() : LLEventTimer(5.f)
	{
		mEventTimer.stop();
	}

	BOOL tick()
	{
		mEventTimer.stop();
		dump_hud_teleport_state("refreshAttachments_timer_before");
		LLAttachmentsMgr::instance().refreshAttachments();
		dump_hud_teleport_state("refreshAttachments_timer_after");
		return FALSE;
	}

	void triggerRefresh()
	{
		mEventTimer.start();
	}
};
static LLRefreshAttachmentsAfterTPTimer sRefreshAttachmentsAfterTPTimer;
const F32 POST_TELEPORT_ATTACHMENT_KILL_DELAY = 3.f;
void process_kill_object(LLMessageSystem* mesgsys, void** user_data)
{
	LL_RECORD_BLOCK_TIME(FTM_PROCESS_OBJECTS);
	auto agent_region = gAgent.getRegion();
	if (!agent_region) return;
	LLUUID		id;
	U32 ip = mesgsys->getSenderIP();
	U32 port = mesgsys->getSenderPort();
	bool different_region = mesgsys->getSender().getIPandPort() != agent_region->getHost().getIPandPort();
	S32 num_objects = mesgsys->getNumberOfBlocksFast(_PREHASH_ObjectData);
	for (S32 i = 0; i < num_objects; ++i)
	{
		U32 local_id;
		mesgsys->getU32Fast(_PREHASH_ObjectData, _PREHASH_ID, local_id, i);
		LLViewerObjectList::getUUIDFromLocal(id, local_id, ip, port);
		if (id == LLUUID::null)
		{
			LL_DEBUGS("Messaging") << "Unknown kill for local " << local_id << LL_ENDL;
			++gObjectList.mNumUnknownKills;
			continue;
		}
		else
		{
			LL_DEBUGS("Messaging") << "Kill message for local " << local_id << LL_ENDL;
		}
		if (id == gAgentID)
		{
			continue;
		}
		LLViewerObject* objectp = gObjectList.findObject(id);
		if (objectp)
		{
			if (gAgentAvatarp == objectp->getAvatar())
			{
				if (different_region)
				{
					LL_WARNS() << "Region other than our own killing our attachments!!" << LL_ENDL;
					log_hud_event("KillObject_IGNORE_other_region", objectp);
					continue;
				}
				else if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
				{
					LL_WARNS() << "Region killing our attachments during teleport!!" << LL_ENDL;
					log_hud_event("KillObject_IGNORE_during_tp", objectp);
					continue;
				}
				else if (gAgent.isCrossingRegion())
				{
					LL_WARNS() << "Region killing our attachments during region cross!!" << LL_ENDL;
					log_hud_event("KillObject_IGNORE_region_cross", objectp);
					continue;
				}
				else if (gPostTeleportFinishKillObjectDelayTimer.getElapsedTimeF32() <= POST_TELEPORT_ATTACHMENT_KILL_DELAY)
				{
					LL_WARNS() << "Region killing our attachments shortly after teleport!!" << LL_ENDL;
					log_hud_event("KillObject_IGNORE_post_tp_delay", objectp);
					sRefreshAttachmentsAfterTPTimer.triggerRefresh();
					continue;
				}
			}
			if (objectp->isHUDAttachment() || objectp->isAttachment())
			{
				log_hud_event("KillObject_KILL", objectp);
			}
			if (gShowObjectUpdates)
			{
				LLColor4 color(0.f, 1.f, 0.f, 1.f);
				gPipeline.addDebugBlip(objectp->getPositionAgent(), color);
			}
			gObjectList.killObject(objectp);
		}
		else
		{
			LL_WARNS("Messaging") << "Object in UUID lookup, but not on object list in kill!" << LL_ENDL;
			gObjectList.mNumUnknownKills++;
		}
		LLSelectMgr::getInstance()->removeObjectFromSelections(id);
	}
}
void process_time_synch(LLMessageSystem* mesgsys, void** user_data)
{
	LLVector3 sun_direction;
	LLVector3 sun_ang_velocity;
	F32 phase;
	U64	space_time_usec;
    U32 seconds_per_day;
    U32 seconds_per_year;
	mesgsys->getU64Fast(_PREHASH_TimeInfo, _PREHASH_UsecSinceStart, space_time_usec);
	mesgsys->getU32Fast(_PREHASH_TimeInfo, _PREHASH_SecPerDay, seconds_per_day);
	mesgsys->getU32Fast(_PREHASH_TimeInfo, _PREHASH_SecPerYear, seconds_per_year);
	mesgsys->getF32Fast(_PREHASH_TimeInfo, _PREHASH_SunPhase, phase);
	mesgsys->getVector3Fast(_PREHASH_TimeInfo, _PREHASH_SunDirection, sun_direction);
	mesgsys->getVector3Fast(_PREHASH_TimeInfo, _PREHASH_SunAngVelocity, sun_ang_velocity);
	LLWorld::getInstance()->setSpaceTimeUSec(space_time_usec);
	LL_DEBUGS("WindlightSync") << "Sun phase: " << phase << " rad = " << fmodf(phase / F_TWO_PI + 0.25, 1.f) * 24.f << " h" << LL_ENDL;
	gSky.setSunPhase(phase);
	gSky.setSunTargetDirection(sun_direction, sun_ang_velocity);
	if (!gNoRender && !(gSavedSettings.getBOOL("SkyOverrideSimSunPosition") || gSky.getOverrideSun()))
	{
		gSky.setSunDirection(sun_direction, sun_ang_velocity);
	}
}
void process_sound_trigger(LLMessageSystem* msg, void**)
{
	if (!gAudiop)
	{
		LL_WARNS("AudioEngine") << "LLAudioEngine instance doesn't exist!" << LL_ENDL;
		return;
	}
	U64		region_handle = 0;
	F32		gain = 0;
	LLUUID	sound_id;
	LLUUID	owner_id;
	LLUUID	object_id;
	LLUUID	parent_id;
	LLVector3	pos_local;
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_SoundID, sound_id);
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_OwnerID, owner_id);
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_ObjectID, object_id);
	if (NACLAntiSpamRegistry::instanceExists())
	{
		auto& antispam = NACLAntiSpamRegistry::instance();
		static const LLCachedControl<U32> _NACL_AntiSpamSoundMulti("_NACL_AntiSpamSoundMulti");
		if (owner_id.isNull())
		{
			bool is_collision_sound(const std::string & sound);
			if (!is_collision_sound(sound_id.asString())
				&& antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SOUND, object_id, LFIDBearer::OBJECT, _NACL_AntiSpamSoundMulti))
				return;
		}
		else if (antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SOUND, owner_id, LFIDBearer::AVATAR, _NACL_AntiSpamSoundMulti)) return;
	}
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_ParentID, parent_id);
	msg->getU64Fast(_PREHASH_SoundData, _PREHASH_Handle, region_handle);
	msg->getVector3Fast(_PREHASH_SoundData, _PREHASH_Position, pos_local);
	msg->getF32Fast(_PREHASH_SoundData, _PREHASH_Gain, gain);
	LLVector3d	pos_global = from_region_handle(region_handle);
	pos_global.mdV[VX] += pos_local.mV[VX];
	pos_global.mdV[VY] += pos_local.mV[VY];
	pos_global.mdV[VZ] += pos_local.mV[VZ];
	if (!LLViewerParcelMgr::getInstance()->canHearSound(pos_global)) return;
	if (LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagObjectSounds)) return;
	if (LLMuteList::getInstance()->isMuted(object_id)) return;
	if (parent_id.notNull()
		&& LLMuteList::getInstance()->isMuted(parent_id))
	{
		return;
	}
	if (!gAgent.canAccessMaturityInRegion(region_handle))
	{
		return;
	}
	if (object_id == owner_id)
	{
		if (!gSavedSettings.getBOOL("EnableGestureSounds"))
		{
			if (owner_id != gAgentID || !gSavedSettings.getBOOL("EnableGestureSoundsSelf"))
				return;
		}
	}
	else if (!gSavedSettings.getBOOL("EnableNongestureSounds"))
	{
		if (owner_id != gAgentID || !gSavedSettings.getBOOL("EnableNongestureSoundsSelf"))
			return;
	}
	gAudiop->triggerSound(sound_id, owner_id, gain, LLAudioEngine::AUDIO_TYPE_SFX, pos_global, object_id);
}
void process_preload_sound(LLMessageSystem* msg, void** user_data)
{
	if (!gAudiop)
	{
		LL_WARNS("AudioEngine") << "LLAudioEngine instance doesn't exist!" << LL_ENDL;
		return;
	}
	LLUUID sound_id;
	LLUUID object_id;
	LLUUID owner_id;
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_SoundID, sound_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_OwnerID, owner_id);
	if (NACLAntiSpamRegistry::instanceExists())
	{
		auto& antispam = NACLAntiSpamRegistry::instance();
		static const LLCachedControl<U32> _NACL_AntiSpamSoundPreloadMulti("_NACL_AntiSpamSoundPreloadMulti");
		if ((owner_id.isNull()
			&& antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SOUND_PRELOAD, object_id, LFIDBearer::OBJECT, _NACL_AntiSpamSoundPreloadMulti))
			|| antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SOUND_PRELOAD, owner_id, LFIDBearer::AVATAR, _NACL_AntiSpamSoundPreloadMulti))
			return;
	}
	LLViewerObject* objectp = gObjectList.findObject(object_id);
	if (!objectp) return;
	if (LLMuteList::getInstance()->isMuted(object_id)) return;
	if (LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagObjectSounds)) return;
	LLAudioSource* sourcep = objectp->getAudioSource(owner_id);
	if (!sourcep) return;
	LLVector3d pos_global = objectp->getPositionGlobal();
	if (gAgent.canAccessMaturityAtGlobal(pos_global))
	{
		sourcep->preload(sound_id);
	}
}
void process_attached_sound(LLMessageSystem* msg, void** user_data)
{
	F32 gain = 0;
	LLUUID sound_id;
	LLUUID object_id;
	LLUUID owner_id;
	U8 flags;
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_SoundID, sound_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_OwnerID, owner_id);
	if (NACLAntiSpamRegistry::instanceExists())
	{
		auto& antispam = NACLAntiSpamRegistry::instance();
		if ((owner_id.isNull()
			&& antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SOUND, object_id, LFIDBearer::OBJECT))
			|| antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SOUND, owner_id))
			return;
	}
	msg->getF32Fast(_PREHASH_DataBlock, _PREHASH_Gain, gain);
	msg->getU8Fast(_PREHASH_DataBlock, _PREHASH_Flags, flags);
	LLViewerObject *objectp = gObjectList.findObject(object_id);
	if (objectp)
	{
		set_attached_sound(objectp, object_id, sound_id, owner_id, gain, flags);
	}
	else if (sound_id.notNull())
	{
		if (!LLMuteList::getInstance()->isMuted(object_id) && !LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagObjectSounds))
			postponed_sounds[object_id] = (PostponedSoundData(object_id, sound_id, owner_id, gain, flags));
		clear_expired_postponed_sounds();
	}
	else
	{
		std::map<LLUUID, PostponedSoundData>::iterator iter = postponed_sounds.find(object_id);
		if (iter != postponed_sounds.end())
		{
			postponed_sounds.erase(iter);
		}
	}
}
void process_attached_sound_gain_change(LLMessageSystem *mesgsys, void **user_data)
{
	F32 gain = 0;
	LLUUID object_guid;
	LLViewerObject* objectp = nullptr;
	mesgsys->getUUIDFast(_PREHASH_DataBlock, _PREHASH_ObjectID, object_guid);
	if (!((objectp = gObjectList.findObject(object_guid))))
	{
		return;
	}
 	mesgsys->getF32Fast(_PREHASH_DataBlock, _PREHASH_Gain, gain);
	objectp->adjustAudioGain(gain);
}
void process_health_message(LLMessageSystem* mesgsys, void** user_data)
{
	F32 health;
	mesgsys->getF32Fast(_PREHASH_HealthData, _PREHASH_Health, health);
	if (gStatusBar)
	{
		gStatusBar->setHealth((S32)health);
	}
}
void process_sim_stats(LLMessageSystem* msg, void** user_data)
{
	static LLHost sLastHost;
	auto& stats = LLViewerStats::instance();
	if (msg->getSender() != sLastHost)
	{
		sLastHost = msg->getSender();
		stats.mSimTimeDilation.reset();
		stats.mSimFPS.reset();
		stats.mSimPhysicsFPS.reset();
		stats.mSimAgentUPS.reset();
		stats.mSimFrameMsec.reset();
		stats.mSimNetMsec.reset();
		stats.mSimSimOtherMsec.reset();
		stats.mSimSimPhysicsMsec.reset();
		stats.mSimAgentMsec.reset();
		stats.mSimImagesMsec.reset();
		stats.mSimScriptMsec.reset();
		stats.mSimObjects.reset();
		stats.mSimActiveObjects.reset();
		stats.mSimMainAgents.reset();
		stats.mSimChildAgents.reset();
		stats.mSimActiveScripts.reset();
		stats.mSimScriptEPS.reset();
		stats.mSimInPPS.reset();
		stats.mSimOutPPS.reset();
		stats.mSimPendingDownloads.reset();
		stats.mSimPendingUploads.reset();
		stats.mSimPendingLocalUploads.reset();
		stats.mSimTotalUnackedBytes.reset();
		stats.mPhysicsPinnedTasks.reset();
		stats.mPhysicsLODTasks.reset();
		stats.mSimSimPhysicsStepMsec.reset();
		stats.mSimSimPhysicsShapeUpdateMsec.reset();
		stats.mSimSimPhysicsOtherMsec.reset();
		stats.mPhysicsMemoryAllocated.reset();
		stats.mSimSpareMsec.reset();
		stats.mSimSleepMsec.reset();
		stats.mSimPumpIOMsec.reset();
		stats.mSimPctScriptsRun.reset();
		stats.mSimSimAIStepMsec.reset();
		stats.mSimSimSkippedSilhouetteSteps.reset();
		stats.mSimSimPctSteppedCharacters.reset();
	}
	S32 count = msg->getNumberOfBlocks("Stat");
	for (S32 i = 0; i < count; ++i)
	{
		U32 stat_id;
		F32 stat_value;
		msg->getU32("Stat", "StatID", stat_id, i);
		msg->getF32("Stat", "StatValue", stat_value, i);
		switch (stat_id)
		{
		case LL_SIM_STAT_TIME_DILATION:
			stats.mSimTimeDilation.addValue(stat_value);
			break;
		case LL_SIM_STAT_FPS:
			stats.mSimFPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_PHYSFPS:
			stats.mSimPhysicsFPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_AGENTUPS:
			stats.mSimAgentUPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_FRAMEMS:
			stats.mSimFrameMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_NETMS:
			stats.mSimNetMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMOTHERMS:
			stats.mSimSimOtherMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSMS:
			stats.mSimSimPhysicsMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_AGENTMS:
			stats.mSimAgentMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_IMAGESMS:
			stats.mSimImagesMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SCRIPTMS:
			stats.mSimScriptMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMTASKS:
			stats.mSimObjects.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMTASKSACTIVE:
			stats.mSimActiveObjects.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMAGENTMAIN:
			stats.mSimMainAgents.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMAGENTCHILD:
			stats.mSimChildAgents.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMSCRIPTSACTIVE:
			if (gSavedSettings.getBOOL("AscentDisplayTotalScriptJumps"))
			{
				auto control = gSavedSettings.getControl("Ascentnumscripts");
				const auto& numscripts = control->get().asFloat();
				if(abs(stat_value-numscripts)>gSavedSettings.getF32("Ascentnumscriptdiff"))
				{
					std::stringstream os;
					os << (U32)numscripts << " to " << (U32)stat_value << " (";
					const S32 tdiff = stat_value - numscripts;
					if (tdiff > 0) os << "+";
					os << tdiff << ')';
					LLChat chat("Total scripts jumped from " + os.str());
					LLFloaterChat::addChat(chat, FALSE, FALSE);
				}
				control->set(stat_value);
			}
			stats.mSimActiveScripts.addValue(stat_value);
			break;
		case LL_SIM_STAT_SCRIPT_EPS:
			stats.mSimScriptEPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_INPPS:
			stats.mSimInPPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_OUTPPS:
			stats.mSimOutPPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_DOWNLOADS:
			stats.mSimPendingDownloads.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_UPLOADS:
			stats.mSimPendingUploads.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_LOCAL_UPLOADS:
			stats.mSimPendingLocalUploads.addValue(stat_value);
			break;
		case LL_SIM_STAT_TOTAL_UNACKED_BYTES:
			stats.mSimTotalUnackedBytes.addValue(stat_value / 1024.f);
			break;
		case LL_SIM_STAT_PHYSICS_PINNED_TASKS:
			stats.mPhysicsPinnedTasks.addValue(stat_value);
			break;
		case LL_SIM_STAT_PHYSICS_LOD_TASKS:
			stats.mPhysicsLODTasks.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSSTEPMS:
			stats.mSimSimPhysicsStepMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSSHAPEMS:
			stats.mSimSimPhysicsShapeUpdateMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSOTHERMS:
			stats.mSimSimPhysicsOtherMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSMEMORY:
			stats.mPhysicsMemoryAllocated.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMSPARETIME:
			stats.mSimSpareMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMSLEEPTIME:
			stats.mSimSleepMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_IOPUMPTIME:
			stats.mSimPumpIOMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_PCTSCRIPTSRUN:
			stats.mSimPctScriptsRun.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMAISTEPTIMEMS:
			stats.mSimSimAIStepMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SKIPPEDAISILSTEPS_PS:
			stats.mSimSimSkippedSilhouetteSteps.addValue(stat_value);
			break;
		case LL_SIM_STAT_PCTSTEPPEDCHARACTERS:
			stats.mSimSimPctSteppedCharacters.addValue(stat_value);
			break;
		default:
 			LL_DEBUGS("Messaging") << "Unknown stat id" << stat_id << LL_ENDL;
		  break;
		}
	}
	U32 max_tasks_per_region;
	U64 region_flags;
	msg->getU32("Region", "ObjectCapacity", max_tasks_per_region);
	if (msg->has(_PREHASH_RegionInfo))
	{
		msg->getU64("RegionInfo", "RegionFlagsExtended", region_flags);
	}
	else
	{
		U32 flags = 0;
		msg->getU32("Region", "RegionFlags", flags);
		region_flags = flags;
	}
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		BOOL was_flying = gAgent.getFlying();
		regionp->setRegionFlags(region_flags);
		regionp->setMaxTasks(max_tasks_per_region);
		if (was_flying && regionp->getBlockFly())
		{
			gAgent.setFlying(gAgent.canFly());
		}
	}
}
void process_avatar_animation(LLMessageSystem* mesgsys, void** user_data)
{
	LLUUID	animation_id;
	LLUUID	uuid;
	S32		anim_sequence_id;
	mesgsys->getUUIDFast(_PREHASH_Sender, _PREHASH_ID, uuid);
	LLVOAvatar* avatarp = gObjectList.findAvatar(uuid);
	if (!avatarp)
	{
		if (gSavedPerAccountSettings.getBOOL("AllynRenderFriendsOnly"))
		{
			std::map<LLUUID, S32> anims;
			S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationList);
			for (S32 i = 0; i < num_blocks; i++)
			{
				mesgsys->getUUIDFast(_PREHASH_AnimationList, _PREHASH_AnimID, animation_id, i);
				mesgsys->getS32Fast(_PREHASH_AnimationList, _PREHASH_AnimSequenceID, anim_sequence_id, i);
				anims[animation_id] = anim_sequence_id;
			}
			LLVOAvatar::cacheAnimationsForFriendsOnly(uuid, anims);
		}
		else
		{
			LL_WARNS("Messaging") << "Received animation state for unknown avatar " << uuid << LL_ENDL;
		}
		return;
	}
	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationList);
	S32 num_source_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationSourceList);
	LL_DEBUGS("Messaging", "Motion") << "Processing " << num_blocks << " Animations" << LL_ENDL;
	avatarp->mSignaledAnimations.clear();
	if (avatarp->isSelf())
	{
		LLUUID object_id;
		for (S32 i = 0; i < num_blocks; i++)
		{
			mesgsys->getUUIDFast(_PREHASH_AnimationList, _PREHASH_AnimID, animation_id, i);
			mesgsys->getS32Fast(_PREHASH_AnimationList, _PREHASH_AnimSequenceID, anim_sequence_id, i);
			avatarp->mSignaledAnimations[animation_id] = anim_sequence_id;
			if (animation_id == ANIM_AGENT_STANDUP && gAgent.getFlying())
			{
				gAgent.setFlying(FALSE);
			}
			if (i < num_source_blocks)
			{
				mesgsys->getUUIDFast(_PREHASH_AnimationSourceList, _PREHASH_ObjectID, object_id, i);
				LLViewerObject* object = gObjectList.findObject(object_id);
				if (object)
				{
					object->setFlagsWithoutUpdate(FLAGS_ANIM_SOURCE, TRUE);
					BOOL anim_found = FALSE;
					LLVOAvatar::AnimSourceIterator anim_it = avatarp->mAnimationSources.find(object_id);
					for (; anim_it != avatarp->mAnimationSources.end(); ++anim_it)
					{
						if (anim_it->second == animation_id)
						{
							anim_found = TRUE;
							break;
						}
					}
					if (!anim_found)
					{
						avatarp->mAnimationSources.insert(LLVOAvatar::AnimationSourceMap::value_type(object_id, animation_id));
					}
				}
				LL_DEBUGS("Messaging", "Motion") << "Anim sequence ID: " << anim_sequence_id
									<< " Animation id: " << animation_id
									<< " From block: " << object_id << LL_ENDL;
			}
			else
			{
				LL_DEBUGS("Messaging", "Motion") << "Anim sequence ID: " << anim_sequence_id
									<< " Animation id: " << animation_id << LL_ENDL;
			}
		}
	}
	else
	{
		for (S32 i = 0; i < num_blocks; i++)
		{
			mesgsys->getUUIDFast(_PREHASH_AnimationList, _PREHASH_AnimID, animation_id, i);
			mesgsys->getS32Fast(_PREHASH_AnimationList, _PREHASH_AnimSequenceID, anim_sequence_id, i);
			avatarp->mSignaledAnimations[animation_id] = anim_sequence_id;
		}
	}
	{
		avatarp->processAnimationStateChanges();
	}
}
void process_object_animation(LLMessageSystem *mesgsys, void **user_data)
{
	LLUUID	animation_id;
	LLUUID	uuid;
	S32		anim_sequence_id;
	mesgsys->getUUIDFast(_PREHASH_Sender, _PREHASH_ID, uuid);
    LL_DEBUGS("AnimatedObjectsNotify") << "Received animation state for object " << uuid << LL_ENDL;
    signaled_animation_map_t signaled_anims;
	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationList);
	LL_DEBUGS("AnimatedObjectsNotify") << "processing object animation requests, num_blocks " << num_blocks << " uuid " << uuid << LL_ENDL;
    for( S32 i = 0; i < num_blocks; i++ )
    {
        mesgsys->getUUIDFast(_PREHASH_AnimationList, _PREHASH_AnimID, animation_id, i);
        mesgsys->getS32Fast(_PREHASH_AnimationList, _PREHASH_AnimSequenceID, anim_sequence_id, i);
        signaled_anims[animation_id] = anim_sequence_id;
        LL_DEBUGS("AnimatedObjectsNotify") << "added signaled_anims animation request for object "
                                    << uuid << " animation id " << animation_id << LL_ENDL;
    }
    LLObjectSignaledAnimationMap::instance().getMap()[uuid] = signaled_anims;
    LLViewerObject *objp = gObjectList.findObject(uuid);
    if (!objp)
    {
		LL_DEBUGS("AnimatedObjectsNotify") << "Received animation state for unknown object " << uuid << LL_ENDL;
        return;
    }
	LLVOVolume *volp = objp->asVolume();
    if (!volp)
    {
		LL_DEBUGS("AnimatedObjectsNotify") << "Received animation state for non-volume object " << uuid << LL_ENDL;
        return;
    }
    if (!volp->isAnimatedObject())
    {
		LL_DEBUGS("AnimatedObjectsNotify") << "Received animation state for non-animated object " << uuid << LL_ENDL;
        return;
    }
    volp->updateControlAvatar();
    LLControlAvatar *avatarp = volp->getControlAvatar();
    if (!avatarp)
    {
        LL_DEBUGS("AnimatedObjectsNotify") << "Received animation request for object with no control avatar, ignoring " << uuid << LL_ENDL;
        return;
    }
    if (!avatarp->mPlaying)
    {
        avatarp->mPlaying = true;
        {
            avatarp->updateVolumeGeom();
            avatarp->mRootVolp->recursiveMarkForUpdate(TRUE);
        }
    }
    avatarp->updateAnimations();
}
void process_avatar_appearance(LLMessageSystem* mesgsys, void** user_data)
{
	LLUUID uuid;
	mesgsys->getUUIDFast(_PREHASH_Sender, _PREHASH_ID, uuid);
	LLVOAvatar* avatarp = gObjectList.findAvatar(uuid);
	if (avatarp)
	{
		avatarp->processAvatarAppearance(mesgsys);
	}
	else if (gSavedPerAccountSettings.getBOOL("AllynRenderFriendsOnly"))
	{
		LLVOAvatar::cacheAppearanceMessageForFriendsOnly(uuid, mesgsys);
	}
	else
	{
		LL_WARNS("Messaging") << "avatar_appearance sent for unknown avatar " << uuid << LL_ENDL;
	}
}
void process_camera_constraint(LLMessageSystem* mesgsys, void** user_data)
{
	static LLCachedControl<bool> disableSimConst(gSavedSettings, "AlchemyDisableSimCamConstraint");
	if (disableSimConst)
		return;
	LLVector4 cameraCollidePlane;
	mesgsys->getVector4Fast(_PREHASH_CameraCollidePlane, _PREHASH_Plane, cameraCollidePlane);
	gAgentCamera.setCameraCollidePlane(cameraCollidePlane);
}
void near_sit_object(BOOL success, void* data)
{
	if (success)
	{
		gMessageSystem->newMessageFast(_PREHASH_AgentSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
	}
}
void process_avatar_sit_response(LLMessageSystem* mesgsys, void** user_data)
{
	LLVector3 sitPosition;
	LLQuaternion sitRotation;
	LLUUID sitObjectID;
	BOOL use_autopilot;
	mesgsys->getUUIDFast(_PREHASH_SitObject, _PREHASH_ID, sitObjectID);
	mesgsys->getBOOLFast(_PREHASH_SitTransform, _PREHASH_AutoPilot, use_autopilot);
	mesgsys->getVector3Fast(_PREHASH_SitTransform, _PREHASH_SitPosition, sitPosition);
	mesgsys->getQuatFast(_PREHASH_SitTransform, _PREHASH_SitRotation, sitRotation);
	LLVector3 camera_eye;
	mesgsys->getVector3Fast(_PREHASH_SitTransform, _PREHASH_CameraEyeOffset, camera_eye);
	LLVector3 camera_at;
	mesgsys->getVector3Fast(_PREHASH_SitTransform, _PREHASH_CameraAtOffset, camera_at);
	BOOL force_mouselook;
	mesgsys->getBOOLFast(_PREHASH_SitTransform, _PREHASH_ForceMouselook, force_mouselook);
	if (isAgentAvatarValid() && dist_vec_squared(camera_eye, camera_at) > CAMERA_POSITION_THRESHOLD_SQUARED)
	{
		gAgentCamera.setSitCamera(sitObjectID, camera_eye, camera_at);
	}
	gAgentCamera.setForceMouselook(force_mouselook);
	if (!gSavedSettings.getBOOL("LiruContinueFlyingOnUnsit"))
		gAgent.setFlying(FALSE);
	LLViewerObject* object = gObjectList.findObject(sitObjectID);
	if (object)
	{
		LLVector3 sit_spot = object->getPositionAgent() + (sitPosition * object->getRotation());
		if (!use_autopilot || (isAgentAvatarValid() && gAgentAvatarp->isSitting() && gAgentAvatarp->getRoot() == object->getRoot()))
		{
		}
		else
		{
			gAgent.startAutoPilotGlobal(gAgent.getPosGlobalFromAgent(sit_spot), "Sit", &sitRotation, near_sit_object, nullptr, 0.5f);
		}
	}
	else
	{
		LL_WARNS("Messaging") << "Received sit approval for unknown object " << sitObjectID << LL_ENDL;
	}
}
void process_clear_follow_cam_properties(LLMessageSystem* mesgsys, void** user_data)
{
	LLUUID		source_id;
	mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, source_id);
	LLFollowCamMgr::removeFollowCamParams(source_id);
}
void process_set_follow_cam_properties(LLMessageSystem* mesgsys, void** user_data)
{
	S32			type;
	F32			value;
	bool		settingPosition = false;
	bool		settingFocus	= false;
	bool		settingFocusOffset = false;
	LLVector3	position;
	LLVector3	focus;
	LLVector3	focus_offset;
	LLUUID		source_id;
	mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, source_id);
	LLViewerObject* objectp = gObjectList.findObject(source_id);
	if (objectp)
	{
		objectp->setFlagsWithoutUpdate(FLAGS_CAMERA_SOURCE, TRUE);
	}
	S32 num_objects = mesgsys->getNumberOfBlocks("CameraProperty");
	for (S32 block_index = 0; block_index < num_objects; block_index++)
	{
		mesgsys->getS32("CameraProperty", "Type", type, block_index);
		mesgsys->getF32("CameraProperty", "Value", value, block_index);
		switch (type)
		{
		case FOLLOWCAM_PITCH:
			LLFollowCamMgr::setPitch(source_id, value);
			break;
		case FOLLOWCAM_FOCUS_OFFSET_X:
			focus_offset.mV[VX] = value;
			settingFocusOffset = true;
			break;
		case FOLLOWCAM_FOCUS_OFFSET_Y:
			focus_offset.mV[VY] = value;
			settingFocusOffset = true;
			break;
		case FOLLOWCAM_FOCUS_OFFSET_Z:
			focus_offset.mV[VZ] = value;
			settingFocusOffset = true;
			break;
		case FOLLOWCAM_POSITION_LAG:
			LLFollowCamMgr::setPositionLag(source_id, value);
			break;
		case FOLLOWCAM_FOCUS_LAG:
			LLFollowCamMgr::setFocusLag(source_id, value);
			break;
		case FOLLOWCAM_DISTANCE:
			LLFollowCamMgr::setDistance(source_id, value);
			break;
		case FOLLOWCAM_BEHINDNESS_ANGLE:
			LLFollowCamMgr::setBehindnessAngle(source_id, value);
			break;
		case FOLLOWCAM_BEHINDNESS_LAG:
			LLFollowCamMgr::setBehindnessLag(source_id, value);
			break;
		case FOLLOWCAM_POSITION_THRESHOLD:
			LLFollowCamMgr::setPositionThreshold(source_id, value);
			break;
		case FOLLOWCAM_FOCUS_THRESHOLD:
			LLFollowCamMgr::setFocusThreshold(source_id, value);
			break;
		case FOLLOWCAM_ACTIVE:
			LLFollowCamMgr::setCameraActive(source_id, value != 0.f);
			break;
		case FOLLOWCAM_POSITION_X:
			settingPosition = true;
			position.mV[0] = value;
			break;
		case FOLLOWCAM_POSITION_Y:
			settingPosition = true;
			position.mV[1] = value;
			break;
		case FOLLOWCAM_POSITION_Z:
			settingPosition = true;
			position.mV[2] = value;
			break;
		case FOLLOWCAM_FOCUS_X:
			settingFocus = true;
			focus.mV[0] = value;
			break;
		case FOLLOWCAM_FOCUS_Y:
			settingFocus = true;
			focus.mV[1] = value;
			break;
		case FOLLOWCAM_FOCUS_Z:
			settingFocus = true;
			focus.mV[2] = value;
			break;
		case FOLLOWCAM_POSITION_LOCKED:
			LLFollowCamMgr::setPositionLocked(source_id, value != 0.f);
			break;
		case FOLLOWCAM_FOCUS_LOCKED:
			LLFollowCamMgr::setFocusLocked(source_id, value != 0.f);
			break;
		default:
			break;
		}
	}
	if (settingPosition)
	{
		LLFollowCamMgr::setPosition(source_id, position);
	}
	if (settingFocus)
	{
		LLFollowCamMgr::setFocus(source_id, focus);
	}
	if (settingFocusOffset)
	{
		LLFollowCamMgr::setFocusOffset(source_id, focus_offset);
	}
}
void process_name_value(LLMessageSystem* mesgsys, void** user_data)
{
	std::string	temp_str;
	LLUUID	id;
	S32		i, num_blocks;
	mesgsys->getUUIDFast(_PREHASH_TaskData, _PREHASH_ID, id);
	LLViewerObject* object = gObjectList.findObject(id);
	if (object)
	{
		num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_NameValueData);
		for (i = 0; i < num_blocks; i++)
		{
			mesgsys->getStringFast(_PREHASH_NameValueData, _PREHASH_NVPair, temp_str, i);
			LL_INFOS("Messaging") << "Added to object Name Value: " << temp_str << LL_ENDL;
			object->addNVPair(temp_str);
		}
	}
	else
	{
		LL_INFOS("Messaging") << "Can't find object " << id << " to add name value pair" << LL_ENDL;
	}
}
void process_remove_name_value(LLMessageSystem* mesgsys, void** user_data)
{
	std::string	temp_str;
	LLUUID	id;
	S32		i, num_blocks;
	mesgsys->getUUIDFast(_PREHASH_TaskData, _PREHASH_ID, id);
	LLViewerObject* object = gObjectList.findObject(id);
	if (object)
	{
		num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_NameValueData);
		for (i = 0; i < num_blocks; i++)
		{
			mesgsys->getStringFast(_PREHASH_NameValueData, _PREHASH_NVPair, temp_str, i);
			LL_INFOS("Messaging") << "Removed from object Name Value: " << temp_str << LL_ENDL;
			object->removeNVPair(temp_str);
		}
	}
	else
	{
		LL_INFOS("Messaging") << "Can't find object " << id << " to remove name value pair" << LL_ENDL;
	}
}
void process_kick_user(LLMessageSystem* msg, void** )
{
	std::string message;
	msg->getStringFast(_PREHASH_UserInfo, _PREHASH_Reason, message);
	LLAppViewer::instance()->forceDisconnect(message);
}
void process_money_balance_reply(LLMessageSystem* msg, void**)
{
	S32 balance = 0;
	S32 credit = 0;
	S32 committed = 0;
	std::string desc;
	LLUUID tid;
	msg->getUUID("MoneyData", "TransactionID", tid);
	msg->getS32("MoneyData", "MoneyBalance", balance);
	msg->getS32("MoneyData", "SquareMetersCredit", credit);
	msg->getS32("MoneyData", "SquareMetersCommitted", committed);
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_Description, desc);
	LL_INFOS("Messaging") << "L$, credit, committed: " << balance << " " << credit << " "
			<< committed << LL_ENDL;
	if (gStatusBar)
	{
		S32 old_balance = gStatusBar->getBalance();
		if (old_balance != 0)
		{
			if (balance > old_balance)
			{
				LLFirstUse::useBalanceIncrease(balance - old_balance);
			}
			else if (balance < old_balance)
			{
				LLFirstUse::useBalanceDecrease(balance - old_balance);
			}
		}
		gStatusBar->setBalance(balance);
		gStatusBar->setLandCredit(credit);
		gStatusBar->setLandCommitted(committed);
	}
	if (desc.empty()
		|| !gSavedSettings.getBOOL("NotifyMoneyChange"))
	{
		return;
	}
	static std::deque<LLUUID> recent;
	if (std::find(recent.rbegin(), recent.rend(), tid) != recent.rend())
	{
		return;
	}
	const U32 MAX_LOOKBACK = 30;
	const S32 POP_FRONT_SIZE = 12;
	if (recent.size() > MAX_LOOKBACK)
	{
		LL_DEBUGS("Messaging") << "Removing oldest transaction records" << LL_ENDL;
		recent.erase(recent.begin(), recent.begin() + POP_FRONT_SIZE);
	}
	recent.push_back(tid);
	if (msg->has("TransactionInfo"))
	{
		process_money_balance_reply_extended(msg);
	}
	else
	{
		LLSD args;
		args["MESSAGE"] = desc;
		LLNotificationsUtil::add("SystemMessage", args);
		LLFloaterChat::addChat(desc);
	}
}
static std::string reason_from_transaction_type(S32 transaction_type,
												const std::string& item_desc)
{
	switch (transaction_type)
	{
		case TRANS_OBJECT_SALE:
		{
			LLStringUtil::format_map_t arg;
			arg["ITEM"] = item_desc;
			return LLTrans::getString("for item", arg);
		}
		case TRANS_LAND_SALE:
			return LLTrans::getString("for a parcel of land");
		case TRANS_LAND_PASS_SALE:
			return LLTrans::getString("for a land access pass");
		case TRANS_GROUP_LAND_DEED:
			return LLTrans::getString("for deeding land");
		case TRANS_GROUP_CREATE:
			return LLTrans::getString("to create a group");
		case TRANS_GROUP_JOIN:
			return LLTrans::getString("to join a group");
		case TRANS_UPLOAD_CHARGE:
			return LLTrans::getString("to upload");
		case TRANS_CLASSIFIED_CHARGE:
			return LLTrans::getString("to publish a classified ad");
	case TRANS_GIFT:
		return (item_desc == "Payment" ? std::string() : item_desc);
		case TRANS_PAY_OBJECT:
		case TRANS_OBJECT_PAYS:
			return std::string();
		default:
			LL_WARNS() << "Unknown transaction type "
				<< transaction_type << LL_ENDL;
			return std::string();
	}
}
static void process_money_balance_reply_extended(LLMessageSystem* msg)
{
	S32 transaction_type = 0;
	LLUUID source_id;
	BOOL is_source_group = FALSE;
	LLUUID dest_id;
	BOOL is_dest_group = FALSE;
	S32 amount = 0;
	std::string item_description;
	BOOL success = FALSE;
	msg->getS32("TransactionInfo", "TransactionType", transaction_type);
	msg->getUUID("TransactionInfo", "SourceID", source_id);
	msg->getBOOL("TransactionInfo", "IsSourceGroup", is_source_group);
	msg->getUUID("TransactionInfo", "DestID", dest_id);
	msg->getBOOL("TransactionInfo", "IsDestGroup", is_dest_group);
	msg->getS32("TransactionInfo", "Amount", amount);
	msg->getString("TransactionInfo", "ItemDescription", item_description);
	msg->getBOOL("MoneyData", "TransactionSuccess", success);
	LL_INFOS("Money") << "MoneyBalanceReply source " << source_id
		<< " dest " << dest_id
		<< " type " << transaction_type
		<< " item " << item_description << LL_ENDL;
	if (source_id.isNull() && dest_id.isNull())
	{
		return;
	}
	if ((U32)amount < gSavedSettings.getU32("LiruShowTransactionThreshold")) return;
	std::string reason =
		reason_from_transaction_type(transaction_type, item_description);
	LLStringUtil::format_map_t args;
	args["REASON"] = reason;
	args["AMOUNT"] = llformat("%d", amount);
	bool is_name_group = false;
	LLUUID name_id;
	std::string message;
	LLSD payload;
	bool you_paid_someone = (source_id == gAgentID);
	std::string gift_suffix = (transaction_type == TRANS_GIFT ? "_gift" : "");
	if (you_paid_someone)
	{
		is_name_group = is_dest_group;
		name_id = dest_id;
		if (!reason.empty())
		{
			if (dest_id.notNull())
			{
				message = success ? "you_paid_ldollars" + gift_suffix :
					          "you_paid_failure_ldollars" + gift_suffix;
			}
			else
			{
				message = success ? "you_paid_ldollars_no_name" :
									"you_paid_failure_ldollars_no_name";
			}
		}
		else
		{
			if (dest_id.notNull())
			{
				message = success ? "you_paid_ldollars_no_reason" :
									"you_paid_failure_ldollars_no_reason";
			}
			else
			{
				message = success ? "you_paid_ldollars_no_info" :
									"you_paid_failure_ldollars_no_info";
			}
		}
	}
	else
	{
		is_name_group = is_source_group;
		name_id = source_id;
		if (!reason.empty())
		{
			message = "paid_you_ldollars" + gift_suffix;
		}
		else
		{
			message = "paid_you_ldollars_no_reason";
		}
		payload["from_id"] = source_id;
		void script_msg_api(const std::string& msg);
		if (!is_source_group) script_msg_api(source_id.asString() + ", 7");
	}
	bool no_transaction_clutter = gSavedSettings.getBOOL("LiruNoTransactionClutter");
	std::string notification = no_transaction_clutter ? "Payment" : "SystemMessage";
	args["NAME"] = is_name_group ? LLGroupActions::getSLURL(name_id) : LLAvatarActions::getSLURL(name_id);
	LLSD msg_args;
	msg_args["MESSAGE"] = LLTrans::getString(message,args);
	LLNotificationsUtil::add(notification,msg_args,payload);
	if (!no_transaction_clutter) LLFloaterChat::addChat(msg_args["MESSAGE"].asString());
}
bool handle_prompt_for_maturity_level_change_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		U8 preferredMaturity = static_cast<U8>(notification["payload"]["_region_access"].asInteger());
		gSavedSettings.setU32("PreferredMaturity", static_cast<U32>(preferredMaturity));
	}
	return false;
}
bool handle_prompt_for_maturity_level_change_and_reteleport_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		U8 preferredMaturity = static_cast<U8>(notification["payload"]["_region_access"].asInteger());
		gSavedSettings.setU32("PreferredMaturity", static_cast<U32>(preferredMaturity));
		gAgent.setMaturityRatingChangeDuringTeleport(preferredMaturity);
		gAgent.restartFailedTeleportRequest();
	}
	else
	{
		gAgent.clearTeleportRequest();
	}
	return false;
}
bool handle_special_notification(std::string notificationID, LLSD& llsdBlock)
{
	bool returnValue = false;
	if (llsdBlock.has("_region_access"))
	{
		U8 regionAccess = static_cast<U8>(llsdBlock["_region_access"].asInteger());
		std::string regionMaturity = LLViewerRegion::accessToString(regionAccess);
		LLStringUtil::toLower(regionMaturity);
		llsdBlock["REGIONMATURITY"] = regionMaturity;
		LLNotificationPtr maturityLevelNotification;
		std::string notifySuffix = "_Notify";
		if (regionAccess == SIM_ACCESS_MATURE)
		{
			if (gAgent.isTeen())
			{
				gAgent.clearTeleportRequest();
				maturityLevelNotification = LLNotificationsUtil::add(notificationID + "_AdultsOnlyContent", llsdBlock);
				returnValue = true;
				notifySuffix = "_NotifyAdultsOnly";
			}
			else if (gAgent.prefersPG())
			{
				maturityLevelNotification = LLNotificationsUtil::add(notificationID + "_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
				returnValue = true;
			}
			else if (LLStringUtil::compareStrings(notificationID, "RegionEntryAccessBlocked") == 0)
			{
				maturityLevelNotification = LLNotificationsUtil::add(notificationID + "_PreferencesOutOfSync", llsdBlock, llsdBlock);
				returnValue = true;
			}
		}
		else if (regionAccess == SIM_ACCESS_ADULT)
		{
			if (!gAgent.isAdult())
			{
				gAgent.clearTeleportRequest();
				maturityLevelNotification = LLNotificationsUtil::add(notificationID + "_AdultsOnlyContent", llsdBlock);
				returnValue = true;
				notifySuffix = "_NotifyAdultsOnly";
			}
			else if (gAgent.prefersPG() || gAgent.prefersMature())
			{
				maturityLevelNotification = LLNotificationsUtil::add(notificationID + "_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
				returnValue = true;
			}
			else if (LLStringUtil::compareStrings(notificationID, "RegionEntryAccessBlocked") == 0)
			{
				maturityLevelNotification = LLNotificationsUtil::add(notificationID + "_PreferencesOutOfSync", llsdBlock, llsdBlock);
				returnValue = true;
			}
		}
		if ((maturityLevelNotification == nullptr) || maturityLevelNotification->isIgnored())
		{
			LLNotificationsUtil::add(notificationID + notifySuffix, llsdBlock);
		}
	}
	return returnValue;
}
bool handle_trusted_experiences_notification(const LLSD& llsdBlock)
{
	if (llsdBlock.has("trusted_experiences"))
	{
		std::ostringstream str;
		const LLSD& experiences = llsdBlock["trusted_experiences"];
		LLSD::array_const_iterator it = experiences.beginArray();
		for (; it != experiences.endArray(); ++it)
		{
			str << LLSLURL("experience", it->asUUID(), "profile").getSLURLString() << "\n";
		}
		std::string str_list = str.str();
		if (!str_list.empty())
		{
			LLNotificationsUtil::add("TrustedExperiencesAvailable", LLSD::emptyMap().with("EXPERIENCE_LIST", (LLSD)str_list));
			return true;
		}
	}
	return false;
}
bool handle_teleport_access_blocked(LLSD& llsdBlock, const std::string& notificationID, const std::string& defaultMessage)
{
	bool returnValue = false;
	if (llsdBlock.has("_region_access"))
	{
		U8 regionAccess = static_cast<U8>(llsdBlock["_region_access"].asInteger());
		std::string regionMaturity = LLViewerRegion::accessToString(regionAccess);
		LLStringUtil::toLower(regionMaturity);
		llsdBlock["REGIONMATURITY"] = regionMaturity;
		LLNotificationPtr tp_failure_notification;
		std::string notifySuffix;
		if (notificationID == std::string("TeleportEntryAccessBlocked"))
		{
			notifySuffix = "_Notify";
			if (regionAccess == SIM_ACCESS_MATURE)
			{
				if (gAgent.isTeen())
				{
					gAgent.clearTeleportRequest();
					tp_failure_notification = LLNotificationsUtil::add(notificationID + "_AdultsOnlyContent", llsdBlock);
					returnValue = true;
					notifySuffix = "_NotifyAdultsOnly";
				}
				else if (gAgent.prefersPG())
				{
					if (gAgent.hasRestartableFailedTeleportRequest())
					{
						tp_failure_notification = LLNotificationsUtil::add(notificationID + "_ChangeAndReTeleport", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_and_reteleport_callback);
						returnValue = true;
					}
					else
					{
						gAgent.clearTeleportRequest();
						tp_failure_notification = LLNotificationsUtil::add(notificationID + "_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
						returnValue = true;
					}
				}
				else
				{
					gAgent.clearTeleportRequest();
					tp_failure_notification = LLNotificationsUtil::add(notificationID + "_PreferencesOutOfSync", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
					returnValue = true;
				}
			}
			else if (regionAccess == SIM_ACCESS_ADULT)
			{
				if (!gAgent.isAdult())
				{
					gAgent.clearTeleportRequest();
					tp_failure_notification = LLNotificationsUtil::add(notificationID + "_AdultsOnlyContent", llsdBlock);
					returnValue = true;
					notifySuffix = "_NotifyAdultsOnly";
				}
				else if (gAgent.prefersPG() || gAgent.prefersMature())
				{
					if (gAgent.hasRestartableFailedTeleportRequest())
					{
						tp_failure_notification = LLNotificationsUtil::add(notificationID + "_ChangeAndReTeleport", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_and_reteleport_callback);
						returnValue = true;
					}
					else
					{
						gAgent.clearTeleportRequest();
						tp_failure_notification = LLNotificationsUtil::add(notificationID + "_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
						returnValue = true;
					}
				}
				else
				{
					gAgent.clearTeleportRequest();
					tp_failure_notification = LLNotificationsUtil::add(notificationID + "_PreferencesOutOfSync", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
					returnValue = true;
				}
			}
		}
		else
		{
			gAgent.clearTeleportRequest();
			if (LLNotificationTemplates::getInstance()->templateExists(notificationID))
			{
				tp_failure_notification = LLNotificationsUtil::add(notificationID, llsdBlock, llsdBlock);
			}
			else
			{
				llsdBlock["MESSAGE"] = defaultMessage;
				tp_failure_notification = LLNotificationsUtil::add("GenericAlertOK", llsdBlock);
			}
			returnValue = true;
		}
		if ((tp_failure_notification == nullptr) || tp_failure_notification->isIgnored())
		{
			LLNotificationsUtil::add(notificationID + notifySuffix, llsdBlock);
		}
	}
	handle_trusted_experiences_notification(llsdBlock);
	return returnValue;
}
void home_position_set()
{
	std::string snap_filename = gDirUtilp->getLindenUserDir();
	snap_filename += gDirUtilp->getDirDelimiter();
	snap_filename += SCREEN_HOME_FILENAME;
	gViewerWindow->saveSnapshot(snap_filename, gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw(), FALSE, FALSE);
}
void update_region_restart(const LLSD& llsdBlock)
{
	const U32 seconds = llsdBlock.has("MINUTES")
		? (60U * static_cast<U32>(llsdBlock["MINUTES"].asInteger()))
		: static_cast<U32>(llsdBlock["SECONDS"].asInteger());
	if (LLFloaterRegionRestarting* restarting_floater = LLFloaterRegionRestarting::findInstance())
	{
		restarting_floater->updateTime(seconds);
	}
	else
	{
		LLSD params;
		params["NAME"] = llsdBlock["NAME"];
		params["SECONDS"] = (LLSD::Integer)seconds;
		LLFloaterRegionRestarting::showInstance(params);
		if (gSavedSettings.getBOOL("LiruRegionRestartMinimized"))
			LLFloaterRegionRestarting::findInstance()->setMinimized(true);
	}
}
bool attempt_standard_notification(LLMessageSystem* msgsystem)
{
	if (msgsystem->has(_PREHASH_AlertInfo) && msgsystem->getNumberOfBlocksFast(_PREHASH_AlertInfo) > 0)
	{
		std::string notificationID;
		msgsystem->getStringFast(_PREHASH_AlertInfo, _PREHASH_Message, notificationID);
		if (!LLNotificationTemplates::getInstance()->templateExists(notificationID))
		{
			return false;
		}
		std::string llsdRaw;
		LLSD llsdBlock;
		msgsystem->getStringFast(_PREHASH_AlertInfo, _PREHASH_ExtraParams, llsdRaw);
		if (llsdRaw.length())
		{
			std::istringstream llsdData(llsdRaw);
			if (!LLSDSerialize::deserialize(llsdBlock, llsdData, llsdRaw.length()))
			{
				LL_WARNS() << "attempt_standard_notification: Attempted to read notification parameter data into LLSD but failed:" << llsdRaw << LL_ENDL;
			}
		}
		handle_trusted_experiences_notification(llsdBlock);
		if (
			(notificationID == "RegionEntryAccessBlocked") ||
			(notificationID == "LandClaimAccessBlocked") ||
			(notificationID == "LandBuyAccessBlocked")
		   )
		{
			if (handle_special_notification(notificationID, llsdBlock))
			{
				return true;
			}
		}
		else if (notificationID == "expired_region_handoff" || notificationID == "invalid_region_handoff")
		{
			gAgent.setIsCrossingRegion(false);
		}
		else
		if (notificationID == "HomePositionSet")
		{
			home_position_set();
		}
		else if (notificationID == "YouDiedAndGotTPHome")
		{
			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_KILLED_COUNT);
		}
		else if (notificationID == "RegionRestartMinutes" ||
				 notificationID == "RegionRestartSeconds")
		{
			update_region_restart(llsdBlock);
			LLUI::sAudioCallback(LLUUID(gSavedSettings.getString("UISndRestart")));
			return true;
		}
		else
		if (notificationID == "SLM_UPDATE_FOLDER")
		{
			std::string state = llsdBlock["state"].asString();
			if (state == "deleted")
			{
				LLMarketplaceData::instance().deleteListing(llsdBlock["listing_id"].asInteger());
				return true;
			}
			return LLMarketplaceData::instance().getListing(llsdBlock["listing_id"].asInteger());
		}
        if (notificationID == "JoinGroupError")
        {
            if (llsdBlock.has("reason"))
            {
                LLNotificationsUtil::add("JoinGroupErrorReason", llsdBlock);
                return true;
            }
            if (llsdBlock.has("group_id"))
            {
                LLGroupData agent_gdatap;
                bool is_member = gAgent.getGroupData(llsdBlock["group_id"].asUUID(), agent_gdatap);
                if (is_member)
                {
                    LLSD args;
                    args["reason"] = LLTrans::getString("AlreadyInGroup");
                    LLNotificationsUtil::add("JoinGroupErrorReason", args);
                    return true;
                }
            }
		}
		LLNotificationsUtil::add(notificationID, llsdBlock);
		return true;
	}
	return false;
}
static void process_special_alert_messages(const std::string& message)
{
	if (message == "You died and have been teleported to your home location")
	{
		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_KILLED_COUNT);
	}
	else if (message == "Home position set.")
	{
		home_position_set();
	}
}
void process_agent_alert_message(LLMessageSystem* msgsystem, void** user_data)
{
	gViewerWindow->getWindow()->resetBusyCount();
	std::string message;
	msgsystem->getStringFast(_PREHASH_AlertData, _PREHASH_Message, message);
	process_special_alert_messages(message);
	if (!attempt_standard_notification(msgsystem))
	{
		BOOL modal = FALSE;
		msgsystem->getBOOL("AlertData", "Modal", modal);
		process_alert_core(message, modal);
	}
}
void process_alert_message(LLMessageSystem* msgsystem, void** user_data)
{
	gViewerWindow->getWindow()->resetBusyCount();
	std::string message;
	msgsystem->getStringFast(_PREHASH_AlertData, _PREHASH_Message, message);
	process_special_alert_messages(message);
	if (!attempt_standard_notification(msgsystem))
	{
		BOOL modal = FALSE;
		process_alert_core(message, modal);
	}
}
bool handle_not_age_verified_alert(const std::string& pAlertName)
{
	LLNotificationPtr notification = LLNotificationsUtil::add(pAlertName);
	if ((notification == nullptr) || notification->isIgnored())
	{
		LLNotificationsUtil::add(pAlertName + "_Notify");
	}
	return true;
}
bool handle_special_alerts(const std::string& pAlertName)
{
	bool isHandled = false;
	if (LLStringUtil::compareStrings(pAlertName, "NotAgeVerified") == 0)
	{
		isHandled = handle_not_age_verified_alert(pAlertName);
	}
	return isHandled;
}
void process_alert_core(const std::string& message, BOOL modal)
{
	const std::string ALERT_PREFIX("ALERT: ");
	const std::string NOTIFY_PREFIX("NOTIFY: ");
	if (message.find(ALERT_PREFIX) == 0)
	{
		std::string alert_name(message.substr(ALERT_PREFIX.length()));
		if (!handle_special_alerts(alert_name))
		{
			LLNotificationsUtil::add(alert_name);
		}
	}
	else if (message.find(NOTIFY_PREFIX) == 0)
	{
		std::string notify_name(message.substr(NOTIFY_PREFIX.length()));
		LLNotificationsUtil::add(notify_name);
	}
	else if (message[0] == '/')
	{
		std::string text(message.substr(1));
		LLSD args;
		if (text.substr(0,17) == "RESTART_X_MINUTES")
		{
			S32 mins = 0;
			LLStringUtil::convertToS32(text.substr(18), mins);
			args["MINUTES"] = llformat("%d",mins);
			update_region_restart(args);
			LLUI::sAudioCallback(LLUUID(gSavedSettings.getString("UISndRestart")));
		}
		else if (text.substr(0,17) == "RESTART_X_SECONDS")
		{
			S32 secs = 0;
			LLStringUtil::convertToS32(text.substr(18), secs);
			args["SECONDS"] = llformat("%d",secs);
			update_region_restart(args);
			LLUI::sAudioCallback(LLUUID(gSavedSettings.getString("UISndRestart")));
		}
		else
		{
		std::string restart_cancelled = "Region restart cancelled.";
		if (text.substr(0, restart_cancelled.length()) == restart_cancelled)
			{
				LLFloaterRegionRestarting::hideInstance();
			}
			std::string new_msg =LLNotificationTemplates::instance().getGlobalString(text);
			if ( (new_msg == text) && (rlv_handler_t::isEnabled()) )
			{
				if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
					RlvUtil::filterLocation(new_msg);
				if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
					RlvUtil::filterNames(new_msg);
			}
			args["MESSAGE"] = new_msg;
			LLNotificationsUtil::add("SystemMessage", args);
		}
	}
	else if (modal)
	{
		LLSD args;
		std::string new_msg =LLNotificationTemplates::instance().getGlobalString(message);
		if ( (new_msg == message) && (rlv_handler_t::isEnabled()) )
		{
			if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
				RlvUtil::filterLocation(new_msg);
			if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
				RlvUtil::filterNames(new_msg);
		}
		args["ERROR_MESSAGE"] = new_msg;
		LLNotificationsUtil::add("ErrorMessage", args);
	}
	else
	{
		const std::string AUTOPILOT_CANCELED_MSG("Autopilot canceled");
		if (message.find(AUTOPILOT_CANCELED_MSG) == std::string::npos )
		{
			LLSD args;
			std::string new_msg =LLNotificationTemplates::instance().getGlobalString(message);
			std::string localized_msg;
			bool is_message_localized = LLTrans::findString(localized_msg, new_msg);
			if ( (new_msg == message) && (rlv_handler_t::isEnabled()) )
			{
				if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
					RlvUtil::filterLocation(new_msg);
				if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES))
					RlvUtil::filterNames(new_msg);
			}
			args["MESSAGE"] = is_message_localized ? localized_msg : new_msg;
			LLNotificationsUtil::add("SystemMessageTip", args);
		}
	}
}
mean_collision_list_t				gMeanCollisionList;
time_t								gLastDisplayedTime = 0;
void handle_show_mean_events(void*)
{
	if (gNoRender)
	{
		return;
	}
	LLFloaterBump::show(nullptr);
}
void mean_name_callback(const LLUUID& id, const LLAvatarName& av_name)
{
	if (gNoRender)
	{
		return;
	}
	static const U32 max_collision_list_size = 20;
	if (gMeanCollisionList.size() > max_collision_list_size)
	{
		mean_collision_list_t::iterator iter = gMeanCollisionList.begin();
		for (U32 i = 0; i < max_collision_list_size; i++) ++iter;
		for_each(iter, gMeanCollisionList.end(), DeletePointer());
		gMeanCollisionList.erase(iter, gMeanCollisionList.end());
	}
	for (auto mcd : gMeanCollisionList)
	{
		if (mcd->mPerp == id)
		{
			mcd->mFullName = av_name.getUserName();
		}
	}
}
void chat_mean_collision(const LLUUID& id, const LLAvatarName& avname, const EMeanCollisionType& type, const F32& mag)
{
	LLStringUtil::format_map_t args;
	if (type == MEAN_BUMP)
		args["ACT"] = LLTrans::getString("bump");
	else if (type == MEAN_LLPUSHOBJECT)
		args["ACT"] = LLTrans::getString("llpushobject");
	else if (type == MEAN_SELECTED_OBJECT_COLLIDE)
		args["ACT"] = LLTrans::getString("selected_object_collide");
	else if (type == MEAN_SCRIPTED_OBJECT_COLLIDE)
		args["ACT"] = LLTrans::getString("scripted_object_collide");
	else if (type == MEAN_PHYSICAL_OBJECT_COLLIDE)
		args["ACT"] = LLTrans::getString("physical_object_collide");
	else
		return;
	const std::string name(avname.getNSName());
	args["NAME"] = name;
	args["MAG"] = llformat("%f", mag);
	LLChat chat(LLTrans::getString("BumpedYou", args));
	chat.mFromName = name;
	chat.mURL = LLAvatarActions::getSLURL(id);
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	LLFloaterChat::addChat(chat);
}
void process_mean_collision_alert_message(LLMessageSystem* msgsystem, void** user_data)
{
	if (gAgent.inPrelude())
	{
		return;
	}
	gViewerWindow->getWindow()->resetBusyCount();
	LLUUID perp;
	U32	   time;
	U8	   u8type;
	EMeanCollisionType	   type;
	F32    mag;
	S32 i, num = msgsystem->getNumberOfBlocks(_PREHASH_MeanCollision);
	for (i = 0; i < num; i++)
	{
		msgsystem->getUUIDFast(_PREHASH_MeanCollision, _PREHASH_Perp, perp);
		msgsystem->getU32Fast(_PREHASH_MeanCollision, _PREHASH_Time, time);
		msgsystem->getF32Fast(_PREHASH_MeanCollision, _PREHASH_Mag, mag);
		msgsystem->getU8Fast(_PREHASH_MeanCollision, _PREHASH_Type, u8type);
		type = (EMeanCollisionType)u8type;
		static const LLCachedControl<bool> chat_collision("AnnounceBumps");
		if (chat_collision) LLAvatarNameCache::get(perp, boost::bind(chat_mean_collision, _1, _2, type, mag));
		BOOL b_found = FALSE;
		for (auto mcd : gMeanCollisionList)
		{
			if ((mcd->mPerp == perp) && (mcd->mType == type))
			{
				mcd->mTime = time;
				mcd->mMag = mag;
				b_found = TRUE;
				break;
			}
		}
		if (!b_found)
		{
			LLMeanCollisionData* mcd = new LLMeanCollisionData(gAgentID, perp, time, type, mag);
			gMeanCollisionList.push_front(mcd);
			LLAvatarNameCache::get(perp, boost::bind(&mean_name_callback, _1, _2));
		}
	}
}
void process_frozen_message(LLMessageSystem* msgsystem, void** user_data)
{
	gViewerWindow->getWindow()->resetBusyCount();
	BOOL b_frozen;
	msgsystem->getBOOL("FrozenData", "Data", b_frozen);
	if (b_frozen)
	{
	}
	else
	{
	}
}
void process_economy_data(LLMessageSystem* msg, void** )
{
	auto& grid = *gHippoGridManager->getConnectedGrid();
	if (grid.isSecondLife() || !LLAgentBenefitsMgr::isCurrent("NonSL")) return;
	LLAgentBenefitsMgr::current().processEconomyData(msg);
}
void notify_cautioned_script_question(const LLSD& notification, const LLSD& response, S32 orig_questions, BOOL granted)
{
	if (NACLAntiSpamRegistry::instanceExists())
	{
		if (NACLAntiSpamRegistry::instance().checkQueue(NACLAntiSpamRegistry::QUEUE_SCRIPT_DIALOG, notification["payload"]["task_id"].asUUID(), LFIDBearer::OBJECT))
			return;
	}
	if (orig_questions)
	{
		LLUIString notice(LLTrans::getString(granted ? "ScriptQuestionCautionChatGranted" : "ScriptQuestionCautionChatDenied"));
		notice.setArg("[OBJECTNAME]", notification["payload"]["object_name"].asString());
		notice.setArg("[OWNERNAME]", notification["payload"]["owner_name"].asString());
		BOOL foundpos = FALSE;
		LLViewerObject* viewobj = gObjectList.findObject(notification["payload"]["task_id"].asUUID());
		if (viewobj)
		{
			LLVector3 objpos(viewobj->getPosition());
			LLViewerRegion* viewregion = viewobj->getRegion();
			if (viewregion)
			{
				notice.setArg("[REGIONNAME]", viewregion->getName());
				std::string formatpos = llformat("%.1f, %.1f,%.1f", objpos[VX], objpos[VY], objpos[VZ]);
				notice.setArg("[REGIONPOS]", formatpos);
				foundpos = TRUE;
			}
		}
		if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
		{
			notice.setArg("[REGIONNAME]", RlvStrings::getString(RLV_STRING_HIDDEN_REGION));
			notice.setArg("[REGIONPOS]", RlvStrings::getString(RLV_STRING_HIDDEN));
		}
		else if (!foundpos)
		{
			notice.setArg("[REGIONNAME]", "(unknown region)");
			notice.setArg("[REGIONPOS]", "(unknown position)");
		}
		BOOL caution = FALSE;
		S32 count = 0;
		std::string perms;
		for (S32 i = 0; i < SCRIPT_PERMISSION_EOF; i++)
		{
			if ( (orig_questions & LSCRIPTRunTimePermissionBits[i]) &&
				 ((SCRIPT_QUESTION_IS_CAUTION[i]) || (notification["payload"]["rlv_notify"].asBoolean())) )
			{
				count++;
				caution = TRUE;
				if ((count > 1) && (i < SCRIPT_PERMISSION_EOF))
				{
					perms.append(", ");
				}
				perms.append(LLTrans::getString(SCRIPT_QUESTIONS[i]));
			}
		}
		notice.setArg("[PERMISSIONS]", perms);
		if (caution)
		{
			LLChat chat_msg(notice.getString());
			chat_msg.mFromName = SYSTEM_FROM;
			chat_msg.mFromID = LLUUID::null;
			chat_msg.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChat(chat_msg);
		}
	}
}
void script_question_mute(const LLUUID& item_id, const std::string& object_name);
void experiencePermissionBlock(LLUUID experience, LLSD result)
{
	LLSD permission;
	LLSD data;
	permission["permission"] = "Block";
	data[experience.asString()] = permission;
	data["experience"] = experience;
	LLEventPumps::instance().obtain("experience_permission").post(data);
}
bool script_question_cb(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLMessageSystem* msg = gMessageSystem;
	S32 orig = notification["payload"]["questions"].asInteger();
	S32 new_questions = orig;
	if (response["Details"])
	{
		LLNotificationsUtil::add(notification["name"], notification["substitutions"], notification["payload"]);
		LLNotificationsUtil::add("DebitPermissionDetails");
		return false;
	}
	LLUUID experience;
	if (notification["payload"].has("experience"))
	{
		experience = notification["payload"]["experience"].asUUID();
	}
	BOOL allowed = TRUE;
	if (option != 0)
	{
		new_questions = 0;
		allowed = FALSE;
	}
	else if (experience.notNull())
	{
		LLSD permission;
		LLSD data;
		permission["permission"] = "Allow";
		data[experience.asString()] = permission;
		data["experience"] = experience;
		LLEventPumps::instance().obtain("experience_permission").post(data);
	}
	LLUUID task_id = notification["payload"]["task_id"].asUUID();
	LLUUID item_id = notification["payload"]["item_id"].asUUID();
	msg->newMessageFast(_PREHASH_ScriptAnswerYes);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addUUIDFast(_PREHASH_TaskID, task_id);
	msg->addUUIDFast(_PREHASH_ItemID, item_id);
	msg->addS32Fast(_PREHASH_Questions, new_questions);
	msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));
	if ( (gSavedSettings.getBOOL("PermissionsCautionEnabled")) || (notification["payload"]["rlv_notify"].asBoolean()) )
	{
		notify_cautioned_script_question(notification, response, orig, allowed);
	}
	if ( (allowed) && (notification["payload"].has("rlv_blocked")) )
	{
		RlvUtil::notifyBlocked(notification["payload"]["rlv_blocked"], LLSD().with("OBJECT", notification["payload"]["object_name"]));
	}
	if (response["Mute"])
	{
		script_question_mute(task_id, notification["payload"]["object_name"].asString());
	}
	if (response["BlockExperience"])
	{
		if (experience.notNull())
		{
			LLViewerRegion* region = gAgent.getRegion();
			if (!region)
				return false;
			LLExperienceCache::instance().setExperiencePermission(experience, std::string("Block"), boost::bind(&experiencePermissionBlock, experience, _1));
		}
	}
	return false;
}
void script_question_mute(const LLUUID& task_id, const std::string& object_name)
{
	LLMuteList::getInstance()->add(LLMute(task_id, object_name, LLMute::OBJECT));
	class OfferMatcher final : public LLNotifyBoxView::Matcher
	{
	public:
		OfferMatcher(const LLUUID& to_block) : blocked_id(to_block)
		{
		}
		bool matches(const LLNotificationPtr notification) const override
		{
			if (notification->getName() == "ScriptQuestionCaution"
                || notification->getName() == "ScriptQuestion")
			{
			return (notification->getPayload()["task_id"].asUUID() == blocked_id);
			}
			return false;
		}
	private:
		const LLUUID& blocked_id;
	};
	gNotifyBoxView->purgeMessagesMatching(OfferMatcher(task_id));
}
static LLNotificationFunctorRegistration script_question_cb_reg_1("ScriptQuestion", script_question_cb);
static LLNotificationFunctorRegistration script_question_cb_reg_2("ScriptQuestionCaution", script_question_cb);
static LLNotificationFunctorRegistration script_question_cb_reg_3("ScriptQuestionExperience", script_question_cb);
void process_script_experience_details(const LLSD& experience_details, LLSD args, LLSD payload)
{
	if (experience_details[LLExperienceCache::PROPERTIES].asInteger() & LLExperienceCache::PROPERTY_GRID)
	{
		args["GRID_WIDE"] = LLTrans::getString("Grid-Scope");
	}
	else
	{
		args["GRID_WIDE"] = LLTrans::getString("Land-Scope");
	}
	args["EXPERIENCE"] = LLSLURL("experience", experience_details[LLExperienceCache::EXPERIENCE_ID].asUUID(), "profile").getSLURLString();
	LLNotificationsUtil::add("ScriptQuestionExperience", args, payload);
}
void process_script_question(LLMessageSystem* msg, void** user_data)
{
	LLHost sender = msg->getSender();
	LLUUID taskid;
	LLUUID itemid;
	S32		questions;
	std::string object_name;
	std::string owner_name;
	LLUUID experienceid;
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_TaskID, taskid);
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_ItemID, itemid);
	if (NACLAntiSpamRegistry::instanceExists())
	{
		auto& antispam = NACLAntiSpamRegistry::instance();
		if ((taskid.isNull()
			&& antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SCRIPT_DIALOG, itemid, LFIDBearer::NONE))
			|| antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SCRIPT_DIALOG, taskid, LFIDBearer::OBJECT))
			return;
	}
	msg->getStringFast(_PREHASH_Data, _PREHASH_ObjectName, object_name);
	msg->getStringFast(_PREHASH_Data, _PREHASH_ObjectOwner, owner_name);
	msg->getS32Fast(_PREHASH_Data, _PREHASH_Questions, questions);
	if (msg->has(_PREHASH_Experience))
	{
		msg->getUUIDFast(_PREHASH_Experience, _PREHASH_ExperienceID, experienceid);
	}
	std::string throttle_name = owner_name;
	std::string self_name;
	LLAgentUI::buildFullname(self_name);
	if (is_spam_filtered(IM_COUNT, false, owner_name == self_name)) return;
	if (owner_name == self_name)
	{
		throttle_name = taskid.getString();
	}
	if (LLMuteList::getInstance()->isMuted(taskid)) return;
	typedef LLKeyThrottle<std::string> LLStringThrottle;
	static LLStringThrottle question_throttle(LLREQUEST_PERMISSION_THROTTLE_LIMIT, LLREQUEST_PERMISSION_THROTTLE_INTERVAL);
	switch (question_throttle.noteAction(throttle_name))
	{
		case LLStringThrottle::THROTTLE_NEWLY_BLOCKED:
			LL_INFOS("Messaging") << "process_script_question throttled"
					<< " owner_name:" << owner_name
					<< LL_ENDL;
		case LLStringThrottle::THROTTLE_BLOCKED:
			return;
		case LLStringThrottle::THROTTLE_OK:
			break;
	}
	std::string script_question;
	if (questions)
	{
		bool caution = false;
		S32 count = 0;
		LLSD args;
		const std::string get_obj_slurl(const LLUUID& id, const std::string& name);
		const std::string get_obj_owner_slurl(const LLUUID& obj_id, const std::string& name, bool* group_ownedp = nullptr);
		args["OBJECTNAME"] = get_obj_slurl(taskid, object_name);
		args["NAME"] = get_obj_owner_slurl(taskid, owner_name);
		S32 known_questions = 0;
		bool has_not_only_debit = questions ^ LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_DEBIT];
		for (S32 i = 0; i < SCRIPT_PERMISSION_EOF; i++)
		{
			if (questions & LSCRIPTRunTimePermissionBits[i])
			{
				count++;
				known_questions |= LSCRIPTRunTimePermissionBits[i];
				caution |= (SCRIPT_QUESTION_IS_CAUTION[i]);
				if (("ScriptTakeMoney" ==  SCRIPT_QUESTIONS[i]) && has_not_only_debit)
					continue;
				if (SCRIPT_QUESTIONS[i] == "JoinAnExperience")
				{
					script_question += "    " + LLTrans::getString("ForceSitAvatar") + "\n";
				}
				script_question += "    " + LLTrans::getString(SCRIPT_QUESTIONS[i]) + "\n";
			}
		}
		args["QUESTIONS"] = script_question;
		if (known_questions != questions)
		{
			LL_WARNS("Messaging") << "Object \"" << object_name << "\" requested " << script_question
								<< " permission. Permission is unknown and can't be granted. Item id: " << itemid
								<< " taskid:" << taskid << LL_ENDL;
		}
		if (known_questions)
		{
			LLSD payload;
			payload["task_id"] = taskid;
			payload["item_id"] = itemid;
			payload["sender"] = sender.getIPandPort();
			payload["questions"] = known_questions;
			payload["object_name"] = object_name;
			payload["owner_name"] = owner_name;
			const char* notification = "ScriptQuestion";
			if (rlv_handler_t::isEnabled())
			{
				RlvUtil::filterScriptQuestions(questions, payload);
				if ( (questions) && (gRlvHandler.hasBehaviour(RLV_BHVR_ACCEPTPERMISSION)) )
				{
					const LLViewerObject* pObj = gObjectList.findObject(taskid);
					if (pObj)
					{
						if ( (pObj->permYouOwner()) && (!pObj->isAttachment()) )
						{
							questions &= ~(LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TAKE_CONTROLS] |
								LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_ATTACH]);
						}
						else
						{
							questions &= ~(LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TAKE_CONTROLS]);
						}
						payload["rlv_notify"] = !pObj->permYouOwner();
					}
				}
			}
			if ( (!caution) && (!questions) )
			{
				LLNotifications::instance().forceResponse(
					LLNotification::Params(notification).substitutions(args).payload(payload), 0);
				return;
			}
			if (caution && gSavedSettings.getBOOL("PermissionsCautionEnabled"))
			{
				args["FOOTERTEXT"] = (count > 1) ? LLTrans::getString("AdditionalPermissionsRequestHeader") + '\n' + script_question : LLStringUtil::null;
				notification = "ScriptQuestionCaution";
			}
			else if (experienceid.notNull())
			{
				payload["experience"] = experienceid;
					LLExperienceCache::instance().get(experienceid, boost::bind(process_script_experience_details, _1, args, payload));
				return;
			}
			LLNotificationsUtil::add(notification, args, payload);
		}
	}
}
void process_derez_container(LLMessageSystem* msg, void**)
{
	LL_WARNS("Messaging") << "call to deprecated process_derez_container" << LL_ENDL;
}
void container_inventory_arrived(LLViewerObject* object,
								 LLInventoryObject::object_list_t* inventory,
								 S32 serial_num,
								 void* data)
{
	LL_DEBUGS("Messaging") << "container_inventory_arrived()" << LL_ENDL;
	if (gAgentCamera.cameraMouselook())
	{
		gAgentCamera.changeCameraToDefault();
	}
	LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel();
	if (inventory->size() > 2)
	{
		LLUUID cat_id;
		cat_id = gInventory.createNewCategory(gInventory.getRootFolderID(),
											  LLFolderType::FT_NONE,
											  LLTrans::getString("AcquiredItems"));
		LLInventoryObject::object_list_t::const_iterator it = inventory->begin();
		LLInventoryObject::object_list_t::const_iterator end = inventory->end();
		for (; it != end; ++it)
		{
			if ((*it)->getType() != LLAssetType::AT_CATEGORY)
			{
				LLInventoryObject* obj = (LLInventoryObject*)(*it);
				LLInventoryItem* item = (LLInventoryItem*)(obj);
				LLUUID item_id;
				item_id.generate();
				time_t creation_date_utc = time_corrected();
				LLPointer<LLViewerInventoryItem> new_item
					= new LLViewerInventoryItem(item_id,
												cat_id,
												item->getPermissions(),
												item->getAssetUUID(),
												item->getType(),
												item->getInventoryType(),
												item->getName(),
												item->getDescription(),
												LLSaleInfo::DEFAULT,
												item->getFlags(),
												creation_date_utc);
				new_item->updateServer(TRUE);
				gInventory.updateItem(new_item);
			}
		}
		gInventory.notifyObservers();
		if (active_panel)
		{
			active_panel->setSelection(cat_id, TAKE_FOCUS_NO);
		}
	}
	else if (inventory->size() == 2)
	{
		LLInventoryObject::object_list_t::iterator it = inventory->begin();
		if ((*it)->getType() == LLAssetType::AT_CATEGORY)
		{
			++it;
		}
		LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
		const LLUUID category = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(item->getType()));
		LLUUID item_id;
		item_id.generate();
		time_t creation_date_utc = time_corrected();
		LLPointer<LLViewerInventoryItem> new_item
			= new LLViewerInventoryItem(item_id, category,
										item->getPermissions(),
										item->getAssetUUID(),
										item->getType(),
										item->getInventoryType(),
										item->getName(),
										item->getDescription(),
										LLSaleInfo::DEFAULT,
										item->getFlags(),
										creation_date_utc);
		new_item->updateServer(TRUE);
		gInventory.updateItem(new_item);
		gInventory.notifyObservers();
		if (active_panel)
		{
			active_panel->setSelection(item_id, TAKE_FOCUS_NO);
		}
	}
	BOOL delete_object = (BOOL)(intptr_t)data;
	LLViewerRegion* region = gAgent.getRegion();
	if (delete_object && region)
	{
		gMessageSystem->newMessageFast(_PREHASH_ObjectDelete);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		const U8 NO_FORCE = 0;
		gMessageSystem->addU8Fast(_PREHASH_Force, NO_FORCE);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
		gMessageSystem->sendReliable(region->getHost());
	}
}
std::string formatted_time(const time_t& the_time)
{
	std::string timestr;
	timeToFormattedString(the_time, gSavedSettings.getString("TimestampFormat"), timestr);
	char buffer[30];
	LLStringUtil::copy(buffer, timestr.c_str(), 30);
	buffer[24] = '\0';
	return std::string(buffer);
}
void process_teleport_failed(LLMessageSystem* msg, void**)
{
	std::string message_id;
	std::string big_reason;
	LLSD args;
	LLViewerParcelMgr::getInstance()->onTeleportFailed();
	if (msg->has(_PREHASH_AlertInfo) && msg->getSizeFast(_PREHASH_AlertInfo, _PREHASH_Message) > 0)
	{
		msg->getStringFast(_PREHASH_AlertInfo, _PREHASH_Message, message_id);
		big_reason = LLAgent::sTeleportErrorMessages[message_id];
		if (big_reason.empty())
		{
			msg->getStringFast(_PREHASH_Info, _PREHASH_Reason, big_reason);
		}
		args["REASON"] = big_reason;
		LLSD llsd_block;
		std::string llsd_raw;
		msg->getStringFast(_PREHASH_AlertInfo, _PREHASH_ExtraParams, llsd_raw);
		if (!llsd_raw.empty())
		{
			std::istringstream llsd_data(llsd_raw);
			if (!LLSDSerialize::deserialize(llsd_block, llsd_data, llsd_raw.length()))
			{
				LL_WARNS() << "process_teleport_failed: Attempted to read alert parameter data into LLSD but failed:" << llsd_raw << LL_ENDL;
			}
			else
			{
				if(llsd_block.has("REGION_NAME"))
				{
					std::string region_name = llsd_block["REGION_NAME"].asString();
					if(!region_name.empty())
					{
						LLStringUtil::format_map_t name_args;
						name_args["[REGION_NAME]"] = region_name;
						LLStringUtil::format(big_reason, name_args);
						args["REASON"] = big_reason;
					}
				}
				if (handle_teleport_access_blocked(llsd_block, message_id, args["REASON"]))
				{
					if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
					{
						gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
					}
					return;
				}
			}
		}
	}
	else
	{
		msg->getStringFast(_PREHASH_Info, _PREHASH_Reason, message_id);
		big_reason = LLAgent::sTeleportErrorMessages[message_id];
		if (!big_reason.empty())
		{
			args["REASON"] = big_reason;
		}
		else
		{
			args["REASON"] = message_id;
		}
	}
	LLNotificationsUtil::add("CouldNotTeleportReason", args);
	if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
	{
		gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
	}
}
void process_teleport_local(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_Info, _PREHASH_AgentID, agent_id);
	if (agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "Got teleport notification for wrong agent!" << LL_ENDL;
		return;
	}
	U32 location_id;
	LLVector3 pos, look_at;
	U32 teleport_flags;
	msg->getU32Fast(_PREHASH_Info, _PREHASH_LocationID, location_id);
	msg->getVector3Fast(_PREHASH_Info, _PREHASH_Position, pos);
	msg->getVector3Fast(_PREHASH_Info, _PREHASH_LookAt, look_at);
	msg->getU32Fast(_PREHASH_Info, _PREHASH_TeleportFlags, teleport_flags);
	if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
	{
		if (gAgent.getTeleportState() == LLAgent::TELEPORT_LOCAL)
		{
			gTeleportDisplayTimer.reset();
			gTeleportDisplay = TRUE;
		}
		else
		{
			gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
		}
	}
	if (teleport_flags & TELEPORT_FLAGS_IS_FLYING || gSavedSettings.getBOOL("LiruFlyAfterTeleport"))
	{
		gAgent.setFlying(TRUE);
	}
	else
	{
		gAgent.setFlying(FALSE);
	}
	gAgent.setPositionAgent(pos);
	gAgentCamera.slamLookAt(look_at);
	if (!(gAgent.getTeleportKeepsLookAt() && LLViewerJoystick::getInstance()->getOverrideCamera()) && gSavedSettings.getBOOL("OptionRotateCamAfterLocalTP"))
	{
		gAgentCamera.resetView(TRUE, TRUE);
	}
	gAgentCamera.updateCamera();
	send_agent_update(TRUE, TRUE);
	LLViewerParcelMgr::getInstance()->onTeleportFinished(true, gAgent.getPosGlobalFromAgent(pos));
}
void send_simple_im(const LLUUID& to_id,
					const std::string& message,
					EInstantMessage dialog,
					const LLUUID& id)
{
	std::string my_name;
	LLAgentUI::buildFullname(my_name);
	send_improved_im(to_id,
					 my_name,
					 message,
					 IM_ONLINE,
					 dialog,
					 id,
					 NO_TIMESTAMP,
					 (U8*)EMPTY_BINARY_BUCKET,
					 EMPTY_BINARY_BUCKET_SIZE);
}
void send_group_notice(const LLUUID& group_id,
					   const std::string& subject,
					   const std::string& message,
					   const LLInventoryItem* item)
{
	std::string my_name;
	LLAgentUI::buildFullname(my_name);
	std::ostringstream subject_and_message;
	subject_and_message << subject << "|" << message;
	U8 bin_bucket[MAX_INVENTORY_BUFFER_SIZE];
	U8* bucket_to_send = bin_bucket;
	bin_bucket[0] = '\0';
	S32 bin_bucket_size = EMPTY_BINARY_BUCKET_SIZE;
	if (item)
	{
		LLSD item_def;
		item_def["item_id"] = item->getUUID();
		item_def["owner_id"] = item->getPermissions().getOwner();
		std::ostringstream ostr;
		LLSDSerialize::serialize(item_def, ostr, LLSDSerialize::LLSD_XML);
		bin_bucket_size = ostr.str().copy(
			(char*)bin_bucket, ostr.str().size());
		bin_bucket[bin_bucket_size] = '\0';
	}
	else
	{
		bucket_to_send = (U8*)EMPTY_BINARY_BUCKET;
	}
	send_improved_im(
			group_id,
			my_name,
			subject_and_message.str(),
			IM_ONLINE,
			IM_GROUP_NOTICE,
			LLUUID::null,
			NO_TIMESTAMP,
			bucket_to_send,
			bin_bucket_size);
}
void send_lures(const LLSD& notification, const LLSD& response)
{
	std::string text = response["message"].asString();
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	text.append("\r\n").append(slurl.getSLURLString());
	const std::string& rlv_hidden(RlvStrings::getString(RLV_STRING_HIDDEN));
	if ( (RlvActions::hasBehaviour(RLV_BHVR_SENDIM)) || (RlvActions::hasBehaviour(RLV_BHVR_SENDIMTO)) )
	{
		for (auto const& entry : notification["payload"]["ids"].array())
		{
			if (!RlvActions::canSendIM(entry.asUUID()))
			{
				text = rlv_hidden;
				break;
			}
		}
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_StartLure);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Info);
	msg->addU8Fast(_PREHASH_LureType, (U8)0);
	msg->addStringFast(_PREHASH_Message, text);
	bool fRlvHideName = gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES);
	bool fRlvNoNearbyNames = gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS);
	for (auto const& entry : notification["payload"]["ids"].array())
	{
		LLUUID target_id = entry.asUUID();
		msg->nextBlockFast(_PREHASH_TargetData);
		msg->addUUIDFast(_PREHASH_TargetID, target_id);
		if (notification["payload"]["ids"].size() < 10)
		{
			fRlvHideName |= notification["payload"]["rlv_shownames"].asBoolean();
			std::string target_name;
			gCacheName->getFullName(target_id, target_name);
			LLSD args;
			if (fRlvNoNearbyNames && RlvUtil::isNearbyAgent(target_id))
				target_name = rlv_hidden;
			else if (fRlvHideName)
				target_name = RlvStrings::getAnonym(target_name);
			else
				target_name = LLAvatarActions::getSLURL(target_id);
			args["TO_NAME"] = target_name;
			LLSD payload;
			payload["from_id"] = target_id;
			payload["SUPPRESS_TOAST"] = true;
			LLNotificationsUtil::add("TeleportOfferSent", args, payload);
		}
	}
	gAgent.sendReliableMessage();
}
bool handle_lure_callback(const LLSD& notification, const LLSD& response)
{
	static const unsigned OFFER_RECIPIENT_LIMIT = 250;
	if (notification["payload"]["ids"].size() > OFFER_RECIPIENT_LIMIT)
	{
		LLSD args;
		args["OFFERS"] = notification["payload"]["ids"].size();
		args["LIMIT"] = static_cast<int>(OFFER_RECIPIENT_LIMIT);
		LLNotificationsUtil::add("TooManyTeleportOffers", args);
		return false;
	}
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		send_lures(notification, response);
	}
	return false;
}
void handle_lure(const LLUUID& invitee)
{
	handle_lure(uuid_vec_t{invitee});
}
void handle_lure(const uuid_vec_t& ids)
{
	if (ids.empty()) return;
	if (!gAgent.getRegion()) return;
	LLSD edit_args;
	edit_args["REGION"] = (!gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) ? gAgent.getRegion()->getName() : RlvStrings::getString(RLV_STRING_HIDDEN);
	LLSD payload;
	bool fRlvShouldHideNames = false;
	for (const LLUUID& idAgent : ids)
	{
		if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
		{
			const LLRelationship* pBuddyInfo = LLAvatarTracker::instance().getBuddyInfo(idAgent);
			if ( (!gRlvHandler.isException(RLV_BHVR_TPLURE, idAgent, RLV_CHECK_PERMISSIVE)) &&
				 ((!pBuddyInfo) || (!pBuddyInfo->isOnline()) || (!pBuddyInfo->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION))) )
			{
				return;
			}
		}
		fRlvShouldHideNames |= !RlvActions::canShowName(RlvActions::SNC_TELEPORTOFFER);
		payload["ids"].append(idAgent);
	}
	payload["rlv_shownames"] = fRlvShouldHideNames;
	if (gAgent.isGodlike())
	{
		LLNotificationsUtil::add("OfferTeleportFromGod", edit_args, payload, handle_lure_callback);
	}
	else
	{
		LLNotificationsUtil::add("OfferTeleport", edit_args, payload, handle_lure_callback);
	}
}
bool teleport_request_callback(const LLSD& notification, const LLSD& response)
{
	LLUUID from_id = notification["payload"]["from_id"].asUUID();
	if (from_id.isNull())
	{
		LL_WARNS() << "from_id is NULL" << LL_ENDL;
		return false;
	}
	std::string from_name;
	gCacheName->getFullName(from_id, from_name);
	if (LLMuteList::getInstance()->isMuted(from_id) && !LLMuteList::getInstance()->isLinden(from_name))
	{
		return false;
	}
	S32 option = 0;
	if (response.isInteger())
	{
		option = response.asInteger();
	}
	else
	{
		option = LLNotificationsUtil::getSelectedOption(notification, response);
	}
	switch (option)
	{
	case 0:
		{
			LLSD dummy_notification;
			dummy_notification["payload"]["ids"][0] = from_id;
			LLSD dummy_response;
			dummy_response["message"] = response["message"];
			send_lures(dummy_notification, dummy_response);
		}
		break;
	case 1:
	default:
		break;
	}
	return false;
}
static LLNotificationFunctorRegistration teleport_request_callback_reg("TeleportRequest", teleport_request_callback);
void send_improved_im(const LLUUID& to_id,
							const std::string& name,
							const std::string& message,
							U8 offline,
							EInstantMessage dialog,
							const LLUUID& id,
							U32 timestamp,
							const U8* binary_bucket,
							S32 binary_bucket_size)
{
	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		to_id,
		name,
		message,
		offline,
		dialog,
		id,
		0,
		LLUUID::null,
		gAgent.getPositionAgent(),
		timestamp,
		binary_bucket,
		binary_bucket_size);
	gAgent.sendReliableMessage();
}
void send_places_query(const LLUUID& query_id,
					   const LLUUID& trans_id,
					   const std::string& query_text,
					   U32 query_flags,
					   S32 category,
					   const std::string& sim_name)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("PlacesQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addUUID("QueryID", query_id);
	msg->nextBlock("TransactionData");
	msg->addUUID("TransactionID", trans_id);
	msg->nextBlock("QueryData");
	msg->addString("QueryText", query_text);
	msg->addU32("QueryFlags", query_flags);
	msg->addS8("Category", (S8)category);
	msg->addString("SimName", sim_name);
	gAgent.sendReliableMessage();
}
void process_user_info_reply(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if (agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "process_user_info_reply - "
				<< "wrong agent id." << LL_ENDL;
	}
	BOOL im_via_email;
	msg->getBOOLFast(_PREHASH_UserData, _PREHASH_IMViaEMail, im_via_email);
	std::string email;
	msg->getStringFast(_PREHASH_UserData, _PREHASH_EMail, email);
	std::string dir_visibility;
	msg->getString("UserData", "DirectoryVisibility", dir_visibility);
	LLFloaterPreference::updateUserInfo(dir_visibility, im_via_email, email);
	LLFloaterPostcard::updateUserInfo(email);
}
const S32 SCRIPT_DIALOG_MAX_BUTTONS = 12;
const char* SCRIPT_DIALOG_HEADER = "Script Dialog:\n";
bool callback_script_dialog(const LLSD& notification, const LLSD& response)
{
	LLNotificationForm form(notification["form"]);
	std::string rtn_text;
	S32 button_idx = LLNotification::getSelectedOption(notification, response);
	if (notification["payload"].has("textbox"))
	{
		rtn_text = response["message"].asString();
	}
	else
	{
		rtn_text = LLNotification::getSelectedOptionName(response);
	}
	if (-2 == button_idx)
	{
		std::string object_name = notification["payload"]["object_name"].asString();
		LLUUID object_id = notification["payload"]["object_id"].asUUID();
		LLMute mute(object_id, object_name, LLMute::OBJECT);
		if (LLMuteList::getInstance()->add(mute))
		{
			LLFloaterMute::showInstance()->selectMute(object_id);
		}
	}
	if (0 <= button_idx)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ScriptDialogReply");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("ObjectID", notification["payload"]["object_id"].asUUID());
		msg->addS32("ChatChannel", notification["payload"]["chat_channel"].asInteger());
		msg->addS32("ButtonIndex", button_idx);
		msg->addString("ButtonLabel", rtn_text);
		msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));
	}
	return false;
}
static LLNotificationFunctorRegistration callback_script_dialog_reg_1("ScriptDialog", callback_script_dialog);
static LLNotificationFunctorRegistration callback_script_dialog_reg_2("ScriptDialogGroup", callback_script_dialog);
void process_script_dialog(LLMessageSystem* msg, void**)
{
	S32 i;
	LLSD payload;
	LLUUID object_id;
	msg->getUUID("Data", "ObjectID", object_id);
	auto antispam = NACLAntiSpamRegistry::getIfExists();
	if (antispam && antispam->checkQueue(NACLAntiSpamRegistry::QUEUE_SCRIPT_DIALOG, object_id, LFIDBearer::OBJECT))
		return;
	std::string first_name;
	msg->getString("Data", "FirstName", first_name);
	bool const is_group = first_name.empty();
    LLUUID owner_id;
	if (gMessageSystem->getNumberOfBlocks("OwnerData") > 0)
	{
    msg->getUUID("OwnerData", "OwnerID", owner_id);
	if (antispam && antispam->checkQueue(NACLAntiSpamRegistry::QUEUE_SCRIPT_DIALOG, owner_id, is_group ? LFIDBearer::GROUP : LFIDBearer::AVATAR))
		return;
	}
	bool has_owner = owner_id.notNull();
	if (!has_owner ? is_spam_filtered(IM_COUNT, LLAvatarActions::isFriend(object_id), object_id == gAgentID)
		: is_spam_filtered(IM_COUNT, LLAvatarActions::isFriend(owner_id), !is_group && owner_id == gAgentID)) return;
	if (LLMuteList::getInstance()->isMuted(object_id) || LLMuteList::getInstance()->isMuted(owner_id))
	{
		return;
	}
	auto chatter = gObjectList.findObject(object_id);
	if (chatter && has_owner) chatter->mOwnerID = owner_id;
	std::string message;
	std::string last_name;
	std::string object_name;
	S32 chat_channel;
	msg->getString("Data", "LastName", last_name);
	msg->getString("Data", "ObjectName", object_name);
	msg->getString("Data", "Message", message);
	msg->getS32("Data", "ChatChannel", chat_channel);
	LLUUID image_id;
	msg->getUUID("Data", "ImageID", image_id);
	payload["sender"] = msg->getSender().getIPandPort();
	payload["object_id"] = object_id;
	payload["object_name"] = object_name;
	payload["chat_channel"] = chat_channel;
	S32 button_count = msg->getNumberOfBlocks("Buttons");
	if (button_count > SCRIPT_DIALOG_MAX_BUTTONS)
	{
		LL_WARNS() << "Too many script dialog buttons - omitting some" << LL_ENDL;
		button_count = SCRIPT_DIALOG_MAX_BUTTONS;
	}
	bool is_text_box = false;
	LLNotificationForm form;
	for (i = 0; i < button_count; i++)
	{
		std::string tdesc;
		msg->getString("Buttons", "ButtonLabel", tdesc, i);
		if (is_text_box = tdesc == TEXTBOX_MAGIC_TOKEN)
			break;
		form.addElement("button", std::string(tdesc));
	}
	LLSD query_string;
	query_string["owner"] = owner_id;
	if (rlv_handler_t::isEnabled() && (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES) || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS)) && !is_group && RlvUtil::isNearbyAgent(owner_id))
	{
		query_string["rlv_shownames"] = true;
	}
	if (const auto& obj = chatter ? chatter : gObjectList.findObject(owner_id))
	{
		auto& slurl = query_string["slurl"];
		const auto& region = obj->getRegion();
		if (rlv_handler_t::isEnabled() && gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC) && LLWorld::instance().isRegionListed(region))
			slurl = RlvStrings::getString(RLV_STRING_HIDDEN_REGION);
		else
		{
			const auto& pos = obj->getPositionRegion();
			S32 x = ll_round((F32)fmod((F64)pos.mV[VX], (F64)REGION_WIDTH_METERS));
			S32 y = ll_round((F32)fmod((F64)pos.mV[VY], (F64)REGION_WIDTH_METERS));
			S32 z = ll_round((F32)pos.mV[VZ]);
			std::ostringstream location;
			location << region->getName() << '/' << x << '/' << y << '/' << z;
			if (chatter != obj) location << "?owner_not_object";
			slurl = location.str();
		}
	}
	query_string["name"] = object_name;
	query_string["groupowned"] = is_group;
	object_name = LLSLURL("objectim", object_id, LLURI::mapToQueryString(query_string)).getSLURLString();
	LLSD args;
	args["TITLE"] = object_name;
	args["MESSAGE"] = LLStringUtil::null;
	args["SCRIPT_MESSAGE"] = message;
	args["CHANNEL"] = chat_channel;
	LLNotificationPtr notification;
	char const* name = (is_group && !is_text_box) ? "GROUPNAME" : "NAME";
	args[name] = has_owner ? is_group ? LLGroupActions::getSLURL(owner_id) : LLAvatarActions::getSLURL(owner_id) :
		is_group ? last_name : LLCacheName::buildFullName(first_name, last_name);
	if (is_text_box)
	{
		payload["textbox"] = true;
		LLNotificationsUtil::add("ScriptTextBoxDialog", args, payload, callback_script_dialog);
	}
	else if (!is_group)
	{
		notification = LLNotifications::instance().add(
			LLNotification::Params("ScriptDialog").substitutions(args).payload(payload).form_elements(form.asLLSD()));
	}
	else
	{
		notification = LLNotifications::instance().add(
			LLNotification::Params("ScriptDialogGroup").substitutions(args).payload(payload).form_elements(form.asLLSD()));
	}
}
std::vector<LLSD> gLoadUrlList;
bool callback_load_url(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLWeb::loadURL(notification["payload"]["url"].asString());
	}
	return false;
}
static LLNotificationFunctorRegistration callback_load_url_reg("LoadWebPage", callback_load_url);
void callback_load_url_name(const LLUUID& id, const std::string& full_name, bool is_group)
{
	std::vector<LLSD>::iterator it;
	for (it = gLoadUrlList.begin(); it != gLoadUrlList.end();)
	{
		LLSD load_url_info = *it;
		if (load_url_info["owner_id"].asUUID() == id)
		{
			it = gLoadUrlList.erase(it);
			std::string owner_name;
			if (is_group)
			{
				owner_name = full_name + LLTrans::getString("Group");
			}
			else
			{
				owner_name = full_name;
			}
			if (LLMuteList::getInstance()->isMuted(LLUUID::null, owner_name))
			{
				continue;
			}
			LLSD args;
			args["URL"] = load_url_info["url"].asString();
			args["MESSAGE"] = load_url_info["message"].asString();
			args["OBJECTNAME"] = load_url_info["object_name"].asString();
			args["NAME"] = owner_name;
			LLNotificationsUtil::add("LoadWebPage", args, load_url_info);
		}
		else
		{
			++it;
		}
	}
}
void callback_load_url_avatar_name(const LLUUID& id, const LLAvatarName& av_name)
{
	callback_load_url_name(id, av_name.getUserName(), false);
}
void process_load_url(LLMessageSystem* msg, void**)
{
	LLUUID object_id;
	LLUUID owner_id;
	BOOL owner_is_group;
	char object_name[256];
	char message[256];
	char url[256];
	msg->getString("Data", "ObjectName", 256, object_name);
	msg->getUUID("Data", "ObjectID", object_id);
	msg->getUUID("Data", "OwnerID", owner_id);
	if (owner_id.isNull() ? is_spam_filtered(IM_COUNT, LLAvatarActions::isFriend(object_id), object_id == gAgentID)
		: is_spam_filtered(IM_COUNT, LLAvatarActions::isFriend(owner_id), owner_id == gAgentID)) return;
	msg->getBOOL("Data", "OwnerIsGroup", owner_is_group);
	if (NACLAntiSpamRegistry::instanceExists())
	{
		auto& antispam = NACLAntiSpamRegistry::instance();
		if ((owner_id.isNull()
			&& antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SCRIPT_DIALOG, object_id, LFIDBearer::OBJECT))
			|| antispam.checkQueue(NACLAntiSpamRegistry::QUEUE_SCRIPT_DIALOG, owner_id, owner_is_group ? LFIDBearer::GROUP : LFIDBearer::AVATAR))
			return;
	}
	msg->getString("Data", "Message", 256, message);
	msg->getString("Data", "URL", 256, url);
	LLSD payload;
	payload["object_id"] = object_id;
	payload["owner_id"] = owner_id;
	payload["owner_is_group"] = owner_is_group;
	payload["object_name"] = object_name;
	payload["message"] = message;
	payload["url"] = url;
	if (LLMuteList::getInstance()->isMuted(object_id, object_name) ||
	    LLMuteList::getInstance()->isMuted(owner_id))
	{
		LL_INFOS("Messaging") << "Ignoring load_url from muted object/owner." << LL_ENDL;
		return;
	}
	gLoadUrlList.push_back(payload);
	if (owner_is_group)
	{
		gCacheName->getGroup(owner_id, boost::bind(&callback_load_url_name, _1, _2, _3));
	}
	else
	{
		LLAvatarNameCache::get(owner_id, boost::bind(&callback_load_url_avatar_name, _1, _2));
	}
}
void callback_download_complete(void** data, S32 result, LLExtStat ext_status)
{
	std::string* filepath = (std::string*)data;
	LLSD args;
	args["DOWNLOAD_PATH"] = *filepath;
	LLNotificationsUtil::add("FinishedRawDownload", args);
	delete filepath;
}
void process_initiate_download(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	if (agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "Initiate download for wrong agent" << LL_ENDL;
		return;
	}
	std::string sim_filename;
	std::string viewer_filename;
	msg->getString("FileData", "SimFilename", sim_filename);
	msg->getString("FileData", "ViewerFilename", viewer_filename);
	if (!gXferManager->validateFileForRequest(viewer_filename))
	{
		LL_WARNS() << "SECURITY: Unauthorized download to local file " << viewer_filename << LL_ENDL;
		return;
	}
	gXferManager->requestFile(viewer_filename,
		sim_filename,
		LL_PATH_NONE,
		msg->getSender(),
		FALSE,
		callback_download_complete,
		(void**)new std::string(viewer_filename));
}
void process_script_teleport_request(LLMessageSystem* msg, void**)
{
	if (!gSavedSettings.getBOOL("ScriptsCanShowUI")) return;
	if (is_spam_filtered(IM_COUNT, false, false)) return;
	std::string object_name;
	std::string sim_name;
	LLVector3 pos;
	LLVector3 look_at;
	msg->getString("Data", "ObjectName", object_name);
	msg->getString("Data", "SimName", sim_name);
	msg->getVector3("Data", "SimPosition", pos);
	msg->getVector3("Data", "LookAt", look_at);
	gFloaterWorldMap->trackURL(sim_name, (S32)pos.mV[VX], (S32)pos.mV[VY], (S32)pos.mV[VZ]);
	LLFloaterWorldMap::show(true);
}
void process_covenant_reply(LLMessageSystem* msg, void**)
{
	LLUUID covenant_id, estate_owner_id;
	std::string estate_name;
	U32 covenant_timestamp;
	msg->getUUID("Data", "CovenantID", covenant_id);
	msg->getU32("Data", "CovenantTimestamp", covenant_timestamp);
	msg->getString("Data", "EstateName", estate_name);
	msg->getUUID("Data", "EstateOwnerID", estate_owner_id);
	LLPanelEstateCovenant::updateEstateName(estate_name);
	LLPanelLandCovenant::updateEstateName(estate_name);
	LLPanelEstateInfo::updateEstateName(estate_name);
	LLFloaterBuyLand::updateEstateName(estate_name);
	LLPanelEstateCovenant::updateEstateOwnerID(estate_owner_id);
	LLPanelLandCovenant::updateEstateOwnerID(estate_owner_id);
	LLPanelEstateInfo::updateEstateOwnerID(estate_owner_id);
	LLFloaterBuyLand::updateEstateOwnerID(estate_owner_id);
	std::string last_modified;
	if (covenant_timestamp == 0)
	{
		last_modified = LLTrans::getString("covenant_never_modified");
	}
	else
	{
		last_modified = LLTrans::getString("covenant_modified") + ' ' + formatted_time((time_t)covenant_timestamp);
	}
	LLPanelEstateCovenant::updateLastModified(last_modified);
	LLPanelLandCovenant::updateLastModified(last_modified);
	LLFloaterBuyLand::updateLastModified(last_modified);
	const BOOL high_priority = TRUE;
	if (covenant_id.notNull())
	{
		gAssetStorage->getEstateAsset(gAgent.getRegionHost(),
									gAgent.getID(),
									gAgent.getSessionID(),
									covenant_id,
                                    LLAssetType::AT_NOTECARD,
									ET_Covenant,
                                    onCovenantLoadComplete,
		                              nullptr,
									high_priority);
	}
	else
	{
		std::string covenant_text;
		if (estate_owner_id.isNull())
		{
			covenant_text = LLTrans::getString("RegionNoCovenant");
		}
		else
		{
			covenant_text = LLTrans::getString("RegionNoCovenantOtherOwner");
		}
		LLPanelEstateCovenant::updateCovenantText(covenant_text, covenant_id);
		LLPanelLandCovenant::updateCovenantText(covenant_text);
		LLFloaterBuyLand::updateCovenantText(covenant_text, covenant_id);
	}
}
void onCovenantLoadComplete(LLVFS* vfs,
					const LLUUID& asset_uuid,
					LLAssetType::EType type,
					void* user_data, S32 status, LLExtStat ext_status)
{
	LL_DEBUGS("Messaging") << "onCovenantLoadComplete()" << LL_ENDL;
	std::string covenant_text;
	if (0 == status)
	{
		LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
		S32 file_length = file.getSize();
		std::vector<char> buffer(file_length + 1);
		file.read((U8*)&buffer[0], file_length);
		buffer[file_length] = '\0';
		if ((file_length > 19) && !strncmp(&buffer[0], "Linden text version", 19))
		{
			LLViewerTextEditor * editor = new LLViewerTextEditor(std::string("temp"), LLRect(0,0,0,0), file_length+1);
			if( !editor->importBuffer( &buffer[0], file_length+1 ) )
			{
				LL_WARNS("Messaging") << "Problem importing estate covenant." << LL_ENDL;
				covenant_text = "Problem importing estate covenant.";
			}
			else
			{
				covenant_text = editor->getText();
			}
			delete editor;
		}
		else
		{
			LL_WARNS("Messaging") << "Problem importing estate covenant: Covenant file format error." << LL_ENDL;
			covenant_text = "Problem importing estate covenant: Covenant file format error.";
		}
	}
	else
	{
		LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		if (LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
		    LL_ERR_FILE_EMPTY == status)
		{
			covenant_text = "Estate covenant notecard is missing from database.";
		}
		else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
		{
			covenant_text = "Insufficient permissions to view estate covenant.";
		}
		else
		{
			covenant_text = "Unable to load estate covenant at this time.";
		}
		LL_WARNS("Messaging") << "Problem loading notecard: " << status << LL_ENDL;
	}
	LLPanelEstateCovenant::updateCovenantText(covenant_text, asset_uuid);
	LLPanelLandCovenant::updateCovenantText(covenant_text);
	LLFloaterBuyLand::updateCovenantText(covenant_text, asset_uuid);
}
void process_feature_disabled_message(LLMessageSystem* msg, void**)
{
	LLUUID	agentID;
	LLUUID	transactionID;
	std::string	messageText;
	msg->getStringFast(_PREHASH_FailureInfo, _PREHASH_ErrorMessage, messageText, 0);
	msg->getUUIDFast(_PREHASH_FailureInfo, _PREHASH_AgentID, agentID);
	msg->getUUIDFast(_PREHASH_FailureInfo, _PREHASH_TransactionID, transactionID);
	LL_WARNS("Messaging") << "Blacklisted Feature Response:" << messageText << LL_ENDL;
}
void invalid_message_callback(LLMessageSystem* msg,
								   void*,
								   EMessageException exception)
{
    LLAppViewer::instance()->badNetworkHandler();
}
void LLOfferInfo::forceResponse(InventoryOfferResponse response)
{
	LLNotification::Params params("UserGiveItem");
	params.functor(boost::bind(&LLOfferInfo::inventory_offer_callback, this, _1, _2));
	LLNotifications::instance().forceResponse(params, response);
}
