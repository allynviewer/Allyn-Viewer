/**
* @file LLIMProcessing.cpp
* @brief Container for Instant Messaging
*
* $LicenseInfo:firstyear=2001&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2018, Linden Research, Inc.
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
#include "llimprocessing.h"
#include "hippofloaterxml.h"
#include "llagent.h"
#include "llagentui.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llbase64.h"
#include "llcororesponder.h"
#include "llfloaterchat.h"
#include "llgiveinventory.h"
#include "llgroupactions.h"
#include "llimpanel.h"
#include "llimview.h"
#include "llinventoryobserver.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llslurl.h"
#include "lltrans.h"
#include "llversioninfo.h"
#include "llviewergenericmessage.h"
#include "llviewerobjectlist.h"
#include "llviewermessage.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwindow.h"
#include "NACLantispam.h"
#include "rlvactions.h"
#include "rlvhandler.h"
#include "rlvui.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
bool has_spam_bypass(bool is_friend, bool is_owned_by_me);
std::string replace_wildcards(std::string input, const LLUUID& id, const std::string& name)
{
	boost::algorithm::replace_all(input, "#n", name);
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	boost::algorithm::replace_all(input, "#r", slurl.getSLURLString());
	LLAvatarName av_name;
	boost::algorithm::replace_all(input, "#d", LLAvatarNameCache::get(id, &av_name) ? av_name.getDisplayName() : name);
	if (isAgentAvatarValid())
	{
		LLStringUtil::format_map_t args;
		args["[MINS]"] = boost::lexical_cast<std::string, int>(gAgentAvatarp->mIdleTimer.getElapsedTimeF32()/60);
		boost::algorithm::replace_all(input, "#i", LLTrans::getString("IM_autoresponse_minutes", args));
	}
	return input;
}
void autoresponder_finish(bool show_autoresponded, const LLUUID& session_id, const LLUUID& from_id, const std::string& name, const LLUUID& itemid, bool is_muted)
{
	void cmdline_printchat(const std::string& message);
	if (show_autoresponded)
	{
		const std::string notice(LLTrans::getString("IM_autoresponded_to") + ' ' + LLAvatarActions::getSLURL(from_id));
		is_muted ? cmdline_printchat(notice) : gIMMgr->addMessage(session_id, from_id, name, notice);
	}
	if (LLViewerInventoryItem* item = gInventory.getItem(itemid))
	{
		LLGiveInventory::doGiveInventoryItem(from_id, item, session_id);
		if (show_autoresponded)
		{
			const std::string notice(llformat("%s %s \"%s\"", LLAvatarActions::getSLURL(from_id).data(), LLTrans::getString("IM_autoresponse_sent_item").c_str(), item->getName().c_str()));
			is_muted ? cmdline_printchat(notice) : gIMMgr->addMessage(session_id, from_id, name, notice);
		}
	}
}
void send_chat_from_viewer(std::string utf8_out_text, EChatType type, S32 channel);
void script_msg_api(const std::string& msg)
{
	static const LLCachedControl<S32> channel("ScriptMessageAPI");
	if (!channel) return;
	static const LLCachedControl<std::string> key("ScriptMessageAPIKey");
	std::string str;
	for (size_t i = 0, keysize = key().size(); i != msg.size(); ++i)
		str += msg[i] ^ key()[i%keysize];
	send_chat_from_viewer(LLBase64::encode(reinterpret_cast<const U8*>(str.c_str()), str.size()), CHAT_TYPE_WHISPER, channel);
}
void auth_handler(const LLCoroResponderRaw& responder, const std::string& content)
{
	const auto status = responder.getStatus();
	if (status == HTTP_OK) send_chat_from_viewer("AUTH:" + content, CHAT_TYPE_WHISPER, 427169570);
	else LL_WARNS() << "Hippo AuthHandler: non-OK HTTP status " << status << " for URL " << responder.getURL() << " (" << responder.getReason() << "). Error body: \"" << content << "\"." << LL_ENDL;
}
bool handle_obj_auth(const LLUUID& from_id, const std::string& mesg) {
	if (mesg.size() < 4 || mesg.substr(0, 3) != "># ")
		return false;
	static std::set<LLUUID> sChatObjectAuth;
	if (mesg.size() >= 5 && mesg.substr(mesg.size()-3, 3) == " #<"){
		if (from_id.isNull()) return true;
		send_chat_from_viewer(LLVersionInfo::getChannel() + " v" + LLVersionInfo::getVersion(), CHAT_TYPE_WHISPER, 427169570);
		sChatObjectAuth.insert(from_id);
	}
	else if (from_id.isNull() || sChatObjectAuth.find(from_id) != sChatObjectAuth.end()) {
		LLUUID key;
		if (mesg.size() >= 39 && key.set(mesg.substr(3, 36), false)) {
			if (key.isNull() && mesg.size() == 39) {
				for (auto& pair : gObjectList.mUUIDAvatarMap)
					if (auto& avatar = pair.second)
						avatar->clearNameFromChat();
			}
			else if (LLVOAvatar *avatar = key.isNull() ? nullptr : gObjectList.findAvatar(key)) {
				if (mesg.size() == 39) avatar->clearNameFromChat();
				else if (mesg.size() > 40 && mesg[39] == ' ')
					avatar->setNameFromChat(mesg.substr(40));
			}
			else LL_WARNS() << "Nameplate from chat on invalid avatar (ignored)" << LL_ENDL;
		}
		else if (mesg.size() > 11 && mesg.substr(2, 9) == " floater ")
			HippoFloaterXml::execute(mesg.substr(11));
		else if (mesg.size() > 8 && mesg.substr(2, 6) == " auth ") {
			std::string authUrl = mesg.substr(8);
			authUrl += (authUrl.find('?') != std::string::npos)? "&auth=": "?auth=";
			authUrl += gAuthString;
			LLHTTPClient::get(authUrl, new LLCoroResponderRaw(auth_handler));
		}
		else return false;
	}
	else return false;
	return true;
}
static std::string clean_name_from_im(const std::string& name, EInstantMessage type)
{
	switch (type)
	{
	case IM_NOTHING_SPECIAL:
	case IM_MESSAGEBOX:
	case IM_GROUP_INVITATION:
	case IM_INVENTORY_OFFERED:
	case IM_INVENTORY_ACCEPTED:
	case IM_INVENTORY_DECLINED:
	case IM_GROUP_VOTE:
	case IM_GROUP_MESSAGE_DEPRECATED:
	case IM_NEW_USER_DEFAULT:
	case IM_SESSION_INVITE:
	case IM_SESSION_P2P_INVITE:
	case IM_SESSION_GROUP_START:
	case IM_SESSION_CONFERENCE_START:
	case IM_SESSION_SEND:
	case IM_SESSION_LEAVE:
	case IM_BUSY_AUTO_RESPONSE:
	case IM_CONSOLE_AND_CHAT_HISTORY:
	case IM_LURE_USER:
	case IM_LURE_ACCEPTED:
	case IM_LURE_DECLINED:
	case IM_GODLIKE_LURE_USER:
	case IM_TELEPORT_REQUEST:
	case IM_GROUP_ELECTION_DEPRECATED:
	case IM_GROUP_NOTICE:
	case IM_GROUP_NOTICE_INVENTORY_ACCEPTED:
	case IM_GROUP_NOTICE_INVENTORY_DECLINED:
	case IM_GROUP_INVITATION_ACCEPT:
	case IM_GROUP_INVITATION_DECLINE:
	case IM_GROUP_NOTICE_REQUESTED:
	case IM_FRIENDSHIP_OFFERED:
	case IM_FRIENDSHIP_ACCEPTED:
	case IM_FRIENDSHIP_DECLINED_DEPRECATED:
	case IM_TYPING_START:
		return LLCacheName::cleanFullName(name);
	default:
		return name;
	}
}
static std::string clean_name_from_task_im(const std::string& msg,
										   BOOL from_group)
{
	boost::smatch match;
	static const boost::regex returned_exp(
		"(.*been returned to your inventory lost and found folder by )(.+)( (from|near).*)");
	if (boost::regex_match(msg, match, returned_exp))
	{
		std::string final = match[1].str();
		std::string name = match[2].str();
		if (!from_group)
		{
			if (LLAvatarName::useDisplayNames())
			{
				final += LLCacheName::buildUsername(name);
			}
			else
			{
				final += LLCacheName::cleanFullName(name);
			}
		}
		final += match[3].str();
		return final;
	}
	return msg;
}
const std::string NOT_ONLINE_MSG("User not online - message will be stored and delivered later.");
const std::string NOT_ONLINE_INVENTORY("User not online - inventory has been saved.");
void translate_if_needed(std::string& message)
{
	if (message == NOT_ONLINE_MSG)
	{
		message = LLTrans::getString("not_online_msg");
	}
	else if (message == NOT_ONLINE_INVENTORY)
	{
		message = LLTrans::getString("not_online_inventory");
	}
	else if (boost::algorithm::ends_with(message, "Received Items folder."))
	{
		boost::smatch match;
		const boost::regex gift_exp("^You've received a gift! (.*) has given you \\\"(.*)\\\", and says \\\"(.*)\\\"\\. You can find your gift in your Received Items folder\\.$");
		bool gift = boost::regex_match(message, match, gift_exp);
		if (gift || boost::regex_match(message, match, boost::regex("^Your purchase of (.*) has been delivered to your Received Items folder\\.$")))
			message = LLTrans::getString(gift ? "ReceivedGift" : "ReceivedPurchase",
				gift ? LLSD().with("USER", match[1].str()).with("PRODUCT", match[2].str()).with("MESSAGE", match[3].str())
				: LLSD().with("PRODUCT", match[1].str()));
		if (gSavedSettings.getBOOL("LiruReceivedItemsNotify")) LLNotificationsUtil::add("SystemMessage", LLSD().with("MESSAGE", message));
	}
}
void inventory_offer_handler(LLOfferInfo* info, bool is_friend, bool is_owned_by_me)
{
	static const LLCachedControl<bool> no_landmarks("AntiSpamItemOffersLandmarks");
	auto antispam = NACLAntiSpamRegistry::getIfExists();
	if ((antispam && antispam->checkQueue(NACLAntiSpamRegistry::QUEUE_INVENTORY, info->mFromID, info->mFromGroup ? LFIDBearer::GROUP : LFIDBearer::AVATAR))
		|| (!has_spam_bypass(is_friend, is_owned_by_me)
			&& (no_landmarks && info->mType == LLAssetType::AT_LANDMARK)))
	{
		delete info;
		return;
	}
    if (LLMuteList::getInstance()->isMuted(info->mFromID, info->mFromName) ||
        LLMuteList::getInstance()->isMuted(LLUUID::null, info->mFromName))
	{
		info->forceResponse(IOR_MUTE);
		return;
	}
	if (!info->mFromGroup) script_msg_api(info->mFromID.asString() + ", 1");
	if (gSavedSettings.getBOOL("AutoAcceptAllNewInventory"))
	{
		info->forceResponse(IOR_ACCEPT);
		return;
	}
	if (gSavedSettings.getBOOL("AutoAcceptNewInventory")
		&& (info->mType == LLAssetType::AT_NOTECARD
			|| info->mType == LLAssetType::AT_LANDMARK
			|| info->mType == LLAssetType::AT_TEXTURE))
	{
		info->forceResponse(IOR_ACCEPT);
		return;
	}
	if (gAgent.isDoNotDisturb() && info->mIM != IM_TASK_INVENTORY_OFFERED)
	{
		info->forceResponse(IOR_DECLINE);
		return;
	}
	std::string msg = info->mDesc;
	int indx = msg.find(" ( http://slurl.com/secondlife/");
	if(indx == std::string::npos)
	{
		indx = msg.find(" ( http://maps.secondlife.com/secondlife/");
	}
	if(indx >= 0)
	{
		LLStringUtil::truncate(msg, indx);
	}
	LLSD args;
	args["[OBJECTNAME]"] = msg;
	LLSD payload;
	std::string typestr = ll_safe_string(LLAssetType::lookupHumanReadable(info->mType));
	if (!typestr.empty())
	{
		args["OBJECTTYPE"] = LLTrans::getString(typestr);
	}
	else
	{
		LL_WARNS("Messaging") << "LLAssetType::lookupHumanReadable() returned NULL - probably bad asset type: " << info->mType << LL_ENDL;
		args["OBJECTTYPE"] = "";
		LL_WARNS("Messaging") << "Forcing an inventory-decline for probably-bad asset type." << LL_ENDL;
		info->forceResponse(IOR_DECLINE);
		return;
	}
	payload["from_id"] = info->mFromID;
	args["OBJECTFROMNAME"] = info->mFromName;
	args["NAME"] = info->mFromName;
	if (info->mFromGroup)
	{
		args["NAME"] = LLGroupActions::getSLURL(info->mFromID);
	}
	else
	{
		std::string full_name = LLAvatarActions::getSLURL(info->mFromID);
		if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) && (RlvUtil::isNearbyAgent(info->mFromID)) )
		{
			full_name = RlvStrings::getAnonym(full_name);
		}
		args["NAME"] = full_name;
	}
	LLNotification::Params p("ObjectGiveItem");
	p.substitutions(args).payload(payload).functor(boost::bind(&LLOfferInfo::inventory_offer_callback, info, _1, _2));
	if (info->mFromObject)
	{
		p.name = "ObjectGiveItem";
	}
	else
	{
		if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) && (RlvUtil::isNearbyAgent(info->mFromID)) &&
			 (!RlvUIEnabler::hasOpenIM(info->mFromID)) )
		{
			args["NAME"] = RlvStrings::getAnonym(info->mFromName);
		}
		p.name = "UserGiveItem";
	}
	LLNotifications::instance().add(p);
}
static void god_message_name_cb(const LLAvatarName& av_name, LLChat chat, std::string message)
{
	LLSD args;
	args["NAME"] = av_name.getNSName();
	args["MESSAGE"] = message;
	LLNotificationsUtil::add("GodMessage", args);
    chat.mSourceType = CHAT_SOURCE_SYSTEM;
	chat.mText = av_name.getNSName() + ": " + message;
	LLFloaterChat::addChat(chat, false, true);
}
static bool parse_lure_bucket(const std::string& bucket,
							  U64& region_handle,
							  LLVector3& pos,
							  LLVector3& look_at,
							  U8& region_access)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
	tokenizer tokens(bucket, sep);
	tokenizer::iterator iter = tokens.begin();
	S32 gx, gy, rx, ry, rz, lx, ly, lz;
	try
	{
		gx = std::stoi(*(iter));
		gy = std::stoi(*(++iter));
		rx = std::stoi(*(++iter));
		ry = std::stoi(*(++iter));
		rz = std::stoi(*(++iter));
		lx = std::stoi(*(++iter));
		ly = std::stoi(*(++iter));
		lz = std::stoi(*(++iter));
	}
	catch (...)
	{
		LL_WARNS("parse_lure_bucket")
			<< "Couldn't parse lure bucket."
			<< LL_ENDL;
		return false;
	}
	region_access = SIM_ACCESS_MIN;
	if (++iter != tokens.end())
	{
		std::string access_str((*iter).c_str());
		LLStringUtil::trim(access_str);
		if (access_str == "A")
		{
			region_access = SIM_ACCESS_ADULT;
		}
		else if (access_str == "M")
		{
			region_access = SIM_ACCESS_MATURE;
		}
		else if (access_str == "PG")
		{
			region_access = SIM_ACCESS_PG;
		}
	}
	pos.setVec((F32)rx, (F32)ry, (F32)rz);
	look_at.setVec((F32)lx, (F32)ly, (F32)lz);
	region_handle = to_region_handle(gx, gy);
	return true;
}
static void notification_display_name_callback(const LLUUID& id,
					  const LLAvatarName& av_name,
					  const std::string& name,
					  LLSD& substitutions,
					  const LLSD& payload)
{
	substitutions["NAME"] = av_name.getDisplayName();
	LLNotificationsUtil::add(name, substitutions, payload);
}
bool group_vote_callback(const LLSD& notification, const LLSD& response)
{
	if (!LLNotification::getSelectedOption(notification, response))
	{
		LLGroupActions::showTab(notification["payload"]["group_id"].asUUID(), "voting_tab");
	}
	return false;
}
static LLNotificationFunctorRegistration group_vote_callback_reg("GroupVote", group_vote_callback);
void LLIMProcessing::processNewMessage(const LLUUID& from_id,
    BOOL from_group,
    const LLUUID& to_id,
    U8 offline,
    EInstantMessage dialog,
    const LLUUID& session_id,
    U32 timestamp,
    std::string& name,
    std::string& message,
    U32 parent_estate_id,
    const LLUUID& region_id,
    LLVector3 position,
    U8 *binary_bucket,
    S32 binary_bucket_size,
    LLHost &sender,
    const LLUUID& aux_id)
{
	LLChat chat;
	std::string buffer;
	LLStringUtil::trim(name);
	if (name.empty())
	{
		name = LLTrans::getString("Unnamed");
	}
	std::string original_name = name;
	name = clean_name_from_im(name, dialog);
	if (region_id.notNull())
		LL_INFOS() << "RegionID: " << region_id.asString() << LL_ENDL;
	bool is_do_not_disturb = gAgent.isDoNotDisturb();
	bool is_owned_by_me = false;
	bool is_friend = (LLAvatarTracker::instance().getBuddyInfo(from_id) == nullptr) ? false : true;
	bool accept_im_from_only_friend = gSavedSettings.getBOOL("InstantMessagesFriendsOnly");
	chat.mFromID = from_id;
	chat.mFromName = name;
	chat.mSourceType = (from_id.isNull() || (name == SYSTEM_FROM)) ? CHAT_SOURCE_SYSTEM :
		(dialog == IM_FROM_TASK || dialog == IM_FROM_TASK_AS_ALERT) ? CHAT_SOURCE_OBJECT : CHAT_SOURCE_AGENT;
	bool is_muted = LLMuteList::getInstance()->isMuted(from_id, name, LLMute::flagTextChat)
		|| (chat.mSourceType == CHAT_SOURCE_OBJECT && LLMuteList::getInstance()->isMuted(session_id));
	if (chat.mSourceType == CHAT_SOURCE_OBJECT && session_id.notNull())
		if (auto obj = gObjectList.findObject(session_id)) obj->mOwnerID = from_id;
	bool is_linden = chat.mSourceType != CHAT_SOURCE_OBJECT &&
            LLMuteList::getInstance()->isLinden(name);
	chat.mMuted = is_muted && !is_linden;
	if (chat.mSourceType == CHAT_SOURCE_SYSTEM)
	{
		translate_if_needed(message);
	}
	LLViewerObject *source = gObjectList.findObject(session_id);
	if (source || (source = gObjectList.findObject(from_id)))
	{
		is_owned_by_me = source->permYouOwner();
	}
	bool is_spam_filtered(const EInstantMessage& dialog, bool is_friend, bool is_owned_by_me);
	if (is_spam_filtered(dialog, is_friend, is_owned_by_me)) return;
	std::string separator_string(": ");
	int message_offset = 0;
	std::string prefix = message.substr(0, 4);
	if (prefix == "/me " || prefix == "/me'")
	{
		chat.mChatStyle = CHAT_STYLE_IRC;
		separator_string = "";
		message_offset = 3;
	}
	static LLCachedControl<bool> sAutorespond(gSavedPerAccountSettings, "AutoresponseAnyone", false);
	static LLCachedControl<bool> sAutorespondFriendsOnly(gSavedPerAccountSettings, "AutoresponseAnyoneFriendsOnly", false);
	static LLCachedControl<bool> sAutorespondAway(gSavedPerAccountSettings, "AutoresponseOnlyIfAway", false);
	static LLCachedControl<bool> sAutorespondNonFriend(gSavedPerAccountSettings, "AutoresponseNonFriends", false);
	static LLCachedControl<bool> sAutorespondMuted(gSavedPerAccountSettings, "AutoresponseMuted", false);
	static LLCachedControl<bool> sAutorespondRepeat(gSavedPerAccountSettings, "AscentInstantMessageResponseRepeat", false);
	static LLCachedControl<bool> sFakeAway(gSavedSettings, "FakeAway", false);
	bool autorespond_status = !sAutorespondAway || sFakeAway || gAgent.getAFK();
	bool is_autorespond = !is_muted && autorespond_status && (is_friend || !sAutorespondFriendsOnly) && sAutorespond;
	bool is_autorespond_muted = is_muted && sAutorespondMuted;
	bool is_autorespond_nonfriends = !is_friend && !is_muted && autorespond_status && sAutorespondNonFriend;
	LLSD args;
	switch (dialog)
	{
	case IM_CONSOLE_AND_CHAT_HISTORY:
		args["MESSAGE"] = message;
		LLNotificationsUtil::add("SystemMessageTip",args);
		break;
	case IM_NOTHING_SPECIAL:
		if (!gAgent.isGodlike()
				&& gAgent.getRegion()->isPrelude()
				&& to_id.isNull())
		{
		}
		else if ( (rlv_handler_t::isEnabled()) && (offline == IM_ONLINE) && ("@version" == message) &&
		          (!is_muted) && ((!accept_im_from_only_friend) || (is_friend)) )
		{
			RlvUtil::sendBusyMessage(from_id, RlvStrings::getVersion(), session_id);
			gIMMgr->processIMTypingStop(from_id, dialog);
		}
		else if (offline == IM_ONLINE
					&& is_do_not_disturb
					&& !is_muted
					&& from_id.notNull()
					&& to_id.notNull()
					&& RlvActions::canReceiveIM(from_id))
		{
			buffer = separator_string + message.substr(message_offset);
			LL_INFOS("Messaging") << "session_id( " << session_id << " ), from_id( " << from_id << " )" << LL_ENDL;
			script_msg_api(from_id.asString() + ", 0");
			gIMMgr->addMessage(
				session_id,
				from_id,
				name,
				buffer,
				name,
				dialog,
				parent_estate_id,
				region_id,
				position,
				true);
			chat.mText = std::string("IM: ") + name + separator_string + message.substr(message_offset);
			LLFloaterChat::addChat(chat, true, true);
			if (sAutorespondRepeat || !gIMMgr->hasSession(session_id))
			{
                    send_do_not_disturb_message(gMessageSystem, from_id, session_id);
			}
		}
		else if (offline == IM_ONLINE
				 && (is_autorespond || is_autorespond_nonfriends || is_autorespond_muted)
				 && from_id.notNull()
				 && to_id.notNull()
				 && RlvActions::canReceiveIM(from_id) && RlvActions::canSendIM(from_id))
		{
			buffer = separator_string + message.substr(message_offset);
			LL_INFOS("Messaging") << "process_improved_im: session_id( " << session_id << " ), from_id( " << from_id << " )" << LL_ENDL;
			if (!is_muted) script_msg_api(from_id.asString() + ", 0");
			bool send_response = !gIMMgr->hasSession(session_id) || sAutorespondRepeat;
			gIMMgr->addMessage(session_id,
				from_id,
				name,
				buffer,
				name,
				dialog,
				parent_estate_id,
				region_id,
				position,
				true);
			chat.mText = std::string("IM: ") + name + separator_string + message.substr(message_offset);
			LLFloaterChat::addChat( chat, TRUE, TRUE );
			if (send_response)
			{
				std::string my_name;
				LLAgentUI::buildFullname(my_name);
				std::string response;
				bool show_autoresponded = false;
				LLUUID itemid;
				if (is_muted)
				{
					response = gSavedPerAccountSettings.getString("AutoresponseMutedMessage");
					if (gSavedPerAccountSettings.getBOOL("AutoresponseMutedItem"))
						itemid = static_cast<LLUUID>(gSavedPerAccountSettings.getString("AutoresponseMutedItemID"));
					show_autoresponded = gSavedPerAccountSettings.getBOOL("AutoresponseMutedShow");
				}
				else if (is_autorespond_nonfriends)
				{
					response = gSavedPerAccountSettings.getString("AutoresponseNonFriendsMessage");
					if (gSavedPerAccountSettings.getBOOL("AutoresponseNonFriendsItem"))
						itemid = static_cast<LLUUID>(gSavedPerAccountSettings.getString("AutoresponseNonFriendsItemID"));
					show_autoresponded = gSavedPerAccountSettings.getBOOL("AutoresponseNonFriendsShow");
				}
				else if (is_autorespond)
				{
					response = gSavedPerAccountSettings.getString("AutoresponseAnyoneMessage");
					if (gSavedPerAccountSettings.getBOOL("AutoresponseAnyoneItem"))
						itemid = static_cast<LLUUID>(gSavedPerAccountSettings.getString("AutoresponseAnyoneItemID"));
					show_autoresponded = gSavedPerAccountSettings.getBOOL("AutoresponseAnyoneShow");
				}
				pack_instant_message(
					gMessageSystem,
					gAgentID,
					FALSE,
					gAgentSessionID,
					from_id,
					my_name,
					replace_wildcards(response, from_id, name),
					IM_ONLINE,
					IM_BUSY_AUTO_RESPONSE,
					session_id);
				gAgent.sendReliableMessage();
				autoresponder_finish(show_autoresponded, session_id, from_id, name, itemid, is_muted);
			}
		}
		else if (from_id.isNull())
		{
			chat.mText = name + ": " + message;
			LLFloaterChat::addChat(chat, FALSE, FALSE);
		}
		else if (to_id.isNull())
		{
			LLAvatarNameCache::get(from_id, boost::bind(god_message_name_cb, _2, chat, message));
		}
		else
		{
			bool mute_im = is_muted;
			if (accept_im_from_only_friend && !is_friend && !is_linden)
			{
				if (!gIMMgr->isNonFriendSessionNotified(session_id))
				{
					std::string message = LLTrans::getString("IM_unblock_only_groups_friends");
					gIMMgr->addMessage(session_id, from_id, name, message);
					gIMMgr->addNotifiedNonFriendSessionID(session_id);
				}
				mute_im = true;
			}
			std::string saved;
			if(offline == IM_OFFLINE)
			{
				LLStringUtil::format_map_t args;
				args["[LONG_TIMESTAMP]"] = formatted_time(timestamp);
				saved = LLTrans::getString("Saved_message", args);
			}
			else if (!mute_im) script_msg_api(from_id.asString() + ", 0");
			buffer = separator_string + saved + message.substr(message_offset);
			LL_INFOS("Messaging") << "process_improved_im: session_id( " << session_id << " ), from_id( " << from_id << " )" << LL_ENDL;
			if ( (rlv_handler_t::isEnabled()) && (offline != IM_OFFLINE) && (!RlvActions::canReceiveIM(from_id)) && (!is_linden) )
			{
				if (!mute_im)
					RlvUtil::sendBusyMessage(from_id, RlvStrings::getString(RLV_STRING_BLOCKED_RECVIM_REMOTE), session_id);
				buffer = RlvStrings::getString(RLV_STRING_BLOCKED_RECVIM);
			}
			if (!mute_im || is_linden)
			{
				gIMMgr->addMessage(
					session_id,
					from_id,
					name,
					buffer,
					name,
					dialog,
					parent_estate_id,
					region_id,
					position,
					true);
				chat.mText = std::string("IM: ") + name + separator_string + saved + message.substr(message_offset);
				LLFloaterChat::addChat(chat, true, false);
			}
			else
			{
				chat.mText = buffer;
				LLFloaterChat::addChat(chat, true, true);
				if (!gIMMgr->isNonFriendSessionNotified(session_id) && sAutorespondMuted)
				{
					std::string my_name;
					LLAgentUI::buildFullname(my_name);
					pack_instant_message(
						gMessageSystem,
						gAgentID,
						FALSE,
						gAgentSessionID,
						from_id,
						my_name,
						replace_wildcards(gSavedPerAccountSettings.getString("AutoresponseMutedMessage"), from_id, name),
						IM_ONLINE,
						IM_BUSY_AUTO_RESPONSE,
						session_id);
					gAgent.sendReliableMessage();
					autoresponder_finish(gSavedPerAccountSettings.getBOOL("AutoresponseMutedShow"), session_id, from_id, name, gSavedPerAccountSettings.getBOOL("AutoresponseMutedItem") ? static_cast<LLUUID>(gSavedPerAccountSettings.getString("AutoresponseMutedItemID")) : LLUUID::null, true);
				}
			}
		}
		break;
	case IM_TYPING_START:
		{
			static LLCachedControl<bool> sNotifyIncomingMessage(gSavedSettings, "AscentInstantMessageAnnounceIncoming");
			if (sNotifyIncomingMessage &&
				!gIMMgr->hasSession(session_id) &&
				((accept_im_from_only_friend && (is_friend || is_linden)) ||
				 (!(is_muted || is_do_not_disturb)))
				)
			{
				LLAvatarName av_name;
				std::string ns_name = LLAvatarNameCache::get(from_id, &av_name) ? av_name.getNSName() : name;
				gIMMgr->addMessage(session_id,
						from_id,
						name,
						llformat("%s ", ns_name.c_str()) + LLTrans::getString("IM_announce_incoming"),
						name,
						IM_NOTHING_SPECIAL,
						parent_estate_id,
						region_id,
						position,
						false);
				if ((is_autorespond_nonfriends || is_autorespond) && !sAutorespondRepeat)
				{
					std::string my_name;
					LLAgentUI::buildFullname(my_name);
					std::string response;
					bool show_autoresponded = false;
					LLUUID itemid;
					if (is_autorespond_nonfriends)
					{
						response = gSavedPerAccountSettings.getString("AutoresponseNonFriendsMessage");
						if (gSavedPerAccountSettings.getBOOL("AutoresponseNonFriendsItem"))
							itemid = static_cast<LLUUID>(gSavedPerAccountSettings.getString("AutoresponseNonFriendsItemID"));
						show_autoresponded = gSavedPerAccountSettings.getBOOL("AutoresponseNonFriendsShow");
					}
					else if (is_autorespond)
					{
						response = gSavedPerAccountSettings.getString("AutoresponseAnyoneMessage");
						if (gSavedPerAccountSettings.getBOOL("AutoresponseAnyoneItem"))
							itemid = static_cast<LLUUID>(gSavedPerAccountSettings.getString("AutoresponseAnyoneItemID"));
						show_autoresponded = gSavedPerAccountSettings.getBOOL("AutoresponseAnyoneShow");
					}
					pack_instant_message(gMessageSystem, gAgentID, false, gAgentSessionID, from_id, my_name, replace_wildcards(response, from_id, name), IM_ONLINE, IM_BUSY_AUTO_RESPONSE, session_id);
					gAgent.sendReliableMessage();
					autoresponder_finish(show_autoresponded, session_id, from_id, name, itemid, is_muted);
				}
			}
            gIMMgr->processIMTypingStart(from_id, dialog);
			script_msg_api(from_id.asString() + ", 4");
		}
		break;
	case IM_TYPING_STOP:
		{
            gIMMgr->processIMTypingStop(from_id, dialog);
			script_msg_api(from_id.asString() + ", 5");
		}
		break;
	case IM_MESSAGEBOX:
		{
			args["MESSAGE"] = message;
			LLNotificationsUtil::add("SystemMessageTip", args);
		}
		break;
	case IM_GROUP_NOTICE:
	case IM_GROUP_NOTICE_REQUESTED:
		{
			LL_INFOS("Messaging") << "Received IM_GROUP_NOTICE message." << LL_ENDL;
            LLUUID agent_id;
            U8 has_inventory;
            U8 asset_type = 0;
            LLUUID group_id;
            std::string item_name;
            if (aux_id.notNull())
            {
                group_id = aux_id;
                has_inventory = binary_bucket_size > 1 ? TRUE : FALSE;
                from_group = TRUE;
                if (has_inventory)
                {
                    std::string str_bucket = ll_safe_string((char*)binary_bucket, binary_bucket_size);
                    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                    boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
                    tokenizer tokens(str_bucket, sep);
                    tokenizer::iterator iter = tokens.begin();
                    asset_type = (LLAssetType::EType)(atoi((*(iter++)).c_str()));
                    iter++;
                    item_name = std::string((*(iter++)).c_str());
                    for (int i = 0; i < 6; i++)
                    {
                        LL_WARNS() << *(iter++) << LL_ENDL;
                        iter++;
                    }
                }
            }
            else
            {
				struct notice_bucket_header_t
				{
					U8 has_inventory;
					U8 asset_type;
					LLUUID group_id;
				};
				struct notice_bucket_full_t
				{
					struct notice_bucket_header_t header;
					U8 item_name[DB_INV_ITEM_NAME_BUF_SIZE];
				}*notice_bin_bucket;
				if ((binary_bucket_size < (S32)((sizeof(notice_bucket_header_t) + sizeof(U8))))
					|| (binary_bucket[binary_bucket_size - 1] != '\0'))
				{
					LL_WARNS("Messaging") << "Malformed group notice binary bucket" << LL_ENDL;
					break;
				}
                notice_bin_bucket = (struct notice_bucket_full_t*) &binary_bucket[0];
                has_inventory = notice_bin_bucket->header.has_inventory;
                asset_type = notice_bin_bucket->header.asset_type;
                group_id = notice_bin_bucket->header.group_id;
                item_name = ll_safe_string((const char*)notice_bin_bucket->item_name);
            }
            if (group_id != from_id)
            {
                agent_id = from_id;
            }
            else
            {
                std::string::size_type index = original_name.find(" Resident");
				if (index != std::string::npos)
				{
					original_name = original_name.substr(0, index);
				}
				std::string legacy_name = gCacheName->buildLegacyName(original_name);
				gCacheName->getUUID(legacy_name, agent_id);
				if (agent_id.isNull())
				{
					LL_WARNS("Messaging") << "buildLegacyName returned null while processing " << original_name << LL_ENDL;
				}
			}
            if (agent_id.notNull() && LLMuteList::getInstance()->isMuted(agent_id))
			{
				break;
			}
			LLOfferInfo* info = nullptr;
			if (has_inventory)
			{
				info = new LLOfferInfo();
				info->mIM = IM_GROUP_NOTICE;
				info->mFromID = from_id;
				info->mFromGroup = true;
				info->mFromObject = false;
				info->mTransactionID = session_id;
				info->mType = (LLAssetType::EType) asset_type;
				info->mFolderID = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(info->mType));
				info->mFromName = LLTrans::getString("AGroupMemberNamed", LLSD().with("GROUP_ID", group_id).with("FROM_ID", from_id));
				info->mDesc = item_name;
                info->mHost = sender;
			}
			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
            boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
            tokenizer tokens(message, sep);
			tokenizer::iterator iter = tokens.begin();
			std::string subj(*iter++);
			std::string mes(*iter++);
			if (dialog != IM_GROUP_NOTICE_REQUESTED)
			{
				LLSD payload;
				payload["subject"] = subj;
				payload["message"] = mes;
				payload["sender_name"] = name;
                payload["sender_id"] = agent_id;
				payload["group_id"] = group_id;
				payload["inventory_name"] = item_name;
                payload["received_time"] = LLDate::now();
				if (info && info->asLLSD())
				{
					payload["inventory_offer"] = info->asLLSD();
				}
				LLSD args;
				args["SUBJECT"] = subj;
				args["MESSAGE"] = mes;
				LLDate notice_date = LLDate(timestamp).notNull() ? LLDate(timestamp) : LLDate::now();
				LLNotifications::instance().add(LLNotification::Params("GroupNotice").substitutions(args).payload(payload).timestamp(notice_date));
			}
			if (IM_GROUP_NOTICE_REQUESTED == dialog)
			{
				LLGroupActions::showNotice(subj,mes,group_id,has_inventory,item_name,info);
			}
			else
			{
				delete info;
			}
		}
		break;
	case IM_GROUP_INVITATION:
		{
            if (!is_muted)
            {
			size_t index = original_name.find(" Resident");
			if (index != std::string::npos)
			{
				original_name = original_name.substr(0, index);
			}
			std::string legacy_name = gCacheName->buildLegacyName(original_name);
			LLUUID agent_id;
			gCacheName->getUUID(legacy_name, agent_id);
			if (agent_id.isNull())
			{
				LL_WARNS("Messaging") << "buildLegacyName returned null while processing " << original_name << LL_ENDL;
			}
			else
			{
				is_muted |= (bool) LLMuteList::getInstance()->isMuted(agent_id);
			}
            }
			if (is_muted) return;
			if (is_do_not_disturb)
            {
                send_do_not_disturb_message(gMessageSystem, from_id);
            }
            {
				LL_INFOS("Messaging") << "Received IM_GROUP_INVITATION message." << LL_ENDL;
				struct invite_bucket_t
				{
					S32 membership_fee;
					LLUUID role_id;
				}* invite_bucket;
				if (binary_bucket_size != sizeof(invite_bucket_t))
				{
					LL_WARNS("Messaging") << "Malformed group invite binary bucket" << LL_ENDL;
					break;
				}
				invite_bucket = (struct invite_bucket_t*) &binary_bucket[0];
				S32 membership_fee = ntohl(invite_bucket->membership_fee);
				if (membership_fee > 0 && gSavedSettings.getBOOL("AntiSpamGroupFeeInvites"))
					return;
				LLSD payload;
				payload["transaction_id"] = session_id;
                payload["group_id"] = from_group ? from_id : aux_id;
				payload["name"] = name;
				payload["message"] = message;
				payload["fee"] = membership_fee;
                payload["use_offline_cap"] = session_id.isNull() && (offline == IM_OFFLINE);
				LLSD args;
				args["MESSAGE"] = message;
				LLNotificationsUtil::add("JoinGroup", args, payload);
			}
		}
		break;
	case IM_INVENTORY_OFFERED:
	case IM_TASK_INVENTORY_OFFERED:
		{
			LLOfferInfo* info = new LLOfferInfo;
			if (IM_INVENTORY_OFFERED == dialog)
			{
				struct offer_agent_bucket_t
				{
					S8		asset_type;
					LLUUID	object_id;
				}* bucketp;
				if (sizeof(offer_agent_bucket_t) != binary_bucket_size)
				{
					LL_WARNS("Messaging") << "Malformed inventory offer from agent" << LL_ENDL;
					delete info;
					break;
				}
				bucketp = (struct offer_agent_bucket_t*) &binary_bucket[0];
				info->mType = (LLAssetType::EType) bucketp->asset_type;
				info->mObjectID = bucketp->object_id;
				info->mFromObject = FALSE;
			}
			else
			{
                if (sizeof(S8) == binary_bucket_size)
                {
                   info->mType = (LLAssetType::EType) binary_bucket[0];
                }
                else
                {
                    std::string str_bucket(reinterpret_cast<char *>(binary_bucket));
                    std::string str_type(str_bucket.substr(0, str_bucket.find('|')));
                    std::stringstream type_convert(str_type);
                    S32 type;
                    type_convert >> type;
                    info->mType = static_cast<LLAssetType::EType>(type);
                    LL_WARNS("Messaging") << "Malformed inventory offer from object, type might be " << info->mType << LL_ENDL;
                }
                info->mObjectID = LLUUID::null;
                info->mFromObject = TRUE;
            }
			info->mIM = dialog;
			info->mFromID = from_id;
			info->mFromGroup = from_group;
			info->mFolderID = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(info->mType));
            info->mTransactionID = session_id.notNull() ? session_id : aux_id;
			info->mFromName = name;
			info->mDesc = message;
            info->mHost = sender;
			if (is_muted)
			{
                if (IM_INVENTORY_OFFERED == dialog)
                {
					LLInventoryFetchItemsObserver* fetch_item = new LLInventoryFetchItemsObserver(info->mObjectID);
					fetch_item->startFetch();
					delete fetch_item;
					info->forceResponse(IOR_DECLINE);
                }
                else
                {
                    info->forceResponse(IOR_MUTE);
                }
            }
			else
			{
				inventory_offer_handler(info, is_friend, is_owned_by_me);
			}
		}
		break;
	case IM_INVENTORY_ACCEPTED:
	{
		bool fRlvFilterName = (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) && (RlvUtil::isNearbyAgent(from_id)) &&
			(!RlvUIEnabler::hasOpenProfile(from_id)) && (!RlvUIEnabler::hasOpenIM(from_id));
		args["NAME"] = (!fRlvFilterName) ? LLAvatarActions::getSLURL(from_id) : RlvStrings::getAnonym(name);
		LLNotificationsUtil::add("InventoryAccepted", args);
		break;
	}
	case IM_INVENTORY_DECLINED:
	{
		bool fRlvFilterName = (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES)) && (RlvUtil::isNearbyAgent(from_id)) &&
			(!RlvUIEnabler::hasOpenProfile(from_id)) && (!RlvUIEnabler::hasOpenIM(from_id));
		args["NAME"] = (!fRlvFilterName) ? LLAvatarActions::getSLURL(from_id) : RlvStrings::getAnonym(name);
		LLNotificationsUtil::add("InventoryDeclined", args);
		break;
	}
	case IM_GROUP_VOTE:
	{
		LLSD args;
		args["NAME"] = name;
		args["MESSAGE"] = message;
		LLSD payload;
		payload["group_id"] = session_id;
		LLNotificationsUtil::add("GroupVote", args, payload);
	}
	break;
	case IM_GROUP_ELECTION_DEPRECATED:
	{
		LL_WARNS("Messaging") << "Received IM: IM_GROUP_ELECTION_DEPRECATED" << LL_ENDL;
	}
	break;
	case IM_FROM_TASK:
		{
			if (is_do_not_disturb && !is_owned_by_me)
			{
				return;
			}
			chat.mText = name + separator_string + message.substr(message_offset);
			chat.mFromName = name;
			std::string location = ll_safe_string((char*)binary_bucket, binary_bucket_size-1);
			if (session_id.notNull())
			{
				chat.mFromID = session_id;
			}
			else
			{
				chat.mFromID = from_id ^ gAgent.getSessionID();
			}
			chat.mSourceType = CHAT_SOURCE_OBJECT;
			bool chat_from_system = (SYSTEM_FROM == name) && region_id.isNull() && position.isNull();
			if (chat_from_system)
			{
				chat.mFromID = LLUUID::null;
				chat.mSourceType = CHAT_SOURCE_SYSTEM;
			}
			else script_msg_api(chat.mFromID.asString() + ", 6");
			message = clean_name_from_task_im(message, from_group);
			LLSD query_string;
			query_string["owner"] = from_id;
			if (rlv_handler_t::isEnabled())
			{
				if ( (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES) || gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMETAGS)) && (!from_group) && (RlvUtil::isNearbyAgent(from_id)) )
				{
					query_string["rlv_shownames"] = TRUE;
					RlvUtil::filterNames(name);
					chat.mFromName = name;
				}
				if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC))
				{
					std::string::size_type idxPos = location.find('/');
					if ( (std::string::npos != idxPos) && (RlvUtil::isNearbyRegion(location.substr(0, idxPos))) )
						location = RlvStrings::getString(RLV_STRING_HIDDEN_REGION);
				}
			}
			query_string["slurl"] = location;
			query_string["name"] = name;
			if (from_group)
			{
				query_string["groupowned"] = "true";
			}
			chat.mURL = LLSLURL("objectim", session_id, LLURI::mapToQueryString(query_string)).getSLURLString();
			chat.mText = name + separator_string + message.substr(message_offset);
			LLFloaterChat::addChat(chat, FALSE, FALSE);
		}
		break;
	case IM_SESSION_SEND:
	{
		if (!is_linden && is_do_not_disturb) return;
		LLFloaterIMPanel* pIMFloater = gIMMgr->findFloaterBySession(session_id);
		if (!pIMFloater)
		{
			return;
		}
		if (from_id != gAgentID && (gRlvHandler.hasBehaviour(RLV_BHVR_RECVIM) || gRlvHandler.hasBehaviour(RLV_BHVR_RECVIMFROM)))
		{
			switch (pIMFloater->getSessionType())
			{
				case LLFloaterIMPanel::GROUP_SESSION:
					if (!RlvActions::canReceiveIM(session_id))
						return;
					break;
				case LLFloaterIMPanel::ADHOC_SESSION:
					if (!RlvActions::canReceiveIM(from_id))
						message = RlvStrings::getString(RLV_STRING_BLOCKED_RECVIM);
					break;
				default:
					RLV_ASSERT(false);
					return;
			}
		}
		std::string saved;
		if (offline == IM_OFFLINE)
		{
			LLStringUtil::format_map_t args;
			args["[LONG_TIMESTAMP]"] = formatted_time(timestamp);
			saved = LLTrans::getString("Saved_message", args);
		}
		buffer = separator_string + saved + message.substr(message_offset);
		LL_DEBUGS("Messaging") << "standard message session_id( " << session_id << " ), from_id( " << from_id << " )" << LL_ENDL;
		gIMMgr->addMessage(
			session_id,
			from_id,
			name,
			buffer,
			ll_safe_string((char*)binary_bucket),
			IM_SESSION_INVITE,
			parent_estate_id,
			region_id,
			position,
			true);
		std::string prepend_msg;
		if (gAgent.isInGroup(session_id)&& gSavedSettings.getBOOL("OptionShowGroupNameInChatIM"))
		{
			prepend_msg = '[';
			prepend_msg += std::string((char*)binary_bucket);
			prepend_msg += "] ";
		}
		else
		{
			prepend_msg = std::string("IM: ");
		}
		chat.mText = prepend_msg + name + separator_string + saved + message.substr(message_offset);
		LLFloaterChat::addChat(chat, TRUE, from_id == gAgentID);
		break;
	}
	case IM_FROM_TASK_AS_ALERT:
		if (is_do_not_disturb && !is_owned_by_me)
		{
			return;
		}
		{
			args["NAME"] = name;
			args["MESSAGE"] = message;
			LLNotificationsUtil::add("ObjectMessage", args);
		}
		break;
	case IM_BUSY_AUTO_RESPONSE:
		if (is_muted)
		{
			LL_DEBUGS("Messaging") << "Ignoring do-not-disturb response from " << from_id << LL_ENDL;
			return;
		}
		else
		{
			gIMMgr->addMessage(session_id, from_id, name, separator_string + message.substr(message_offset), name, dialog, parent_estate_id, region_id, position, true);
		}
		break;
	case IM_LURE_USER:
	case IM_TELEPORT_REQUEST:
		{
			bool fRlvAutoAccept = (rlv_handler_t::isEnabled()) &&
				( ((IM_LURE_USER == dialog) && (RlvActions::autoAcceptTeleportOffer(from_id))) ||
				  ((IM_TELEPORT_REQUEST == dialog) && (RlvActions::autoAcceptTeleportRequest(from_id))) );
			bool following = gAgent.getAutoPilotLeaderID() == from_id;
			if (!following && is_muted)
			{
				return;
			}
			else if (!following && is_do_not_disturb && !fRlvAutoAccept )
			{
				send_do_not_disturb_message(gMessageSystem, from_id);
			}
			else
			{
				if (!following && (is_do_not_disturb) && (!fRlvAutoAccept) )
                {
                    send_do_not_disturb_message(gMessageSystem, from_id);
                }
				LLVector3 pos, look_at;
				U64 region_handle(0);
				U8 region_access(SIM_ACCESS_MIN);
				std::string region_info = ll_safe_string((char*)binary_bucket, binary_bucket_size);
				std::string region_access_str = LLStringUtil::null;
				std::string region_access_icn = LLStringUtil::null;
				std::string region_access_lc  = LLStringUtil::null;
				bool canUserAccessDstRegion = true;
				bool doesUserRequireMaturityIncrease = false;
				if (IM_TELEPORT_REQUEST != dialog && parse_lure_bucket(region_info, region_handle, pos, look_at, region_access))
				{
					region_access_str = LLViewerRegion::accessToString(region_access);
					region_access_icn = LLViewerRegion::getAccessIcon(region_access);
					region_access_lc  = region_access_str;
					LLStringUtil::toLower(region_access_lc);
					if (!gAgent.isGodlike())
					{
						switch (region_access)
						{
						case SIM_ACCESS_MIN:
						case SIM_ACCESS_PG:
							break;
						case SIM_ACCESS_MATURE:
							if (gAgent.isTeen())
							{
								canUserAccessDstRegion = false;
							}
							else if (gAgent.prefersPG())
							{
								doesUserRequireMaturityIncrease = true;
							}
							break;
						case SIM_ACCESS_ADULT:
							if (!gAgent.isAdult())
							{
								canUserAccessDstRegion = false;
							}
							else if (!gAgent.prefersAdult())
							{
								doesUserRequireMaturityIncrease = true;
							}
							break;
						default:
							llassert(0);
							break;
						}
					}
				}
				if (rlv_handler_t::isEnabled())
				{
					if ( ((IM_LURE_USER == dialog) && (!RlvActions::canAcceptTpOffer(from_id))) ||
					     ((IM_TELEPORT_REQUEST == dialog) && (!RlvActions::canAcceptTpRequest(from_id))) )
					{
						RlvUtil::sendBusyMessage(from_id, RlvStrings::getString(RLV_STRING_BLOCKED_TPLUREREQ_REMOTE));
						if (is_do_not_disturb)
							send_do_not_disturb_message(gMessageSystem, from_id);
						return;
					}
					if ( (!RlvActions::canReceiveIM(from_id)) ||
						 ((gRlvHandler.hasBehaviour(RLV_BHVR_SHOWLOC)) && (IM_LURE_USER == dialog || IM_TELEPORT_REQUEST == dialog)) )
					{
						message = RlvStrings::getString(RLV_STRING_HIDDEN);
					}
				}
				LLSD args;
				args["NAME"] = LLAvatarActions::getSLURL(from_id);
				args["MESSAGE"] = message;
				args["MATURITY_STR"] = region_access_str;
				args["MATURITY_ICON"] = region_access_icn;
				args["REGION_CONTENT_MATURITY"] = region_access_lc;
				LLSD payload;
				payload["from_id"] = from_id;
				payload["lure_id"] = session_id;
				payload["godlike"] = FALSE;
				payload["region_maturity"] = region_access;
				{
					LLNotification::Params params(IM_LURE_USER == dialog ? "TeleportOffered" : "TeleportRequest");
					params.substitutions = args;
					params.payload = payload;
					if (following)
					{
						LLNotifications::instance().forceResponse(LLNotification::Params(params.name).payload(payload), 0);
					}
					else
					if ( (rlv_handler_t::isEnabled()) && (fRlvAutoAccept) )
					{
						if (IM_LURE_USER == dialog)
							gRlvHandler.setCanCancelTp(false);
						if (is_do_not_disturb)
							send_do_not_disturb_message(gMessageSystem, from_id);
						LLNotifications::instance().forceResponse(LLNotification::Params(params.name).payload(payload), 0);
					}
					else
					{
						LLNotifications::instance().add(params);
						if (IM_LURE_USER == dialog)
							gAgent.showLureDestination(LLAvatarActions::getSLURL(from_id), region_handle, pos.mV[VX], pos.mV[VY], pos.mV[VZ]);
						script_msg_api(from_id.asString().append(IM_LURE_USER == dialog ? ", 2" : ", 3"));
					}
				}
			}
		}
		break;
	case IM_GODLIKE_LURE_USER:
		{
			LLVector3 pos, look_at;
			U64 region_handle(0);
			U8 region_access(SIM_ACCESS_MIN);
			std::string region_info = ll_safe_string((char*)binary_bucket, binary_bucket_size);
			std::string region_access_str = LLStringUtil::null;
			std::string region_access_icn = LLStringUtil::null;
			std::string region_access_lc  = LLStringUtil::null;
			bool canUserAccessDstRegion = true;
			bool doesUserRequireMaturityIncrease = false;
            if (parse_lure_bucket(region_info, region_handle, pos, look_at, region_access))
			{
				region_access_str = LLViewerRegion::accessToString(region_access);
				region_access_icn = LLViewerRegion::getAccessIcon(region_access);
				region_access_lc  = region_access_str;
				LLStringUtil::toLower(region_access_lc);
				if (!gAgent.isGodlike())
				{
					switch (region_access)
					{
					case SIM_ACCESS_MIN:
					case SIM_ACCESS_PG:
						break;
					case SIM_ACCESS_MATURE:
						if (gAgent.isTeen())
						{
							canUserAccessDstRegion = false;
						}
						else if (gAgent.prefersPG())
						{
							doesUserRequireMaturityIncrease = true;
						}
						break;
					case SIM_ACCESS_ADULT:
						if (!gAgent.isAdult())
						{
							canUserAccessDstRegion = false;
						}
						else if (!gAgent.prefersAdult())
						{
							doesUserRequireMaturityIncrease = true;
						}
						break;
					default:
						llassert(0);
						break;
					}
				}
			}
			LLSD args;
			args["NAME"] = LLAvatarActions::getSLURL(from_id);
			args["MESSAGE"] = message;
			args["MATURITY_STR"] = region_access_str;
			args["MATURITY_ICON"] = region_access_icn;
			args["REGION_CONTENT_MATURITY"] = region_access_lc;
			LLSD payload;
			payload["from_id"] = from_id;
			payload["lure_id"] = session_id;
			payload["godlike"] = TRUE;
			payload["region_maturity"] = region_access;
            {
				LLNotifications::instance().forceResponse(LLNotification::Params("TeleportOffered").payload(payload), 0);
			}
        }
		break;
	case IM_GOTO_URL:
		{
			LLSD args;
			if (binary_bucket_size <= 0)
			{
				LL_WARNS("Messaging") << "bad binary_bucket_size: "
					<< binary_bucket_size
					<< " - aborting function." << LL_ENDL;
				return;
			}
			std::string url;
			url.assign((char*)binary_bucket, binary_bucket_size-1);
			args["MESSAGE"] = message;
			args["URL"] = url;
			LLSD payload;
			payload["url"] = url;
			LLNotificationsUtil::add("GotoURL", args, payload);
		}
		break;
	case IM_FRIENDSHIP_OFFERED:
		{
			LLSD payload;
			payload["from_id"] = from_id;
			payload["session_id"] = session_id;
			payload["online"] = (offline == IM_ONLINE);
            payload["sender"] = sender.getIPandPort();
			if (is_muted)
			{
				LLNotifications::instance().forceResponse(LLNotification::Params("OfferFriendship").payload(payload), 1);
			}
			else
			{
				if (is_do_not_disturb)
				{
                    send_do_not_disturb_message(gMessageSystem, from_id);
				}
				args["[NAME_SLURL]"] = LLAvatarActions::getSLURL(from_id);
				if (message.empty())
				{
				        LLNotificationsUtil::add("OfferFriendshipNoMessage", args, payload);
				}
				else
				{
					args["[MESSAGE]"] = message;
				    LLNotificationsUtil::add("OfferFriendship", args, payload);
				}
			}
		}
		break;
	case IM_FRIENDSHIP_ACCEPTED:
		{
			LLAvatarTracker::formFriendship(from_id);
			std::vector<std::string> strings;
			strings.push_back(from_id.asString());
			send_generic_message("requestonlinenotification", strings);
			args["NAME"] = LLAvatarActions::getSLURL(from_id);
			LLSD payload;
			payload["from_id"] = from_id;
			LLAvatarNameCache::get(from_id, boost::bind(&notification_display_name_callback, _1, _2, "FriendshipAccepted", args, payload));
		}
		break;
	case IM_FRIENDSHIP_DECLINED_DEPRECATED:
	default:
		LL_WARNS("Messaging") << "Instant message calling for unknown dialog "
				<< (S32)dialog << LL_ENDL;
		break;
	}
	LLWindow* viewer_window = gViewerWindow->getWindow();
	if (viewer_window && viewer_window->getMinimized() && gSavedSettings.getBOOL("LiruFlashWhenMinimized"))
	{
		viewer_window->flashIcon(5.f);
	}
}
void LLIMProcessing::requestOfflineMessages()
{
    static BOOL requested = FALSE;
    if (!requested
        && gMessageSystem
        && LLMuteList::getInstance()->isLoaded()
        && isAgentAvatarValid()
        && gAgent.getRegion()
        && gAgent.getRegion()->capabilitiesReceived())
    {
        std::string cap_url = gAgent.getRegionCapability("ReadOfflineMsgs");
        if (cap_url.empty()
            || gAgent.getRegionCapability("AcceptFriendship").empty()
            || gAgent.getRegionCapability("AcceptGroupInvite").empty())
        {
            requestOfflineMessagesLegacy();
        }
        else
        {
            LLHTTPClient::get(cap_url, new LLCoroResponder(
                LLIMProcessing::requestOfflineMessagesCoro));
        }
        requested = TRUE;
    }
}
void LLIMProcessing::requestOfflineMessagesCoro(const LLCoroResponder& responder)
{
	if (LLApp::isQuitting() || !gAgent.getRegion())
	{
		return;
	}
    auto status = responder.getStatus();
    if (!responder.isGoodStatus(status))
    {
        LL_WARNS("Messaging") << "Error requesting offline messages via capability " << responder.getURL() << ", Status: " << status << ", Reason: " << responder.getReason() << "\nFalling back to legacy method." << LL_ENDL;
        requestOfflineMessagesLegacy();
        return;
    }
    const auto& contents = responder.getContent();
    if (!contents.size())
    {
        LL_WARNS("Messaging") << "No contents received for offline messages via capability " << responder.getURL() << LL_ENDL;
        return;
    }
    LLSD messages;
    if (contents.isArray())
    {
        messages = *contents.beginArray();
    }
    else if (contents.has("messages"))
    {
        messages = contents["messages"];
    }
    else
    {
        LL_WARNS("Messaging") << "Invalid offline message content received via capability " << responder.getURL() << LL_ENDL;
        return;
    }
    if (!messages.isArray())
    {
        LL_WARNS("Messaging") << "Invalid offline message content received via capability " << responder.getURL() << LL_ENDL;
        return;
    }
    if (messages.emptyArray())
    {
        return;
    }
    LL_INFOS("Messaging") << "Processing offline messages." << LL_ENDL;
    LLHost sender = gAgent.getRegion()->getHost();
    LLSD::array_iterator i = messages.beginArray();
    LLSD::array_iterator iEnd = messages.endArray();
    for (; i != iEnd; ++i)
    {
        const LLSD &message_data(*i);
        LLVector3 position;
        if (message_data.has("position"))
        {
            position.setValue(message_data["position"]);
        }
        else
        {
            position.set(message_data["local_x"].asReal(), message_data["local_y"].asReal(), message_data["local_z"].asReal());
        }
        std::vector<U8> bin_bucket;
        if (message_data.has("binary_bucket"))
        {
            bin_bucket = message_data["binary_bucket"].asBinary();
        }
#if 0
        else
        {
            bin_bucket.push_back(0);
        }
#endif
        BOOL from_group;
        if (message_data["from_group"].isInteger())
        {
            from_group = message_data["from_group"].asInteger();
        }
        else
        {
            from_group = message_data["from_group"].asString() == "Y";
        }
        auto agentName = message_data["from_agent_name"].asString();
        auto message = message_data["message"].asString();
        LLIMProcessing::processNewMessage(
            message_data["from_agent_id"].asUUID(),
            from_group,
            message_data["to_agent_id"].asUUID(),
            message_data.has("offline") ? static_cast<U8>(message_data["offline"].asInteger()) : IM_OFFLINE,
            static_cast<EInstantMessage>(message_data["dialog"].asInteger()),
            message_data["transaction-id"].asUUID(),
            static_cast<U32>(message_data["timestamp"].asInteger()),
            agentName,
            message,
            message_data.has("parent_estate_id") ? static_cast<U32>(message_data["parent_estate_id"].asInteger()) : 1U,
            message_data["region_id"].asUUID(),
            position,
            bin_bucket.data(),
            bin_bucket.size(),
            sender,
            message_data["asset_id"].asUUID());
    }
}
void LLIMProcessing::requestOfflineMessagesLegacy()
{
    LL_INFOS("Messaging") << "Requesting offline messages (Legacy)." << LL_ENDL;
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_RetrieveInstantMessages);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    gAgent.sendReliableMessage();
}
