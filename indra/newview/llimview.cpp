/** 
 * @file LLIMMgr.cpp
 * @brief Container for Instant Messaging
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
#include "llviewerprecompiledheaders.h"
#include "llimview.h"
#include "llhttpclient.h"
#include "llhttpnode.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llsdutil_math.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentui.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llgroupactions.h"
#include "llimpanel.h"
#include "llchataitranslate.h"
#include "llmutelist.h"
#include "llspeakers.h"
#include "llvoavatar.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "rlvactions.h"
#include "rlvcommon.h"
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy viewerChatterBoxInvitationAcceptResponder_timeout;
LLIMMgr* gIMMgr = NULL;
std::string formatted_time(const time_t& the_time);
LLVOAvatar* find_avatar_from_object(const LLUUID& id);
LLColor4 agent_chat_color(const LLUUID& id, const std::string& name, bool local_chat)
{
	if (id.isNull() || (name == SYSTEM_FROM))
		return gSavedSettings.getColor4("SystemChatColor");
	if (id == gAgentID)
		return gSavedSettings.getColor4("UserChatColor");
	static const LLCachedControl<bool> color_linden_chat("ColorLindenChat");
	if (color_linden_chat && LLMuteList::getInstance()->isLinden(id))
		return gSavedSettings.getColor4("AscentLindenColor");
	if (local_chat && RlvActions::hasBehaviour(RLV_BHVR_SHOWNAMES))
		return gSavedSettings.getColor4("AgentChatColor");
	static const LLCachedControl<bool> color_friend_chat("ColorFriendChat");
	if (color_friend_chat && LLAvatarTracker::instance().isBuddy(id))
		return gSavedSettings.getColor4("AscentFriendColor");
	static const LLCachedControl<bool> color_eo_chat("ColorEstateOwnerChat");
	if (color_eo_chat)
	{
		const LLViewerObject* obj = gObjectList.findObject(id);
		if (const LLViewerRegion* parent_estate = obj ? obj->getRegion() : gAgent.getRegion())
			if (id == parent_estate->getOwner())
				return gSavedSettings.getColor4("AscentEstateOwnerColor");
	}
	return local_chat ? gSavedSettings.getColor4("AgentChatColor") : gSavedSettings.getColor("IMChatColor");
}
bool block_conference(const LLUUID& id)
{
	const U32 block(gSavedSettings.getU32("LiruBlockConferences"));
	if (block == 2) return !LLAvatarActions::isFriend(id);
	return block;
}
class LLViewerChatterBoxInvitationAcceptResponder : public LLHTTPClient::ResponderWithResult
{
public:
	LLViewerChatterBoxInvitationAcceptResponder(
		const LLUUID& session_id,
		LLIMMgr::EInvitationType invitation_type)
	{
		mSessionID = session_id;
		mInvitiationType = invitation_type;
	}
	void httpSuccess(void)
	{
		if ( gIMMgr)
		{
			LLFloaterIMPanel* floater = gIMMgr->findFloaterBySession(mSessionID);
			LLIMSpeakerMgr* speaker_mgr = floater ? floater->getSpeakerManager() : NULL;
			if (speaker_mgr)
			{
				speaker_mgr->setSpeakers(mContent);
				speaker_mgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(mSessionID));
			}
			if (LLIMMgr::INVITATION_TYPE_VOICE == mInvitiationType)
			{
				gIMMgr->startCall(mSessionID, LLVoiceChannel::INCOMING_CALL);
			}
			if ((mInvitiationType == LLIMMgr::INVITATION_TYPE_VOICE
				|| mInvitiationType == LLIMMgr::INVITATION_TYPE_IMMEDIATE)
				&& floater)
			{
				if (floater->getParent() == gFloaterView)
					floater->open();
				else
					LLFloaterChatterBox::showInstance(TRUE);
			}
			gIMMgr->clearPendingAgentListUpdates(mSessionID);
			gIMMgr->clearPendingInvitation(mSessionID);
		}
	}
	void httpFailure(void)
	{
		LL_WARNS() << "LLViewerChatterBoxInvitationAcceptResponder error [status:"
				<< mStatus << "]: " << mReason << LL_ENDL;
		if ( gIMMgr )
		{
			gIMMgr->clearPendingAgentListUpdates(mSessionID);
			gIMMgr->clearPendingInvitation(mSessionID);
			LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(mSessionID);
			if ( floaterp )
			{
				if ( 404 == mStatus )
				{
					std::string error_string;
					error_string = "session_does_not_exist_error";
					floaterp->showSessionStartError(error_string);
				}
			}
		}
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy(void) const { return viewerChatterBoxInvitationAcceptResponder_timeout; }
	char const* getName(void) const { return "LLViewerChatterBoxInvitationAcceptResponder"; }
private:
	LLUUID mSessionID;
	LLIMMgr::EInvitationType mInvitiationType;
};
LLUUID LLIMMgr::computeSessionID(
	EInstantMessage dialog,
	const LLUUID& other_participant_id)
{
	LLUUID session_id;
	if (IM_SESSION_GROUP_START == dialog)
	{
		session_id = other_participant_id;
	}
	else if (IM_SESSION_CONFERENCE_START == dialog)
	{
		session_id.generate();
	}
	else if (IM_SESSION_INVITE == dialog)
	{
		session_id = other_participant_id;
	}
	else
	{
		LLUUID agent_id = gAgent.getID();
		if (other_participant_id == agent_id)
		{
			session_id = agent_id;
		}
		else
		{
			session_id = other_participant_id ^ agent_id;
		}
	}
	return session_id;
}
bool inviteUserResponse(const LLSD& notification, const LLSD& response)
{
	if (!gIMMgr)
		return false;
	const LLSD& payload = notification["payload"];
	LLUUID session_id = payload["session_id"].asUUID();
	EInstantMessage type = (EInstantMessage)payload["type"].asInteger();
	LLIMMgr::EInvitationType inv_type = (LLIMMgr::EInvitationType)payload["inv_type"].asInteger();
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:
		{
			if (type == IM_SESSION_P2P_INVITE)
			{
				session_id = gIMMgr->addP2PSession(
					payload["session_name"].asString(),
					payload["caller_id"].asUUID(),
					payload["session_handle"].asString(),
					payload["session_uri"].asString());
				gIMMgr->startCall(session_id);
				LLFloaterChatterBox::showInstance(session_id);
				gIMMgr->clearPendingAgentListUpdates(session_id);
				gIMMgr->clearPendingInvitation(session_id);
			}
			else
			{
				gIMMgr->addSession(
					payload["session_name"].asString(),
					type,
					session_id);
				std::string url = gAgent.getRegion()->getCapability(
					"ChatSessionRequest");
				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						inv_type));
			}
		}
		break;
	case 2:
	{
		if (!LLMuteList::getInstance()->isMuted(payload["caller_id"].asUUID()))
		{
			LLMute mute(payload["caller_id"].asUUID(), payload["caller_name"].asString(), LLMute::AGENT);
			LLMuteList::getInstance()->add(mute);
		}
	}
	case 1:
	{
		if (type == IM_SESSION_P2P_INVITE)
		{
			std::string s = payload["session_handle"].asString();
			LLVoiceClient::getInstance()->declineInvite(s);
		}
		else
		{
			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");
			LLSD data;
			data["method"] = "decline invitation";
			data["session-id"] = session_id;
			LLHTTPClient::post(
				url,
				data,
				new LLHTTPClient::ResponderIgnore);
		}
	}
	gIMMgr->clearPendingAgentListUpdates(session_id);
	gIMMgr->clearPendingInvitation(session_id);
	break;
	}
	return false;
}
void LLIMMgr::toggle()
{
	static BOOL return_to_mouselook = FALSE;
	bool old_state = getFloaterOpen();
	if( gAgentCamera.cameraMouselook() && old_state )
	{
		return_to_mouselook = TRUE;
		gAgentCamera.changeCameraToDefault();
		return;
	}
	BOOL new_state = !old_state;
	if (new_state)
	{
		if ( gAgentCamera.cameraMouselook() )
		{
			return_to_mouselook = TRUE;
			gAgentCamera.changeCameraToDefault();
		}
	}
	else
	{
		if ( gAgentCamera.cameraThirdPerson() && return_to_mouselook )
		{
			gAgentCamera.changeCameraToMouselook();
		}
		return_to_mouselook = FALSE;
	}
	setFloaterOpen(new_state);
}
LLIMMgr::LLIMMgr() :
	mIMUnreadCount(0)
{
	mPendingInvitations = LLSD::emptyMap();
	mPendingAgentListUpdates = LLSD::emptyMap();
}
LLIMMgr::~LLIMMgr()
{
}
void LLIMMgr::addMessage(
	const LLUUID& session_id,
	const LLUUID& target_id,
	const std::string& from,
	const std::string& msg,
	const std::string& session_name,
	EInstantMessage dialog,
	U32 parent_estate_id,
	const LLUUID& region_id,
	const LLVector3& position,
	bool link_name)
{
	LLUUID other_participant_id = target_id;
	if (LLMuteList::getInstance()->isMuted(
			other_participant_id,
			LLMute::flagTextChat) && !LLMuteList::getInstance()->isLinden(from))
	{
		return;
	}
	LLUUID new_session_id = session_id;
	if (new_session_id.isNull())
	{
		new_session_id = computeSessionID(dialog, other_participant_id);
	}
	LLFloaterIMPanel* floater = findFloaterBySession(new_session_id);
	if (!floater)
	{
		floater = findFloaterBySession(other_participant_id);
		if (floater)
		{
			LL_INFOS() << "found the IM session " << new_session_id
				<< " by participant " << other_participant_id << LL_ENDL;
		}
	}
	if (LLVOAvatar* from_avatar = find_avatar_from_object(target_id)) from_avatar->mIdleTimer.reset();
	if(!floater)
	{
               if (gAgent.isInGroup(session_id) ? getIgnoreGroup(session_id) : dialog != IM_NOTHING_SPECIAL && dialog != IM_SESSION_P2P_INVITE && block_conference(other_participant_id))
			return;
		std::string name = (session_name.size() > 1) ? session_name : from;
		floater = createFloater(new_session_id, other_participant_id, name, dialog);
		if(gAgent.isGodlike())
		{
			std::ostringstream bonus_info;
			bonus_info << LLTrans::getString("***")+ " "+ LLTrans::getString("IMParentEstate") + LLTrans::getString(":") + " "
				<< parent_estate_id
				<< ((parent_estate_id == 1) ? LLTrans::getString(",") + LLTrans::getString("IMMainland") : "")
				<< ((parent_estate_id == 5) ? LLTrans::getString(",") + LLTrans::getString ("IMTeen") : "");
			floater->addHistoryLine(bonus_info.str(), gSavedSettings.getColor4("SystemChatColor"));
		}
		make_ui_sound("UISndNewIncomingIMSession");
	}
	LLColor4 color = agent_chat_color(other_participant_id, from, false);
	if (dialog == IM_BUSY_AUTO_RESPONSE)
	{
		color *= .75f;
		color += LLColor4::transparent*.25f;
	}
	if ( !link_name )
	{
		if (!LLChatAITranslate::instance().handleIncomingIM(floater, msg, color, false, other_participant_id, from))
			floater->addHistoryLine(msg, color, true, other_participant_id);
	}
	else
	{
		if( other_participant_id == session_id )
		{
			if (!LLChatAITranslate::instance().handleIncomingIM(floater, msg, color, true, LLUUID::null, from))
				floater->addHistoryLine(msg, color, true, LLUUID::null, from);
		}
		else
		{
			if (!LLChatAITranslate::instance().handleIncomingIM(floater, msg, color, true, other_participant_id, from))
				floater->addHistoryLine(msg, color, true, other_participant_id, from);
		}
	}
	if (!gIMMgr->getFloaterOpen() && floater->getParent() != gFloaterView)
	{
		mIMUnreadCount++;
	}
}
void LLIMMgr::addSystemMessage(const LLUUID& session_id, const std::string& message_name, const LLSD& args)
{
	LLUIString message;
	if (session_id.isNull())
	{
		message = LLTrans::getString(message_name);
		message.setArgs(args);
		LLChat chat(message);
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		LLFloaterChat::getInstance()->addChatHistory(chat);
	}
	else
	{
		message = LLTrans::getString(message_name + "-im");
		message.setArgs(args);
		if (hasSession(session_id))
		{
			gIMMgr->addMessage(session_id, LLUUID::null, SYSTEM_FROM, message.getString());
		}
	}
}
void LLIMMgr::clearNewIMNotification()
{
	mIMUnreadCount = 0;
}
int LLIMMgr::getIMUnreadCount()
{
	return mIMUnreadCount;
}
BOOL LLIMMgr::isIMSessionOpen(const LLUUID& uuid)
{
	LLFloaterIMPanel* floater = findFloaterBySession(uuid);
	if(floater) return TRUE;
	return FALSE;
}
void LLIMMgr::autoStartCallOnStartup(const LLUUID& session_id)
{
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if (!floater) return;
	if (floater->getSessionInitialized())
	{
		startCall(session_id);
	}
	else
	{
		floater->mStartCallOnInitialize = true;
	}
}
LLUUID LLIMMgr::addP2PSession(const std::string& name,
							const LLUUID& other_participant_id,
							const std::string& voice_session_handle,
							const std::string& caller_uri)
{
	LLUUID session_id = addSession(name, IM_NOTHING_SPECIAL, other_participant_id);
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if (floater)
	{
		LLVoiceChannelP2P* voice_channel = dynamic_cast<LLVoiceChannelP2P*>(floater->getVoiceChannel());
		if (voice_channel)
		{
			voice_channel->setSessionHandle(voice_session_handle, caller_uri);
		}
	}
	return session_id;
}
LLUUID LLIMMgr::addSession(
	const std::string& name,
	EInstantMessage dialog,
	const LLUUID& other_participant_id)
{
	LLUUID session_id = computeSessionID(dialog, other_participant_id);
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if(!floater)
	{
		uuid_vec_t ids;
		ids.push_back(other_participant_id);
		floater = createFloater(session_id, other_participant_id, name, dialog, ids, true);
		noteOfflineUsers(floater, ids);
		LLFloaterChatterBox::showInstance(session_id);
		if( dialog == IM_NOTHING_SPECIAL )
		{
			noteMutedUsers(floater, ids);
		}
		static LLCachedControl<bool> tear_off("OtherChatsTornOff");
		if(tear_off)
		{
			LLFloaterChatterBox::getInstance(LLSD())->removeFloater(floater);
			gFloaterView->addChild(floater);
			gFloaterView->bringToFront(floater);
		}
		else
			LLFloaterChatterBox::getInstance(LLSD())->showFloater(floater);
	}
	else
	{
		floater->open();
	}
	if(gSavedSettings.getBOOL("PhoenixIMAnnounceStealFocus"))
	{
		floater->setInputFocus(TRUE);
	}
	return floater->getSessionID();
}
LLUUID LLIMMgr::addSession(
	const std::string& name,
	EInstantMessage dialog,
	const LLUUID& other_participant_id,
	const uuid_vec_t& ids)
{
	if (0 == ids.size())
	{
		return LLUUID::null;
	}
	LLUUID session_id = computeSessionID(
		dialog,
		other_participant_id);
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if(!floater)
	{
		floater = createFloater(session_id, other_participant_id, name, dialog, ids, true);
		if ( !floater ) return LLUUID::null;
		noteOfflineUsers(floater, ids);
		LLFloaterChatterBox::showInstance(session_id);
		if( dialog == IM_NOTHING_SPECIAL )
		{
			noteMutedUsers(floater, ids);
		}
	}
	else
	{
		floater->open();
	}
	if(gSavedSettings.getBOOL("PhoenixIMAnnounceStealFocus"))
	{
		floater->setInputFocus(TRUE);
	}
	return floater->getSessionID();
}
void LLIMMgr::removeSession(const LLUUID& session_id)
{
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if(floater)
	{
		mFloaters.erase(floater->getHandle());
		LLFloaterChatterBox::getInstance(LLSD())->removeFloater(floater);
		clearPendingInvitation(session_id);
		clearPendingAgentListUpdates(session_id);
	}
}
void LLIMMgr::inviteToSession(
	const LLUUID& session_id,
	const std::string& session_name,
	const LLUUID& caller_id,
	const std::string& caller_name,
	EInstantMessage type,
	EInvitationType inv_type,
	const std::string& session_handle,
	const std::string& session_uri)
{
	if (LLMuteList::getInstance()->isMuted(caller_id))
	{
		return;
	}
	std::string notify_box_type;
	BOOL ad_hoc_invite = FALSE;
	if(type == IM_SESSION_P2P_INVITE)
	{
		notify_box_type = "VoiceInviteP2P";
	}
	else if ( gAgent.isInGroup(session_id) )
	{
		notify_box_type = "VoiceInviteGroup";
	}
	else if ( inv_type == INVITATION_TYPE_VOICE )
	{
		notify_box_type = "VoiceInviteAdHoc";
		ad_hoc_invite = TRUE;
	}
	else if ( inv_type == INVITATION_TYPE_IMMEDIATE )
	{
		notify_box_type = "InviteAdHoc";
		ad_hoc_invite = TRUE;
	}
	LLSD payload;
	payload["session_id"] = session_id;
	payload["session_name"] = session_name;
	payload["caller_id"] = caller_id;
	payload["caller_name"] = caller_name;
	payload["type"] = type;
	payload["inv_type"] = inv_type;
	payload["session_handle"] = session_handle;
	payload["session_uri"] = session_uri;
	payload["notify_box_type"] = notify_box_type;
	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(session_id);
	if (channelp && channelp->callStarted())
	{
		LLNotifications::instance().forceResponse(LLNotification::Params("VoiceInviteP2P").payload(payload), 0);
		return;
	}
	if (channelp && channelp->isCapRequestPending())
	{
		LL_INFOS("Voice") << "Ignoring voice invite; Cap already pending for session " << session_id << LL_ENDL;
		return;
	}
	if (type == IM_SESSION_P2P_INVITE || ad_hoc_invite)
	{
		if	(
				(gSavedSettings.getBOOL("VoiceCallsFriendsOnly") && (LLAvatarTracker::instance().getBuddyInfo(caller_id) == NULL))
			)
		{
			LLNotifications::instance().forceResponse(LLNotification::Params("VoiceInviteP2P").payload(payload), 1);
			return;
		}
	}
	if ( !mPendingInvitations.has(session_id.asString()) )
	{
		LLSD args;
		args["NAME"] = LLAvatarActions::getSLURL(caller_id);
		args["GROUP"] = LLGroupActions::getSLURL(session_id);
		LLNotifications::instance().add(notify_box_type,
				args,
				payload,
				&inviteUserResponse);
		mPendingInvitations[session_id.asString()] = LLSD();
	}
}
void LLIMMgr::setFloaterOpen(BOOL set_open)
{
	if (set_open)
	{
		LLFloaterChatterBox::showInstance();
	}
	else
	{
		LLFloaterChatterBox::hideInstance();
	}
}
BOOL LLIMMgr::getFloaterOpen()
{
	return LLFloaterChatterBox::instanceVisible(LLSD());
}
void LLIMMgr::disconnectAllSessions()
{
	LLFloaterIMPanel* floater = NULL;
	std::set<LLHandle<LLFloater> >::iterator handle_it;
	for(handle_it = mFloaters.begin();
		handle_it != mFloaters.end();
		)
	{
		floater = (LLFloaterIMPanel*)handle_it->get();
		++handle_it;
		if (floater)
		{
			floater->setEnabled(FALSE);
			floater->close(TRUE);
		}
	}
}
LLFloaterIMPanel* LLIMMgr::findFloaterBySession(const LLUUID& session_id)
{
	LLFloaterIMPanel* rv = NULL;
	std::set<LLHandle<LLFloater> >::iterator handle_it;
	for(handle_it = mFloaters.begin();
		handle_it != mFloaters.end();
		++handle_it)
	{
		rv = (LLFloaterIMPanel*)handle_it->get();
		if(rv && session_id == rv->getSessionID())
		{
			break;
		}
		rv = NULL;
	}
	return rv;
}
BOOL LLIMMgr::hasSession(const LLUUID& session_id)
{
	return (findFloaterBySession(session_id) != NULL);
}
void LLIMMgr::clearPendingInvitation(const LLUUID& session_id)
{
	if ( mPendingInvitations.has(session_id.asString()) )
	{
		mPendingInvitations.erase(session_id.asString());
	}
}
void LLIMMgr::processAgentListUpdates(const LLUUID& session_id, const LLSD& body)
{
	LLFloaterIMPanel* im_floater = gIMMgr->findFloaterBySession(session_id);
	if (!im_floater) return;
	LLIMSpeakerMgr* speaker_mgr = im_floater->getSpeakerManager();
	if (speaker_mgr)
	{
		speaker_mgr->updateSpeakers(body);
		speaker_mgr->update(true);
	}
	else
	{
		gIMMgr->addPendingAgentListUpdates(
			session_id,
			body);
	}
}
LLSD LLIMMgr::getPendingAgentListUpdates(const LLUUID& session_id)
{
	if ( mPendingAgentListUpdates.has(session_id.asString()) )
	{
		return mPendingAgentListUpdates[session_id.asString()];
	}
	else
	{
		return LLSD();
	}
}
void LLIMMgr::addPendingAgentListUpdates(
	const LLUUID& session_id,
	const LLSD& updates)
{
	LLSD::map_const_iterator iter;
	if ( !mPendingAgentListUpdates.has(session_id.asString()) )
	{
		mPendingAgentListUpdates[session_id.asString()] = LLSD::emptyMap();
	}
	if (
		updates.has("agent_updates") &&
		updates["agent_updates"].isMap() &&
		updates.has("updates") &&
		updates["updates"].isMap() )
	{
		LLSD update_types = LLSD::emptyArray();
		LLSD::array_iterator array_iter;
		update_types.append("agent_updates");
		update_types.append("updates");
		for (
			array_iter = update_types.beginArray();
			array_iter != update_types.endArray();
			++array_iter)
		{
			for (
				iter = updates[array_iter->asString()].beginMap();
				iter != updates[array_iter->asString()].endMap();
				++iter)
			{
				mPendingAgentListUpdates[session_id.asString()][array_iter->asString()][iter->first] =
					iter->second;
			}
		}
	}
	else if (
		updates.has("updates") &&
		updates["updates"].isMap() )
	{
		for (
			iter = updates["updates"].beginMap();
			iter != updates["updates"].endMap();
			++iter)
		{
			mPendingAgentListUpdates[session_id.asString()]["updates"][iter->first] =
				iter->second;
		}
	}
}
void LLIMMgr::clearPendingAgentListUpdates(const LLUUID& session_id)
{
	if ( mPendingAgentListUpdates.has(session_id.asString()) )
	{
		mPendingAgentListUpdates.erase(session_id.asString());
	}
}
bool LLIMMgr::startCall(const LLUUID& session_id, LLVoiceChannel::EDirection direction)
{
	LLFloaterIMPanel* floater = gIMMgr->findFloaterBySession(session_id);
	if (!floater) return false;
	LLVoiceChannel* voice_channel = floater->getVoiceChannel();
	if (!voice_channel) return false;
	voice_channel->setCallDirection(direction);
	voice_channel->activate();
	return true;
}
bool LLIMMgr::endCall(const LLUUID& session_id)
{
	LLFloaterIMPanel* floater = gIMMgr->findFloaterBySession(session_id);
	if (!floater) return false;
	LLVoiceChannel* voice_channel = floater->getVoiceChannel();
	if (!voice_channel) return false;
	voice_channel->deactivate();
	{
		floater->getSpeakerManager()->update(FALSE);
	}
	return true;
}
LLFloaterIMPanel* LLIMMgr::createFloater(
	const LLUUID& session_id,
	const LLUUID& other_participant_id,
	const std::string& session_label,
	const EInstantMessage& dialog,
	const uuid_vec_t& ids,
	bool user_initiated)
{
	if (session_id.isNull())
	{
		LL_WARNS() << "Creating LLFloaterIMPanel with null session ID" << LL_ENDL;
	}
	LL_INFOS() << "LLIMMgr::createFloater: from " << other_participant_id
			<< " in session " << session_id << LL_ENDL;
	LLFloaterIMPanel* floater = new LLFloaterIMPanel(session_label, session_id, other_participant_id, dialog, ids);
	LLTabContainer::eInsertionPoint i_pt = user_initiated ? LLTabContainer::RIGHT_OF_CURRENT : LLTabContainer::END;
	LLFloaterChatterBox::getInstance(LLSD())->addFloater(floater, FALSE, i_pt);
	static LLCachedControl<bool> tear_off("OtherChatsTornOff");
	if (tear_off)
	{
		LLFloaterChatterBox::getInstance(LLSD())->removeFloater(floater);
		gFloaterView->addChild(floater);
		LLFloater* focused_floater = gFloaterView->getFocusedFloater();
		floater->open();
		static LLCachedControl<bool> minimize("OtherChatsTornOffAndMinimized");
		if (focused_floater != NULL)
		{
			floater->setMinimized(minimize);
			focused_floater->setFocus(true);
		}
		else if (minimize)
		{
			floater->setFocus(false);
			floater->setMinimized(true);
		}
	}
	mFloaters.insert(floater->getHandle());
	return floater;
}
void LLIMMgr::addNotifiedNonFriendSessionID(const LLUUID& session_id)
{
	mNotifiedNonFriendSessions.insert(session_id);
}
bool LLIMMgr::isNonFriendSessionNotified(const LLUUID& session_id)
{
	return mNotifiedNonFriendSessions.end() != mNotifiedNonFriendSessions.find(session_id);
}
std::string LLIMMgr::getOfflineMessage(const LLUUID& id)
{
	std::string full_name;
	if (LLAvatarNameCache::getNSName(id, full_name))
	{
		LLUIString offline = LLTrans::getString("offline_message");
		offline.setArg("[NAME]", full_name);
		return offline;
	}
	return LLStringUtil::null;
}
void LLIMMgr::noteOfflineUsers(
	LLFloaterIMPanel* floater,
	const uuid_vec_t& ids)
{
	if(ids.empty())
	{
		const std::string& only_user = LLTrans::getString("only_user_message");
		floater->addHistoryLine(only_user, gSavedSettings.getColor4("SystemChatColor"));
	}
	else
	{
		const LLRelationship* info = nullptr;
		LLAvatarTracker& at = LLAvatarTracker::instance();
		for(const auto& id : ids)
		{
			info = at.getBuddyInfo(id);
			if (info && !info->isOnline())
			{
				auto offline(getOfflineMessage(id));
				if (!offline.empty())
					floater->addHistoryLine(offline, gSavedSettings.getColor4("SystemChatColor"));
			}
		}
	}
}
void LLIMMgr::noteMutedUsers(LLFloaterIMPanel* floater,
								  const uuid_vec_t& ids)
{
	LLMuteList *ml = LLMuteList::getInstance();
	if( !ml )
	{
		return;
	}
	S32 count = ids.size();
	if(count > 0)
	{
		for(S32 i = 0; i < count; ++i)
		{
			if( ml->isMuted(ids.at(i)) )
			{
				LLUIString muted = LLTrans::getString("muted_message");
				floater->addHistoryLine(muted);
				break;
			}
		}
	}
}
void LLIMMgr::processIMTypingStart(const LLUUID& from_id, const EInstantMessage im_type)
{
	processIMTypingCore(from_id, im_type, TRUE);
}
void LLIMMgr::processIMTypingStop(const LLUUID& from_id, const EInstantMessage im_type)
{
	processIMTypingCore(from_id, im_type, FALSE);
}
void LLIMMgr::processIMTypingCore(const LLUUID& from_id, const EInstantMessage im_type, BOOL typing)
{
	LLUUID session_id = computeSessionID(im_type, from_id);
	LLFloaterIMPanel* im_floater = findFloaterBySession(session_id);
	if (im_floater)
	{
		im_floater->processIMTyping(from_id, typing);
	}
}
void LLIMMgr::updateFloaterSessionID(
	const LLUUID& old_session_id,
	const LLUUID& new_session_id)
{
	LLFloaterIMPanel* floater = findFloaterBySession(old_session_id);
	if (floater)
	{
		floater->sessionInitReplyReceived(new_session_id);
	}
}
void LLIMMgr::loadIgnoreGroup()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "ignore_groups.xml");
	LLSD settings_llsd;
	llifstream file;
	file.open(filename);
	if (file.is_open())
	{
		LLSDSerialize::fromXML(settings_llsd, file);
		mIgnoreGroupList.clear();
		for(LLSD::array_const_iterator iter = settings_llsd.beginArray();
		    iter != settings_llsd.endArray(); ++iter)
		{
			mIgnoreGroupList.push_back( iter->asUUID() );
		}
	}
	else
	{
	}
}
void LLIMMgr::saveIgnoreGroup()
{
	std::string user_dir = gDirUtilp->getLindenUserDir(true);
	if (!user_dir.empty())
	{
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "ignore_groups.xml");
		LLSD settings_llsd = LLSD::emptyArray();
		for(std::list<LLUUID>::iterator iter = mIgnoreGroupList.begin();
		    iter != mIgnoreGroupList.end(); ++iter)
		{
			settings_llsd.append(*iter);
		}
		llofstream file;
		file.open(filename);
		LLSDSerialize::toPrettyXML(settings_llsd, file);
	}
}
void LLIMMgr::updateIgnoreGroup(const LLUUID& group_id, bool ignore)
{
	if (group_id.isNull()) return;
	if (getIgnoreGroup(group_id) == ignore)
	{
		return;
	}
	else if (!ignore)
	{
		mIgnoreGroupList.remove(group_id);
	}
	else
	{
		mIgnoreGroupList.push_back(group_id);
	}
}
bool LLIMMgr::getIgnoreGroup(const LLUUID& group_id) const
{
	if (group_id.notNull())
	{
		std::list<LLUUID>::const_iterator found =
			std::find( mIgnoreGroupList.begin(), mIgnoreGroupList.end(),
			           group_id);
		if (found != mIgnoreGroupList.end())
		{
			return true;
		}
	}
	return false;
}
LLFloaterChatterBox* LLIMMgr::getFloater()
{
	return LLFloaterChatterBox::getInstance(LLSD());
}
class LLViewerChatterBoxSessionStartReply final : public LLHTTPNode
{
public:
	void describe(Description& desc) const override
	{
		desc.shortInfo("Used for receiving a reply to a request to initialize an ChatterBox session");
		desc.postAPI();
		desc.input(
			"{\"client_session_id\": UUID, \"session_id\": UUID, \"success\" boolean, \"reason\": string");
		desc.source(__FILE__, __LINE__);
	}
	void post(ResponsePtr response,
					  const LLSD& context,
	          const LLSD& input) const override
	{
		LLUUID session_id;
		LLSD body = input["body"];
		bool success = body["success"].asBoolean();
		LLUUID temp_session_id = body["temp_session_id"].asUUID();
		if ( success )
		{
			session_id = body["session_id"].asUUID();
			gIMMgr->updateFloaterSessionID(temp_session_id, session_id);
			LLFloaterIMPanel* im_floater = gIMMgr->findFloaterBySession(session_id);
			LLIMSpeakerMgr* speaker_mgr = im_floater ? im_floater->getSpeakerManager() : NULL;
			if (speaker_mgr)
			{
				speaker_mgr->setSpeakers(body);
				speaker_mgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(session_id));
			}
			if (im_floater)
			{
				if ( body.has("session_info") )
				{
					im_floater->processSessionUpdate(body["session_info"]);
				}
			}
			gIMMgr->clearPendingAgentListUpdates(session_id);
		}
		else
		{
			if (LLFloaterIMPanel* floater = gIMMgr->findFloaterBySession(temp_session_id))
			{
				floater->showSessionStartError(body["error"].asString());
			}
		}
		gIMMgr->clearPendingAgentListUpdates(session_id);
	}
};
class LLViewerChatterBoxSessionEventReply final : public LLHTTPNode
{
public:
	void describe(Description& desc) const override
	{
		desc.shortInfo("Used for receiving a reply to a ChatterBox session event");
		desc.postAPI();
		desc.input(
			"{\"event\": string, \"reason\": string, \"success\": boolean, \"session_id\": UUID");
		desc.source(__FILE__, __LINE__);
	}
	void post(ResponsePtr response,
					  const LLSD& context,
	          const LLSD& input) const override
	{
		LLSD body = input["body"];
		bool success = body["success"].asBoolean();
		LLUUID session_id = body["session_id"].asUUID();
		if ( !success )
		{
			if (auto* floater =  gIMMgr->findFloaterBySession(session_id))
			{
				floater->showSessionEventError(
					body["event"].asString(),
					body["error"].asString());
			}
		}
	}
};
class LLViewerForceCloseChatterBoxSession: public LLHTTPNode
{
public:
	void post(ResponsePtr response,
					  const LLSD& context,
	          const LLSD& input) const override
	{
		LLUUID session_id = input["body"]["session_id"].asUUID();
		std::string reason = input["body"]["reason"].asString();
		if (auto* floater = gIMMgr ->findFloaterBySession(session_id))
		{
			floater->showSessionForceClose(reason);
		}
	}
};
class LLViewerChatterBoxSessionAgentListUpdates final : public LLHTTPNode
{
public:
	void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const override
	{
		const LLUUID& session_id = input["body"]["session_id"].asUUID();
		gIMMgr->processAgentListUpdates(session_id, input["body"]);
	}
};
class LLViewerChatterBoxSessionUpdate final : public LLHTTPNode
{
public:
	void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const override
	{
		LLUUID session_id = input["body"]["session_id"].asUUID();
		LLFloaterIMPanel* im_floater = gIMMgr->findFloaterBySession(session_id);
		if ( im_floater )
		{
			im_floater->processSessionUpdate(input["body"]["info"]);
		}
		LLIMSpeakerMgr* im_mgr = im_floater ? im_floater->getSpeakerManager() : NULL;
		if (im_mgr)
		{
			im_mgr->processSessionUpdate(input["body"]["info"]);
		}
	}
};
void leave_group_chat(const LLUUID& from_id, const LLUUID& session_id)
{
	std::string name;
	LLAgentUI::buildFullname(name);
	pack_instant_message(gMessageSystem, gAgentID, false, gAgentSessionID, from_id,
		name, LLStringUtil::null, IM_ONLINE, IM_SESSION_LEAVE, session_id);
	gAgent.sendReliableMessage();
	gIMMgr->removeSession(session_id);
}
class LLViewerChatterBoxInvitation final : public LLHTTPNode
{
public:
	void post(
		ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const override
	{
		if ( input["body"].has("instantmessage") )
		{
			LLSD message_params =
				input["body"]["instantmessage"]["message_params"];
			if (gNoRender)
			{
				return;
			}
			LLChat chat;
			std::string message = message_params["message"].asString();
			std::string name = message_params["from_name"].asString();
			LLUUID from_id = message_params["from_id"].asUUID();
			LLUUID session_id = message_params["id"].asUUID();
			std::vector<U8> bin_bucket = message_params["data"]["binary_bucket"].asBinary();
			U8 offline = (U8)message_params["offline"].asInteger();
			time_t timestamp =
				(time_t) message_params["timestamp"].asInteger();
			bool is_do_not_disturb = gAgent.isDoNotDisturb();
			BOOL is_muted = LLMuteList::getInstance()->isMuted(
				from_id,
				name,
				LLMute::flagTextChat);
			BOOL is_linden = LLMuteList::getInstance()->isLinden(name);
			std::string separator_string(": ");
			int message_offset=0;
			std::string prefix = message.substr(0, 4);
			if (prefix == "/me " || prefix == "/me'")
			{
				separator_string = "";
				message_offset = 3;
				chat.mChatStyle = CHAT_STYLE_IRC;
			}
			chat.mMuted = is_muted && !is_linden;
			chat.mFromID = from_id;
			chat.mFromName = name;
			if (!is_linden && (is_do_not_disturb || is_muted))
			{
				return;
			}
			bool group = gAgent.isInGroup(session_id);
			if ( (RlvActions::hasBehaviour(RLV_BHVR_RECVIM)) || (RlvActions::hasBehaviour(RLV_BHVR_RECVIMFROM)) )
			{
				if (group)
				{
					if (!RlvActions::canReceiveIM(session_id))
						return;
				}
				else if (!RlvActions::canReceiveIM(from_id))
				{
					message = RlvStrings::getString(RLV_STRING_BLOCKED_RECVIM);
				}
			}
			std::string saved;
			if(offline == IM_OFFLINE)
			{
				LLStringUtil::format_map_t args;
				args["[LONG_TIMESTAMP]"] = formatted_time(timestamp);
				saved = LLTrans::getString("Saved_message", args);
			}
			std::string buffer = separator_string + saved + message.substr(message_offset);
			BOOL is_this_agent = FALSE;
			if(from_id == gAgentID)
			{
				is_this_agent = TRUE;
			}
			gIMMgr->addMessage(
				session_id,
				from_id,
				name,
				buffer,
				std::string((char*)&bin_bucket[0]),
				IM_SESSION_INVITE,
				message_params["parent_estate_id"].asInteger(),
				message_params["region_id"].asUUID(),
				ll_vector3_from_sd(message_params["position"]),
				true);
			std::string prepend_msg;
			if (group)
			{
				if (gIMMgr->getIgnoreGroup(session_id))
				{
					leave_group_chat(from_id, session_id);
					return;
				}
				else if (gSavedSettings.getBOOL("OptionShowGroupNameInChatIM"))
				{
					LLGroupData group_data;
					gAgent.getGroupData(session_id, group_data);
					prepend_msg = "[";
					prepend_msg += group_data.mName;
					prepend_msg += "] ";
				}
			}
			else
			{
				if (from_id != session_id && block_conference(from_id))
				{
					leave_group_chat(from_id, session_id);
					return;
				}
				prepend_msg = std::string("IM: ");
			}
			chat.mText = prepend_msg + name + separator_string + saved + message.substr(message_offset);
			LLFloaterChat::addChat(chat, TRUE, is_this_agent);
			std::string url = gAgent.getRegionCapability("ChatSessionRequest");
			if (!url.empty())
			{
				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						LLIMMgr::INVITATION_TYPE_INSTANT_MESSAGE));
			}
		}
		else if ( input["body"].has("voice") )
		{
			if (gNoRender)
			{
				return;
			}
			if(!LLVoiceClient::getInstance()->voiceEnabled())
			{
				return;
			}
			gIMMgr->inviteToSession(
				input["body"]["session_id"].asUUID(),
				input["body"]["session_name"].asString(),
				input["body"]["from_id"].asUUID(),
				input["body"]["from_name"].asString(),
				IM_SESSION_INVITE,
				LLIMMgr::INVITATION_TYPE_VOICE);
		}
		else if ( input["body"].has("immediate") )
		{
			gIMMgr->inviteToSession(
				input["body"]["session_id"].asUUID(),
				input["body"]["session_name"].asString(),
				input["body"]["from_id"].asUUID(),
				input["body"]["from_name"].asString(),
				IM_SESSION_INVITE,
				LLIMMgr::INVITATION_TYPE_IMMEDIATE);
		}
	}
};
LLHTTPRegistration<LLViewerChatterBoxSessionStartReply>
   gHTTPRegistrationMessageChatterboxsessionstartreply(
	   "/message/ChatterBoxSessionStartReply");
LLHTTPRegistration<LLViewerChatterBoxSessionEventReply>
   gHTTPRegistrationMessageChatterboxsessioneventreply(
	   "/message/ChatterBoxSessionEventReply");
LLHTTPRegistration<LLViewerForceCloseChatterBoxSession>
    gHTTPRegistrationMessageForceclosechatterboxsession(
		"/message/ForceCloseChatterBoxSession");
LLHTTPRegistration<LLViewerChatterBoxSessionAgentListUpdates>
    gHTTPRegistrationMessageChatterboxsessionagentlistupdates(
	    "/message/ChatterBoxSessionAgentListUpdates");
LLHTTPRegistration<LLViewerChatterBoxSessionUpdate>
    gHTTPRegistrationMessageChatterBoxSessionUpdate(
	    "/message/ChatterBoxSessionUpdate");
LLHTTPRegistration<LLViewerChatterBoxInvitation>
    gHTTPRegistrationMessageChatterBoxInvitation(
		"/message/ChatterBoxInvitation");
