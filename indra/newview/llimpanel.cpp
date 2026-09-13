/** 
 * @file llimpanel.cpp
 * @brief LLIMPanel class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "llimpanel.h"
#include "llchataitranslate.h"
#include "ascentkeyword.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentui.h"
#include "llautoreplace.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloateravatarpicker.h"
#include "llfloaterchat.h"
#include "llfloaterimtranslationconfig.h"
#include "llviewerregion.h"
#include "llfloaterinventory.h"
#include "llfloaterreporter.h"
#include "llfloaterwebcontent.h"
#include "llgroupactions.h"
#include "llhttpclient.h"
#include "llimview.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "lllineeditor.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llparticipantlist.h"
#include "llspeakers.h"
#include "llstylemap.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llversioninfo.h"
#include "llviewerobjectlist.h"
#include "llviewertexteditor.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoicechannel.h"
#include "llvoiceclient.h"
#include "llvoicewebrtc.h"
#include "llviewercontrol.h"
#include <boost/algorithm/string.hpp>
#include <boost/lambda/lambda.hpp>
#include "rlvactions.h"
#include "rlvcommon.h"
BOOL is_agent_mappable(const LLUUID& agent_id);
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy sessionInviteResponder_timeout;
const S32 LINE_HEIGHT = 16;
const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 130;
static std::string sTitleString = "Instant Message with [NAME]";
static std::string sTypingStartString = "[NAME]: ...";
static std::string sSessionStartString = "Starting session with [NAME] please wait.";
void session_starter_helper(
	const LLUUID& temp_session_id,
	const LLUUID& other_participant_id,
	EInstantMessage im_type)
{
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, other_participant_id);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addU8Fast(_PREHASH_Dialog, im_type);
	msg->addUUIDFast(_PREHASH_ID, temp_session_id);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP);
	std::string name;
	LLAgentUI::buildFullname(name);
	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, LLStringUtil::null);
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
}
void start_deprecated_conference_chat(
	const LLUUID& temp_session_id,
	const LLUUID& creator_id,
	const LLUUID& other_participant_id,
	const LLSD& agents_to_invite)
{
	U8* bucket;
	U8* pos;
	S32 count;
	S32 bucket_size;
	count = agents_to_invite.size();
	bucket_size = UUID_BYTES * count;
	bucket = new U8[bucket_size];
	pos = bucket;
	for(S32 i = 0; i < count; ++i)
	{
		LLUUID agent_id = agents_to_invite[i].asUUID();
		memcpy(pos, &agent_id, UUID_BYTES);
		pos += UUID_BYTES;
	}
	session_starter_helper(
		temp_session_id,
		other_participant_id,
		IM_SESSION_CONFERENCE_START);
	gMessageSystem->addBinaryDataFast(
		_PREHASH_BinaryBucket,
		bucket,
		bucket_size);
	gAgent.sendReliableMessage();
	delete[] bucket;
}
class LLStartConferenceChatResponder : public LLHTTPClient::ResponderIgnoreBody
{
public:
	LLStartConferenceChatResponder(
		const LLUUID& temp_session_id,
		const LLUUID& creator_id,
		const LLUUID& other_participant_id,
		const LLSD& agents_to_invite)
	{
		mTempSessionID = temp_session_id;
		mCreatorID = creator_id;
		mOtherParticipantID = other_participant_id;
		mAgents = agents_to_invite;
	}
	void httpFailure()
	{
		if ( mStatus == 400 )
		{
			start_deprecated_conference_chat(
				mTempSessionID,
				mCreatorID,
				mOtherParticipantID,
				mAgents);
		}
	}
	char const* getName() const { return "LLStartConferenceChatResponder"; }
private:
	LLUUID mTempSessionID;
	LLUUID mCreatorID;
	LLUUID mOtherParticipantID;
	LLSD mAgents;
};
class LLStartP2PVoiceResponder : public LLHTTPClient::ResponderWithResult
{
public:
	explicit LLStartP2PVoiceResponder(const LLUUID& session_id)
	: mSessionID(session_id)
	{
	}
	void httpSuccess(void) override
	{
		LL_INFOS("Voice") << "start p2p voice (HTTP) OK session=" << mSessionID
						  << " body=" << getContent() << LL_ENDL;
		if (LLVoiceChannel* ch = LLVoiceChannel::getChannelByID(mSessionID))
		{
			ch->setCapRequestPending(false);
		}
	}
	void httpFailure(void) override
	{
		LL_WARNS("Voice") << "start p2p voice (HTTP) failed status=" << mStatus
						  << " reason=" << mReason
						  << " session=" << mSessionID << LL_ENDL;
		if (LLVoiceChannel* ch = LLVoiceChannel::getChannelByID(mSessionID))
		{
			ch->setCapRequestPending(false);
		}
		if (mStatus == 400)
		{
			LLFloaterIMPanel* floater = gIMMgr ? gIMMgr->findFloaterBySession(mSessionID) : NULL;
			if (floater)
			{
				floater->showSessionStartError("session_does_not_exist_error");
			}
		}
	}
	char const* getName() const override { return "LLStartP2PVoiceResponder"; }
private:
	LLUUID mSessionID;
};
bool send_start_session_messages(
	const LLUUID& temp_session_id,
	const LLUUID& other_participant_id,
	const uuid_vec_t& ids,
	EInstantMessage dialog,
	bool p2p_as_adhoc_call)
{
	if ( dialog == IM_SESSION_GROUP_START )
	{
		session_starter_helper(
			temp_session_id,
			other_participant_id,
			dialog);
		gMessageSystem->addBinaryDataFast(
			_PREHASH_BinaryBucket,
			EMPTY_BINARY_BUCKET,
			EMPTY_BINARY_BUCKET_SIZE);
		gAgent.sendReliableMessage();
		return true;
	}
	else if ( dialog == IM_SESSION_CONFERENCE_START )
	{
		if (ids.empty()) return true;
		LLSD agents;
		for (int i = 0; i < (S32) ids.size(); i++)
		{
			agents.append(ids.at(i));
		}
		LLViewerRegion* region = gAgent.getRegion();
		std::string url(region ? region->getCapability("ChatSessionRequest") : "");
		if (!url.empty())
		{
			LLSD data;
			data["method"] = "start conference";
			data["session-id"] = temp_session_id;
			data["params"] = agents;
			LLHTTPClient::post(
				url,
				data,
				new LLStartConferenceChatResponder(
					temp_session_id,
					gAgent.getID(),
					other_participant_id,
					data["params"]));
		}
		else
		{
			start_deprecated_conference_chat(
				temp_session_id,
				gAgent.getID(),
				other_participant_id,
				agents);
		}
	}
	else if (p2p_as_adhoc_call
			 && (dialog == IM_SESSION_P2P_INVITE || dialog == IM_NOTHING_SPECIAL))
	{
		LLViewerRegion* region = gAgent.getRegion();
		std::string url(region ? region->getCapability("ChatSessionRequest") : "");
		if (!url.empty())
		{
			LLSD data;
			data["method"] = "start p2p voice";
			data["session-id"] = temp_session_id;
			data["params"] = other_participant_id;
			LLSD alt;
			std::string voice_type = gSavedSettings.getString("VoiceServerType");
			if (voice_type.empty())
			{
				voice_type = LLVoiceClient::getInstance()->getVersion().internalVoiceServerType;
			}
			if (voice_type.empty())
			{
				voice_type = WEBRTC_VOICE_SERVER_TYPE;
			}
			alt["voice_server_type"] = voice_type;
			data["alt_params"] = alt;
			if (LLVoiceChannel* ch = LLVoiceChannel::getChannelByID(temp_session_id))
			{
				ch->setCapRequestPending(true);
			}
			LL_INFOS("Voice") << "start p2p voice (HTTP) session=" << temp_session_id
							  << " peer=" << other_participant_id
							  << " type=" << voice_type << LL_ENDL;
			LLHTTPClient::post(url, data, new LLStartP2PVoiceResponder(temp_session_id));
		}
		return false;
	}
	return false;
}
LLFloaterIMPanel::LLFloaterIMPanel(
	const std::string& log_label,
	const LLUUID& session_id,
	const LLUUID& other_participant_id,
	const EInstantMessage& dialog,
	const uuid_vec_t& ids) :
	LLFloater(log_label, LLRect(), log_label),
	mStartCallOnInitialize(false),
	mInputEditor(NULL),
	mHistoryEditor(NULL),
	mSessionInitialized(false),
	mSessionStartMsgPos({0, 0}),
	mSessionType(P2P_SESSION),
	mSessionUUID(session_id),
	mLogLabel(log_label),
	mQueuedMsgsForInit(),
	mOtherParticipantUUID(other_participant_id),
	mInitialTargetIDs(ids),
	mDialog(dialog),
	mTyping(false),
	mTypingLineStartIndex(0),
	mOtherTyping(false),
	mOtherTypingName(),
	mNumUnreadMessages(0),
	mSentTypingState(true),
	mShowSpeakersOnConnect(true),
	mDing(false),
	mRPMode(false),
	mTextIMPossible(true),
	mCallBackEnabled(true),
	mSpeakers(NULL),
	mSpeakerPanel(NULL),
	mVoiceChannel(NULL),
	mFirstKeystrokeTimer(),
	mLastKeystrokeTimer()
{
	if (mOtherParticipantUUID.isNull())
	{
		LL_WARNS() << "Other participant is NULL" << LL_ENDL;
	}
	static LLCachedControl<bool> concise_im("UseConciseIMButtons");
	std::string xml_filename = concise_im ? "floater_instant_message_concisebuttons.xml" : "floater_instant_message.xml";
	switch(mDialog)
	{
	case IM_SESSION_GROUP_START:
	case IM_SESSION_INVITE:
	case IM_SESSION_CONFERENCE_START:
		mCommitCallbackRegistrar.add("FlipDing", [=](LLUICtrl*, const LLSD&) { mDing = !mDing; });
		if (gAgent.isInGroup(mSessionUUID))
		{
			static LLCachedControl<bool> concise("UseConciseGroupChatButtons");
			xml_filename = concise ? "floater_instant_message_group_concisebuttons.xml" : "floater_instant_message_group.xml";
			bool support = boost::starts_with(mLogLabel, LLTrans::getString("SHORT_APP_NAME") + ' ');
			mSessionType = support ? SUPPORT_SESSION : GROUP_SESSION;
		}
		else
		{
			static LLCachedControl<bool> concise("UseConciseConferenceButtons");
			xml_filename = concise ? "floater_instant_message_ad_hoc_concisebuttons.xml" : "floater_instant_message_ad_hoc.xml";
			mSessionType = ADHOC_SESSION;
		}
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, mLogLabel, false);
		break;
	default:
		LL_WARNS() << "Unknown session type: " << mDialog << LL_ENDL;
	case IM_NOTHING_SPECIAL:
		mTextIMPossible = LLVoiceClient::getInstance()->isSessionTextIMPossible(mSessionUUID);
		mCallBackEnabled = LLVoiceClient::getInstance()->isSessionCallBackPossible(mSessionUUID);
	case IM_SESSION_P2P_INVITE:
	{
		LLVoiceP2POutgoingCallInterface* outgoing =
			LLVoiceClient::getInstance()->getOutgoingCallInterface();
		if (outgoing)
		{
			mVoiceChannel = new LLVoiceChannelP2P(
				mSessionUUID, mLogLabel, mOtherParticipantUUID, outgoing);
		}
		else
		{
			mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, mLogLabel, true);
		}
		LLAvatarTracker::instance().addParticularFriendObserver(mOtherParticipantUUID, this);
		LLMuteList::instance().addObserver(this);
		mDing = gSavedSettings.getBOOL("LiruNewMessageSoundIMsOn");
		break;
	}
	}
	mSpeakers = new LLIMSpeakerMgr(mVoiceChannel);
	LLUICtrlFactory::getInstance()->buildFloater(this,
								xml_filename,
								&getFactoryMap(),
								FALSE);
	mInputEditor->setEnableLineHistory(TRUE);
	if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
	{
		LLLogChat::loadHistory(mLogLabel, mSessionType == P2P_SESSION ? mOtherParticipantUUID : mSessionUUID, boost::bind(&LLFloaterIMPanel::chatFromLogFile, this, _1, _2));
	}
	if ( !mSessionInitialized )
	{
		const bool p2p_as_adhoc =
			(mDialog == IM_NOTHING_SPECIAL || mDialog == IM_SESSION_P2P_INVITE)
			&& (LLVoiceClient::getInstance()->getOutgoingCallInterface() == nullptr);
		if ( !send_start_session_messages(
				 mSessionUUID,
				 mOtherParticipantUUID,
				 ids,
				 mDialog,
				 p2p_as_adhoc) )
		{
			mSessionInitialized = true;
		}
		else
		{
			LLUIString session_start = sSessionStartString;
			session_start.setArg("[NAME]", getTitle());
			mSessionStartMsgPos.first = mHistoryEditor->getLength();
			addHistoryLine(
				session_start,
				gSavedSettings.getColor4("SystemChatColor"),
				false);
			mSessionStartMsgPos.second = mHistoryEditor->getLength() - mSessionStartMsgPos.first;
		}
	}
}
void LLFloaterIMPanel::onAvatarNameLookup(const LLAvatarName& avatar_name)
{
	setTitle(avatar_name.getNSName());
	const S32& ns(gSavedSettings.getS32("IMNameSystem"));
	std::string title(avatar_name.getNSName(ns));
	if (!ns || ns == 3)
	{
		size_t pos(title.find(" Resident"));
		if (pos != std::string::npos && !gSavedSettings.getBOOL("LiruShowLastNameResident"))
			title.erase(pos, 9);
	}
	setShortTitle(title);
	if (LLMultiFloater* mf = dynamic_cast<LLMultiFloater*>(getParent()))
		mf->updateFloaterTitle(this);
}
LLFloaterIMPanel::~LLFloaterIMPanel()
{
	LLAvatarTracker::instance().removeParticularFriendObserver(mOtherParticipantUUID, this);
	LLMuteList::instance().removeObserver(this);
	delete mSpeakers;
	mSpeakers = NULL;
	if(LLVoiceClient::instanceExists() && mOtherParticipantUUID.notNull())
	{
		switch(mDialog)
		{
			case IM_NOTHING_SPECIAL:
			case IM_SESSION_P2P_INVITE:
				LLVoiceClient::getInstance()->endUserIMSession(mOtherParticipantUUID);
			break;
			default:
			break;
		}
	}
	mVoiceChannel->deactivate();
	delete mVoiceChannel;
	mVoiceChannel = NULL;
}
void add_map_option(LLComboBox& flyout, const std::string& map, const LLUUID& id, U8& did)
{
	flyout.remove(map);
	if (is_agent_mappable(id) && LLAvatarTracker::instance().isBuddyOnline(id))
	{
		flyout.add(map, -2);
		did |= 0x02;
	}
	did |= 0x01;
}
void LLFloaterIMPanel::changed(U32 mask)
{
	if (mask & (REMOVE|ADD|POWERS|ONLINE))
	{
		U8 did(0);
		LLComboBox& flyout = *getChild<LLComboBox>("instant_message_flyout");
		if (mask & POWERS)
		{
			add_map_option(flyout, getString("find on map"), mOtherParticipantUUID, did);
		}
		if (mask & ONLINE)
		{
			if (~did & 0x01)
				add_map_option(flyout, getString("find on map"), mOtherParticipantUUID, did);
		}
		if (mask & (REMOVE|ADD) || did & 0x02)
			rebuildDynamics(&flyout);
	}
}
void LLFloaterIMPanel::onChangeDetailed(const LLMute& mute)
{
	if (mute.mID == mOtherParticipantUUID)
		rebuildDynamics(getChild<LLComboBox>("instant_message_flyout"));
}
BOOL LLFloaterIMPanel::postBuild()
{
	requires<LLLineEditor>("chat_editor");
	requires<LLTextEditor>("im_history");
	if (checkRequirements())
	{
		setTitle(mLogLabel);
		if (mSessionType == P2P_SESSION && LLVoiceClient::getInstance()->isParticipantAvatar(mSessionUUID))
			LLAvatarNameCache::get(mOtherParticipantUUID, boost::bind(&LLFloaterIMPanel::onAvatarNameLookup, this, _2));
		mInputEditor = getChild<LLLineEditor>("chat_editor");
		mInputEditor->setAutoreplaceCallback(boost::bind(&LLAutoReplace::autoreplaceCallback, LLAutoReplace::getInstance(), _1, _2, _3, _4, _5));
		mInputEditor->setFocusReceivedCallback( boost::bind(&LLFloaterIMPanel::onInputEditorFocusReceived, this) );
		mInputEditor->setFocusLostCallback(boost::bind(&LLFloaterIMPanel::setTyping, this, false));
		mInputEditor->setKeystrokeCallback( boost::bind(&LLFloaterIMPanel::onInputEditorKeystroke, this, _1) );
		mInputEditor->setCommitCallback( boost::bind(&LLFloaterIMPanel::onSendMsg,this) );
		mInputEditor->setCommitOnFocusLost( FALSE );
		mInputEditor->setRevertOnEsc( FALSE );
		mInputEditor->setReplaceNewlinesWithSpaces( FALSE );
		mInputEditor->setPassDelete( TRUE );
		if (LLComboBox* flyout = findChild<LLComboBox>("instant_message_flyout"))
		{
			flyout->setCommitCallback(boost::bind(&LLFloaterIMPanel::onFlyoutCommit, this, flyout, _2));
			if (mSessionType == P2P_SESSION)
			{
				if (is_agent_mappable(mOtherParticipantUUID))
					flyout->add(getString("find on map"), -2);
				addDynamics(flyout);
				if (gObjectList.findAvatar(mOtherParticipantUUID))
					flyout->add(getString("focus"), -3);
			}
		}
		if (LLUICtrl* ctrl = findChild<LLUICtrl>("tp_btn"))
			ctrl->setCommitCallback(boost::bind(static_cast<void(*)(const LLUUID&)>(LLAvatarActions::offerTeleport), mOtherParticipantUUID));
		if (LLUICtrl* ctrl = findChild<LLUICtrl>("pay_btn"))
			ctrl->setCommitCallback(boost::bind(LLAvatarActions::pay, mOtherParticipantUUID));
		if (LLButton* btn = findChild<LLButton>("group_info_btn"))
			btn->setCommitCallback(boost::bind(LLGroupActions::show, mSessionUUID));
		if (LLUICtrl* ctrl = findChild<LLUICtrl>("history_btn"))
			ctrl->setCommitCallback(boost::bind(&LLFloaterIMPanel::onClickHistory, this));
		getChild<LLButton>("start_call_btn")->setCommitCallback(boost::bind(&LLIMMgr::startCall, gIMMgr, boost::ref(mSessionUUID), LLVoiceChannel::OUTGOING_CALL));
		getChild<LLButton>("end_call_btn")->setCommitCallback(boost::bind(&LLIMMgr::endCall, gIMMgr, boost::ref(mSessionUUID)));
		getChild<LLButton>("send_btn")->setCommitCallback(boost::bind(&LLFloaterIMPanel::onSendMsg,this));
		if (LLButton* btn = findChild<LLButton>("toggle_active_speakers_btn"))
			btn->setCommitCallback(boost::bind(&LLView::setVisible, getChildView("active_speakers_panel"), _2));
		if (LLCheckBoxCtrl* check = findChild<LLCheckBoxCtrl>("im_translate_check"))
		{
			syncTranslationCheckbox();
			check->setCommitCallback(boost::bind(&LLFloaterIMPanel::onTranslateCheck, this));
		}
		if (LLButton* btn = findChild<LLButton>("im_translate_btn"))
			btn->setCommitCallback(boost::bind(&LLFloaterIMPanel::onTranslateConfig, this));
		mHistoryEditor = getChild<LLViewerTextEditor>("im_history");
		mHistoryEditor->setParseHTML(TRUE);
		sTitleString = getString("title_string");
		sTypingStartString = getString("typing_start_string");
		sSessionStartString = getString("session_start_string");
		if (mSpeakerPanel)
		{
			mSpeakerPanel->refreshSpeakers();
		}
		switch (mSessionType)
		{
		case P2P_SESSION:
		{
			getChild<LLUICtrl>("mute_btn")->setCommitCallback(boost::bind(&LLFloaterIMPanel::onClickMuteVoice, this));
			getChild<LLUICtrl>("speaker_volume")->setCommitCallback(boost::bind(&LLVoiceClient::setUserVolume, LLVoiceClient::getInstance(), mOtherParticipantUUID, _2));
		}
		break;
		case SUPPORT_SESSION:
		{
			auto support = getChildView("Support Check");
			support->setVisible(true);
			auto control = gSavedSettings.getControl(support->getControlName());
			if (control->get().asInteger() == -1)
			{
				LLNotificationsUtil::add("SupportChatShowInfo", LLSD(), LLSD(), [control](const LLSD& p, const LLSD& f)
				{
					control->set(!LLNotificationsUtil::getSelectedOption(p, f));
				});
			}
		}
		break;
		default:
		break;
		}
		setDefaultBtn("send_btn");
		mVolumeSlider.connect(this,"speaker_volume");
		mEndCallBtn.connect(this,"end_call_btn");
		mStartCallBtn.connect(this,"start_call_btn");
		mSendBtn.connect(this,"send_btn");
		mMuteBtn.connect(this,"mute_btn");
		return TRUE;
	}
	return FALSE;
}
void* LLFloaterIMPanel::createSpeakersPanel(void* data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)data;
	floaterp->mSpeakerPanel = new LLParticipantList(floaterp->mSpeakers, true);
	return floaterp->mSpeakerPanel;
}
void LLFloaterIMPanel::onClickMuteVoice()
{
	LLMute mute(mOtherParticipantUUID, getTitle(), LLMute::AGENT);
	if (!LLMuteList::getInstance()->isMuted(mOtherParticipantUUID, LLMute::flagVoiceChat))
	{
		LLMuteList::getInstance()->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		LLMuteList::getInstance()->remove(mute, LLMute::flagVoiceChat);
	}
}
void LLFloaterIMPanel::draw()
{
	bool enable_connect = mSessionInitialized && LLVoiceClient::getInstance()->voiceEnabled() && mCallBackEnabled;
	if (mEndCallBtn)
	{
		mEndCallBtn->setVisible(LLVoiceClient::getInstance()->voiceEnabled() && mVoiceChannel && mVoiceChannel->getState() >= LLVoiceChannel::STATE_CALL_STARTED);
	}
	if (mStartCallBtn)
	{
		mStartCallBtn->setVisible(LLVoiceClient::getInstance()->voiceEnabled() && mVoiceChannel && mVoiceChannel->getState() < LLVoiceChannel::STATE_CALL_STARTED);
		mStartCallBtn->setEnabled(enable_connect);
	}
	if (mSendBtn && mInputEditor)
	{
		mSendBtn->setEnabled(!mInputEditor->getValue().asString().empty());
	}
	LLPointer<LLSpeaker> self_speaker = mSpeakers->findSpeaker(gAgent.getID());
	if(!mTextIMPossible)
	{
		mInputEditor->setEnabled(FALSE);
		mInputEditor->setLabel(getString("unavailable_text_label"));
	}
	else if (self_speaker.notNull() && self_speaker->mModeratorMutedText)
	{
		mInputEditor->setEnabled(FALSE);
		mInputEditor->setLabel(getString("muted_text_label"));
	}
	else
	{
		mInputEditor->setEnabled(TRUE);
		mInputEditor->setLabel(getString("default_text_label"));
	}
	if (mShowSpeakersOnConnect && mVoiceChannel->isActive())
	{
		if (mSpeakerPanel) mSpeakerPanel->setVisible(true);
		mShowSpeakersOnConnect = false;
	}
	if (mTyping)
	{
		if (mLastKeystrokeTimer.getElapsedTimeF32() > LLAgent::TYPING_TIMEOUT_SECS)
		{
			setTyping(false);
		}
		if (!mSentTypingState
			&& mFirstKeystrokeTimer.getElapsedTimeF32() > 1.f)
		{
			sendTypingState(true);
			mSentTypingState = true;
		}
	}
	if (mSpeakerPanel)
	{
		if (mSpeakerPanel->getVisible())
		{
			mSpeakerPanel->refreshSpeakers();
		}
	}
	else
	{
		mVolumeSlider->setVisible(LLVoiceClient::getInstance()->voiceEnabled() && mVoiceChannel->isActive());
		mVolumeSlider->setValue(LLVoiceClient::getInstance()->getUserVolume(mOtherParticipantUUID));
		mMuteBtn->setValue(LLMuteList::getInstance()->isMuted(mOtherParticipantUUID, LLMute::flagVoiceChat));
		mMuteBtn->setVisible(LLVoiceClient::getInstance()->voiceEnabled() && mVoiceChannel->isActive());
	}
	LLFloater::draw();
}
class LLSessionInviteResponder : public LLHTTPClient::ResponderIgnoreBody
{
public:
	LLSessionInviteResponder(const LLUUID& session_id) : mSessionID(session_id) {}
	void httpFailure()
	{
		LL_WARNS() << "Error inviting all agents to session [status:" << mStatus << "]: " << mReason << LL_ENDL;
	}
	char const* getName() const { return "LLSessionInviteResponder"; }
private:
	LLUUID mSessionID;
};
bool LLFloaterIMPanel::inviteToSession(const uuid_vec_t& ids)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
	{
		return FALSE;
	}
	S32 count = ids.size();
	if( isInviteAllowed() && (count > 0) )
	{
		LL_INFOS() << "LLFloaterIMPanel::inviteToSession() - inviting participants" << LL_ENDL;
		std::string url = region->getCapability("ChatSessionRequest");
		LLSD data;
		data["params"] = LLSD::emptyArray();
		for (int i = 0; i < count; i++)
		{
			data["params"].append(ids.at(i));
		}
		data["method"] = "invite";
		data["session-id"] = mSessionUUID;
		LLHTTPClient::post(
			url,
			data,
			new LLSessionInviteResponder(
				mSessionUUID));
	}
	else
	{
		LL_INFOS() << "LLFloaterIMPanel::inviteToSession -"
				<< " no need to invite agents for "
				<< mDialog << LL_ENDL;
	}
	return TRUE;
}
void LLFloaterIMPanel::addHistoryLine(const std::string &utf8msg, LLColor4 incolor, bool log_to_file, const LLUUID& source, const std::string& name)
{
	bool is_agent(gAgentID == source), from_user(source.notNull());
	if (!is_agent)
	{
		static const LLCachedControl<bool> mKeywordsChangeColor(gSavedPerAccountSettings, "KeywordsChangeColor", false);
		if (mKeywordsChangeColor)
		{
			if (AscentKeyword::hasKeyword(utf8msg, 2))
			{
				static const LLCachedControl<LLColor4> mKeywordsColor(gSavedPerAccountSettings, "KeywordsColor", LLColor4(1.f, 1.f, 1.f, 1.f));
				incolor = mKeywordsColor;
			}
		}
		bool focused(hasFocus());
		if (mDing && (!focused || !gFocusMgr.getAppHasFocus()))
		{
			static const LLCachedControl<std::string> ding("LiruNewMessageSound");
			static const LLCachedControl<std::string> dong("LiruNewMessageSoundForSystemMessages");
			LLUI::sAudioCallback(LLUUID(from_user ? ding : dong));
		}
		LLMultiFloater* host = getHost();
		const bool conversation_hidden = !isInVisibleChain();
		if (from_user && (conversation_hidden || (!host && focused)))
		{
			++mNumUnreadMessages;
		}
		if (from_user && conversation_hidden && host)
		{
			host->setFloaterFlashing(this, true);
			host->setFloaterUnreadCount(this, mNumUnreadMessages);
		}
	}
	removeTypingIndicator(source);
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("IMShowTimestamps"))
	{
		mHistoryEditor->appendTime(prepend_newline);
		prepend_newline = false;
	}
	std::string show_name = name;
	bool is_irc = false;
	bool system = name.empty();
	if (!system)
	{
		if (name == SYSTEM_FROM)
		{
			system = true;
			mHistoryEditor->appendColoredText(name,false,prepend_newline,incolor);
		}
		else
		{
			system = !from_user;
			static const LLCachedControl<bool> italicize("LiruItalicizeActions");
			is_irc = italicize && utf8msg[0] != ':';
			if (from_user)
				LLAvatarNameCache::getNSName(source, show_name);
			LLStyleSP source_style = LLStyleMap::instance().lookupAgent(source);
			source_style->mItalic = is_irc;
			mHistoryEditor->appendText(show_name,false,prepend_newline,source_style, system);
		}
		prepend_newline = false;
	}
	{
		LLStyleSP style(new LLStyle);
		style->setColor(incolor);
		style->mItalic = is_irc;
		style->mBold = from_user && gSavedSettings.getBOOL("AllynBoldGroupModerator") && isModerator(source);
		mHistoryEditor->appendText(utf8msg, false, prepend_newline, style, system);
	}
	if (log_to_file
		&& gSavedPerAccountSettings.getBOOL("LogInstantMessages") )
	{
		std::string histstr;
		if (gSavedPerAccountSettings.getBOOL("IMLogTimestamp"))
			histstr = LLLogChat::timestamp(gSavedPerAccountSettings.getBOOL("LogTimestampDate")) + show_name + utf8msg;
		else
			histstr = show_name + utf8msg;
		LLLogChat::saveHistory(mLogLabel, mSessionType == P2P_SESSION ? mOtherParticipantUUID : mSessionUUID, histstr);
	}
	if (from_user)
	{
		mSpeakers->speakerChatted(source);
		mSpeakers->setSpeakerTyping(source, FALSE);
	}
}
void LLFloaterIMPanel::setVisible(BOOL b)
{
	LLPanel::setVisible(b);
	LLMultiFloater* hostp = getHost();
	if( b && hostp )
	{
		mNumUnreadMessages = 0;
		hostp->setFloaterUnreadCount(this, 0);
		hostp->setFloaterFlashing(this, FALSE);
	}
}
void LLFloaterIMPanel::setInputFocus(bool b)
{
	mInputEditor->setFocus( b );
}
BOOL LLFloaterIMPanel::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;
	if (KEY_RETURN == key)
	{
		onSendMsg();
		handled = TRUE;
		if (mask == MASK_NONE)
			closeIfNotPinned();
	}
	else if (KEY_ESCAPE == key && mask == MASK_NONE)
	{
		handled = TRUE;
		gFocusMgr.setKeyboardFocus(NULL);
		closeIfNotPinned();
	}
	return handled;
}
void LLFloaterIMPanel::closeIfNotPinned()
{
	if (gSavedSettings.getBOOL("PinTalkViewOpen")) return;
	if (getParent() == gFloaterView)
		setMinimized(true);
	else
		gIMMgr->toggle();
}
BOOL LLFloaterIMPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								  EDragAndDropType cargo_type,
								  void* cargo_data,
								  EAcceptance* accept,
								  std::string& tooltip_msg)
{
	if (mSessionType == P2P_SESSION)
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mOtherParticipantUUID, mSessionUUID, drop,
												 cargo_type, cargo_data, accept);
	}
	else if (isInviteAllowed())
	{
		*accept = ACCEPT_NO;
		if (cargo_type == DAD_CALLINGCARD)
		{
			if (dropCallingCard((LLInventoryItem*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
		else if (cargo_type == DAD_CATEGORY)
		{
			if (dropCategory((LLInventoryCategory*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
	}
	return TRUE;
}
BOOL LLFloaterIMPanel::dropCallingCard(LLInventoryItem* item, BOOL drop)
{
	if (item && item->getCreatorUUID().notNull())
	{
		if (drop)
		{
			inviteToSession({ item->getCreatorUUID() });
		}
		return true;
	}
	return false;
}
BOOL LLFloaterIMPanel::dropCategory(LLInventoryCategory* category, BOOL drop)
{
	if (category)
	{
		LLInventoryModel::cat_array_t cats;
		LLInventoryModel::item_array_t items;
		LLUniqueBuddyCollector buddies;
		gInventory.collectDescendentsIf(category->getUUID(),
										cats,
										items,
										LLInventoryModel::EXCLUDE_TRASH,
										buddies);
		S32 count = items.size();
		if(count == 0)
		{
			return false;
		}
		else if(drop)
		{
			uuid_vec_t ids;
			for(S32 i = 0; i < count; ++i)
			{
				ids.push_back(items.at(i)->getCreatorUUID());
			}
			inviteToSession(ids);
		}
	}
	return true;
}
bool LLFloaterIMPanel::isInviteAllowed() const
{
	return mSessionType == ADHOC_SESSION;
}
void LLFloaterIMPanel::onAddButtonClicked()
{
	LLView * button = findChild<LLButton>("instant_message_flyout");
	LLFloater* root_floater = gFloaterView->getParentFloater(this);
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterIMPanel::addSessionParticipants, this, _1), TRUE, TRUE, FALSE, root_floater->getName(), button);
	if (!picker)
	{
		return;
	}
	picker->setOkBtnEnableCb(boost::bind(&LLFloaterIMPanel::canAddSelectedToChat, this, _1));
	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}
}
bool LLFloaterIMPanel::canAddSelectedToChat(const uuid_vec_t& uuids) const
{
	switch (mSessionType)
	{
	case P2P_SESSION: return true;
	case ADHOC_SESSION:
	{
		LLSpeakerMgr::speaker_list_t speaker_list;
		LLIMSpeakerMgr* speaker_mgr = getSpeakerManager();
		if (speaker_mgr)
		{
			speaker_mgr->getSpeakerList(&speaker_list, true);
		}
		for (const auto& id : uuids)
			for (const LLPointer<LLSpeaker>& speaker : speaker_list)
				if (id == speaker->mID)
					return false;
	}
	return true;
	default: return false;
	}
}
void LLFloaterIMPanel::addSessionParticipants(const uuid_vec_t& uuids)
{
	if (mSessionType == P2P_SESSION)
	{
		LLSD payload;
		LLSD args;
		LLNotificationsUtil::add("ConfirmAddingChatParticipants", args, payload,
				boost::bind(&LLFloaterIMPanel::addP2PSessionParticipants, this, _1, _2, uuids));
	}
	else inviteToSession(uuids);
}
void LLFloaterIMPanel::addP2PSessionParticipants(const LLSD& notification, const LLSD& response, const uuid_vec_t& uuids)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)
	{
		return;
	}
	LLVoiceChannel* voice_channel = LLActiveSpeakerMgr::getInstance()->getVoiceChannel();
	bool is_voice_call = voice_channel != nullptr && voice_channel->getSessionID() == mSessionUUID && voice_channel->isActive();
	uuid_vec_t temp_ids;
	temp_ids.push_back(mOtherParticipantUUID);
	temp_ids.insert(temp_ids.end(), uuids.begin(), uuids.end());
	if (is_voice_call)
	{
		LLAvatarActions::startAdhocCall(temp_ids);
	}
	else
	{
		LLAvatarActions::startConference(temp_ids);
	}
}
void LLFloaterIMPanel::removeDynamics(LLComboBox* flyout)
{
	flyout->remove(mDing ? getString("ding on") : getString("ding off"));
	flyout->remove(mRPMode ? getString("rp mode on") : getString("rp mode off"));
	flyout->remove(LLAvatarActions::isFriend(mOtherParticipantUUID) ? getString("remove friend") : getString("add friend"));
	flyout->remove(LLAvatarActions::isBlocked(mOtherParticipantUUID) ? getString("unmute") : getString("mute"));
}
void LLFloaterIMPanel::addDynamics(LLComboBox* flyout)
{
	flyout->add(mDing ? getString("ding on") : getString("ding off"), 6);
	flyout->add(mRPMode ? getString("rp mode on") : getString("rp mode off"), 7);
	flyout->add(LLAvatarActions::isFriend(mOtherParticipantUUID) ? getString("remove friend") : getString("add friend"), 8);
	flyout->add(LLAvatarActions::isBlocked(mOtherParticipantUUID) ? getString("unmute") : getString("mute"), 9);
}
void LLFloaterIMPanel::addDynamicFocus()
{
	findChild<LLComboBox>("instant_message_flyout")->add(getString("focus"), -3);
}
void LLFloaterIMPanel::removeDynamicFocus()
{
	findChild<LLComboBox>("instant_message_flyout")->remove(getString("focus"));
}
void copy_profile_uri(const LLUUID& id, const LFIDBearer::Type& type = LFIDBearer::AVATAR);
void LLFloaterIMPanel::onFlyoutCommit(LLComboBox* flyout, const LLSD& value)
{
	if (value.isUndefined() || value.asInteger() == 0)
	{
		switch (mSessionType)
		{
			case SUPPORT_SESSION:
			case GROUP_SESSION: LLGroupActions::show(mSessionUUID); return;
			case P2P_SESSION: LLAvatarActions::showProfile(mOtherParticipantUUID); return;
			default: onClickHistory(); return;
		}
	}
	switch (int option = value.asInteger())
	{
	case 1:	onClickHistory(); break;
	case 2: LLAvatarActions::offerTeleport(mOtherParticipantUUID); break;
	case 3: LLAvatarActions::teleportRequest(mOtherParticipantUUID); break;
	case 4: LLAvatarActions::pay(mOtherParticipantUUID); break;
	case 5: LLAvatarActions::inviteToGroup(mOtherParticipantUUID); break;
	case -1: copy_profile_uri(mOtherParticipantUUID); break;
	case -2: LLAvatarActions::showOnMap(mOtherParticipantUUID); break;
	case -3: gAgentCamera.lookAtObject(mOtherParticipantUUID); break;
	case -4: onAddButtonClicked(); break;
	case -5: LLFloaterReporter::showFromAvatar(mOtherParticipantUUID, mLogLabel); break;
	default:
	{
		removeDynamics(flyout);
		switch (option)
		{
			case 6: mDing = !mDing; break;
			case 7: mRPMode = !mRPMode; break;
			case 8: LLAvatarActions::isFriend(mOtherParticipantUUID) ? LLAvatarActions::removeFriendDialog(mOtherParticipantUUID) : LLAvatarActions::requestFriendshipDialog(mOtherParticipantUUID); break;
			case 9: LLAvatarActions::toggleBlock(mOtherParticipantUUID); break;
		}
		addDynamics(flyout);
		const std::string focus(getString("focus"));
		if (flyout->remove(focus))
			flyout->add(focus, -3);
	}
	}
}
void show_log_browser(const std::string& name, const LLUUID& id)
{
	const std::string file(LLLogChat::makeLogFileName(name, id));
	if (!LLFile::isfile(file))
	{
		make_ui_sound("UISndBadKeystroke");
		LL_WARNS() << "File not present: " << file << LL_ENDL;
		return;
	}
	if (gSavedSettings.getBOOL("LiruLegacyLogLaunch"))
	{
		if (!LLWindow::ShellEx(file))
			return;
	}
	LLFloaterWebContent::Params p;
	p.url("file:///" + file);
	p.id(id.asString());
	p.show_chrome(false);
	p.trusted_content(true);
	LLFloaterWebContent::showInstance("log", p);
}
void LLFloaterIMPanel::onClickHistory()
{
	if (mOtherParticipantUUID.notNull())
	{
		show_log_browser(mLogLabel, mSessionType == P2P_SESSION ? mOtherParticipantUUID : mSessionUUID);
	}
}
void LLFloaterIMPanel::onInputEditorFocusReceived()
{
	if (gSavedSettings.getBOOL("LiruLegacyScrollToEnd"))
		mHistoryEditor->setCursorAndScrollToEnd();
}
void LLFloaterIMPanel::onInputEditorKeystroke(LLLineEditor* caller)
{
	setTyping(!caller->getText().empty());
}
void leave_group_chat(const LLUUID& from_id, const LLUUID& session_id);
void LLFloaterIMPanel::onClose(bool app_quitting)
{
	setTyping(false);
	leave_group_chat(mOtherParticipantUUID, mSessionUUID);
	destroy();
}
void LLFloaterIMPanel::handleVisibilityChange(BOOL new_visibility)
{
	if (new_visibility)
	{
		mNumUnreadMessages = 0;
		if (LLMultiFloater* hostp = getHost())
		{
			hostp->setFloaterUnreadCount(this, 0);
			hostp->setFloaterFlashing(this, FALSE);
		}
	}
}
void deliver_message(const std::string& utf8_text,
					 const LLUUID& im_session_id,
					 const LLUUID& other_participant_id,
					 EInstantMessage dialog)
{
	std::string name;
	bool sent = false;
	LLAgentUI::buildFullname(name);
	const LLRelationship* info = LLAvatarTracker::instance().getBuddyInfo(other_participant_id);
	U8 offline = (!info || info->isOnline()) ? IM_ONLINE : IM_OFFLINE;
	if((offline == IM_OFFLINE) && (LLVoiceClient::getInstance()->isOnlineSIP(other_participant_id)))
	{
	}
	if(!sent)
	{
		U8 new_dialog = dialog;
		if ( dialog != IM_NOTHING_SPECIAL )
		{
			new_dialog = IM_SESSION_SEND;
		}
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			other_participant_id,
			name.c_str(),
			utf8_text.c_str(),
			offline,
			(EInstantMessage)new_dialog,
			im_session_id);
		gAgent.sendReliableMessage();
	}
	if ( LLMuteList::getInstance() )
	{
		switch( dialog )
		{
		case IM_NOTHING_SPECIAL:
		case IM_GROUP_INVITATION:
		case IM_INVENTORY_OFFERED:
		case IM_SESSION_INVITE:
		case IM_SESSION_P2P_INVITE:
		case IM_SESSION_CONFERENCE_START:
		case IM_SESSION_SEND:
		case IM_LURE_USER:
		case IM_GODLIKE_LURE_USER:
		case IM_FRIENDSHIP_OFFERED:
			LLMuteList::getInstance()->autoRemove(other_participant_id, LLMuteList::AR_IM);
			break;
		default: ;
		}
	}
}
bool convert_roleplay_text(std::string& text);
void LLFloaterIMPanel::syncTranslationCheckbox()
{
	LLCheckBoxCtrl* check = findChild<LLCheckBoxCtrl>("im_translate_check");
	if (!check)
		return;
	bool enabled = false;
	bool incoming = true;
	bool outgoing = true;
	std::string source, target;
	LLChatAITranslate::instance().ensureSessionConfig(this, enabled, source, target, incoming, outgoing);
	check->set(enabled);
}
void LLFloaterIMPanel::onTranslateCheck()
{
	LLCheckBoxCtrl* check = findChild<LLCheckBoxCtrl>("im_translate_check");
	if (!check)
		return;
	bool unused_enabled = false;
	bool incoming = true;
	bool outgoing = true;
	std::string source, target;
	LLChatAITranslate::instance().ensureSessionConfig(this, unused_enabled, source, target, incoming, outgoing);
	const LLUUID key = LLChatAITranslate::instance().sessionConfigKey(this);
	LLChatAITranslate::instance().setSessionTranslation(key, check->get(), source, target, incoming, outgoing);
}
void LLFloaterIMPanel::onTranslateConfig()
{
	LLFloaterIMTranslationConfig::show(this);
}
void LLFloaterIMPanel::onSendMsg()
{
	if (!gAgent.isGodlike()
		&& (mSessionType == P2P_SESSION)
		&& mOtherParticipantUUID.isNull())
	{
		LL_INFOS() << "Cannot send IM to everyone unless you're a god." << LL_ENDL;
		return;
	}
	if (mInputEditor)
	{
		LLWString text = mInputEditor->getConvertedText();
		if(!text.empty())
		{
			if (mInputEditor) mInputEditor->updateHistory();
			std::string utf8_text = wstring_to_utf8str(text);
			bool action = convert_roleplay_text(utf8_text);
			if (!action && mRPMode)
				utf8_text = "((" + utf8_text + "))";
			if ( (RlvActions::hasBehaviour(RLV_BHVR_SENDIM)) || (RlvActions::hasBehaviour(RLV_BHVR_SENDIMTO)) )
			{
				bool fRlvFilter = false;
				switch (mSessionType)
				{
					case P2P_SESSION:
						fRlvFilter = !RlvActions::canSendIM(mOtherParticipantUUID);
						break;
					case GROUP_SESSION:
						fRlvFilter = !RlvActions::canSendIM(mSessionUUID);
						break;
					case SUPPORT_SESSION:
						break;
					case ADHOC_SESSION:
						{
							if (!mSpeakers)
							{
								fRlvFilter = true;
								break;
							}
							LLSpeakerMgr::speaker_list_t speakers;
							mSpeakers->getSpeakerList(&speakers, TRUE);
							for (LLSpeakerMgr::speaker_list_t::const_iterator itSpeaker = speakers.begin();
									itSpeaker != speakers.end(); ++itSpeaker)
							{
								const LLSpeaker* pSpeaker = *itSpeaker;
								if ( (gAgentID != pSpeaker->mID) && (!RlvActions::canSendIM(pSpeaker->mID)) )
								{
									fRlvFilter = true;
									break;
								}
							}
						}
						break;
					default:
						fRlvFilter = true;
						break;
				}
				if (fRlvFilter)
				{
					utf8_text = RlvStrings::getString(RLV_STRING_BLOCKED_SENDIM);
				}
			}
			if (mSessionType == SUPPORT_SESSION && getChildView("Support Check")->getValue())
			{
				utf8_text.insert(action ? 3 : 0, llformat(action ? " (%d%s)" : "(%d%s): ", LL_VIEWER_VERSION_BUILD, wstring_to_utf8str(LL_VIEWER_CHANNEL_GRK).data()));
			}
			if ( mSessionInitialized )
			{
				if (LLChatAITranslate::instance().maybeDeferOutgoingIM(this, utf8_text, action))
				{
				}
				else
				{
				U32 split = MAX_MSG_BUF_SIZE - 1;
				U32 pos = 0;
				U32 total = utf8_text.length();
				while (pos < total)
				{
					U32 next_split = split;
					if (pos + next_split > total)
					{
						next_split = total - pos;
					}
					else
					{
						while (U8(utf8_text[pos + next_split]) != 0x20
							&& U8(utf8_text[pos + next_split]) != 0x21
							&& U8(utf8_text[pos + next_split]) != 0x2C
							&& U8(utf8_text[pos + next_split]) != 0x2E
							&& U8(utf8_text[pos + next_split]) != 0x3F
							&& next_split > 0)
						{
							--next_split;
						}
						if (next_split == 0)
						{
							next_split = split;
							LL_WARNS("Splitting") << "utf-8 couldn't be split correctly" << LL_ENDL;
						}
						else
						{
							++next_split;
						}
					}
					std::string send = utf8_text.substr(pos, next_split);
					pos += next_split;
					LL_DEBUGS("Splitting") << "Pos: " << pos << " next_split: " << next_split << LL_ENDL;
					deliver_message(send,
									mSessionUUID,
									mOtherParticipantUUID,
									mDialog);
				}
				if((mSessionType == P2P_SESSION) &&
				   (mOtherParticipantUUID.notNull()))
				{
					std::string name;
					LLAgentUI::buildFullname(name);
					if (action)
					{
						utf8_text.erase(0,3);
					}
					else
					{
						utf8_text.insert(0, ": ");
					}
					bool other_was_typing = mOtherTyping;
					addHistoryLine(utf8_text, gSavedSettings.getColor("UserChatColor"), true, gAgentID, name);
					if (other_was_typing) addTypingIndicator(mOtherParticipantUUID);
				}
				}
			}
			else
			{
				mQueuedMsgsForInit.append(utf8_text);
			}
		}
		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_IM_COUNT);
		mInputEditor->setText(LLStringUtil::null);
	}
	mTyping = false;
	mSentTypingState = true;
}
void LLFloaterIMPanel::processSessionUpdate(const LLSD& session_update)
{
	if (
		session_update.has("moderated_mode") &&
		session_update["moderated_mode"].has("voice") )
	{
		bool voice_moderated = session_update["moderated_mode"]["voice"].asBoolean();
		if (voice_moderated)
		{
			setTitle(mLogLabel + std::string(" ") + getString("moderated_chat_label"));
		}
		else
		{
			setTitle(mLogLabel);
		}
		if (mSpeakerPanel)
		{
			mSpeakerPanel->setVoiceModerationCtrlMode(voice_moderated);
		}
		if (mSpeakers)
		{
			mSpeakers->processSessionUpdate(session_update);
		}
	}
}
void LLFloaterIMPanel::sessionInitReplyReceived(const LLUUID& session_id)
{
	mSessionUUID = session_id;
	mVoiceChannel->updateSessionID(session_id);
	mSessionInitialized = true;
	mHistoryEditor->remove(mSessionStartMsgPos.first, mSessionStartMsgPos.second, true);
	for (LLSD::array_iterator iter = mQueuedMsgsForInit.beginArray();
		  iter != mQueuedMsgsForInit.endArray();
		  ++iter)
	{
		deliver_message(
			iter->asString(),
			mSessionUUID,
			mOtherParticipantUUID,
			mDialog);
	}
	if (mStartCallOnInitialize)
	{
		gIMMgr->startCall(mSessionUUID);
	}
}
void LLFloaterIMPanel::setTyping(bool typing)
{
	if (typing)
	{
		mLastKeystrokeTimer.reset();
		if (!mTyping)
		{
			mFirstKeystrokeTimer.reset();
			mSentTypingState = false;
		}
		mSpeakers->setSpeakerTyping(gAgent.getID(), TRUE);
	}
	else
	{
		if (mTyping)
		{
			sendTypingState(false);
			mSentTypingState = true;
		}
		mSpeakers->setSpeakerTyping(gAgent.getID(), FALSE);
	}
	mTyping = typing;
}
void LLFloaterIMPanel::sendTypingState(bool typing)
{
	if(gSavedSettings.getBOOL("AscentHideTypingNotification"))
		return;
	if (mSessionType != P2P_SESSION) return;
	std::string name;
	LLAgentUI::buildFullname(name);
	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		mOtherParticipantUUID,
		name,
		std::string("typing"),
		IM_ONLINE,
		(typing ? IM_TYPING_START : IM_TYPING_STOP),
		mSessionUUID);
	gAgent.sendReliableMessage();
}
void LLFloaterIMPanel::processIMTyping(const LLUUID& from_id, BOOL typing)
{
	if (typing)
	{
		addTypingIndicator(from_id);
	}
	else
	{
		removeTypingIndicator(from_id);
	}
}
void LLFloaterIMPanel::addTypingIndicator(const LLUUID& from_id)
{
	if (from_id.notNull() && !mOtherTyping)
	{
		mOtherTyping = true;
		LLAvatarNameCache::getNSName(from_id, mOtherTypingName);
		mTypingLineStartIndex = mHistoryEditor->getWText().length();
		LLUIString typing_start = sTypingStartString;
		typing_start.setArg("[NAME]", mOtherTypingName);
		addHistoryLine(typing_start, gSavedSettings.getColor4("SystemChatColor"), false);
		LLIMSpeakerMgr* speaker_mgr = mSpeakers;
		if ( speaker_mgr )
		{
			speaker_mgr->setSpeakerTyping(from_id, TRUE);
		}
		mOtherTyping = true;
	}
}
void LLFloaterIMPanel::removeTypingIndicator(const LLUUID& from_id)
{
	if (mOtherTyping)
	{
		mOtherTyping = false;
		S32 chars_to_remove = mHistoryEditor->getWText().length() - mTypingLineStartIndex;
		mHistoryEditor->removeTextFromEnd(chars_to_remove);
		if (from_id.notNull())
		{
			mSpeakers->setSpeakerTyping(from_id, FALSE);
		}
	}
}
void LLFloaterIMPanel::chatFromLogFile(LLLogChat::ELogLineType type, const std::string& line)
{
	bool log_line = type == LLLogChat::LOG_LINE;
	if (log_line || gSavedPerAccountSettings.getBOOL("LogInstantMessages"))
	{
		LLStyleSP style(new LLStyle(true, gSavedSettings.getColor4("LogChatColor"), LLStringUtil::null));
		mHistoryEditor->appendText(log_line ? line :
			getString(type == LLLogChat::LOG_END ? "IM_end_log_string" : "IM_logging_string"),
			false, true, style, false);
	}
}
void LLFloaterIMPanel::showSessionStartError(
	const std::string& error_string)
{
	LLSD args;
	args["REASON"] = LLTrans::getString(error_string);
	args["RECIPIENT"] = getTitle();
	LLSD payload;
	payload["session_id"] = mSessionUUID;
	LLNotifications::instance().add(
		"ChatterBoxSessionStartError",
		args,
		payload,
		onConfirmForceCloseError);
}
void LLFloaterIMPanel::showSessionEventError(
	const std::string& event_string,
	const std::string& error_string)
{
	LLSD args;
	LLStringUtil::format_map_t event_args;
	event_args["RECIPIENT"] = getTitle();
	args["REASON"] =
		LLTrans::getString(error_string);
	args["EVENT"] =
		LLTrans::getString(event_string, event_args);
	LLNotifications::instance().add(
		"ChatterBoxSessionEventError",
		args);
}
void LLFloaterIMPanel::showSessionForceClose(
	const std::string& reason_string)
{
	LLSD args;
	args["NAME"] = getTitle();
	args["REASON"] = LLTrans::getString(reason_string);
	LLSD payload;
	payload["session_id"] = mSessionUUID;
	LLNotifications::instance().add(
		"ForceCloseChatterBoxSession",
		args,
		payload,
		LLFloaterIMPanel::onConfirmForceCloseError);
}
bool LLFloaterIMPanel::onConfirmForceCloseError(const LLSD& notification, const LLSD& response)
{
	LLUUID session_id = notification["payload"]["session_id"];
	if (gIMMgr)
	{
		LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(session_id);
		if (floaterp) floaterp->close(FALSE);
	}
	return false;
}
const bool LLFloaterIMPanel::isModerator(const LLUUID& speaker_id)
{
	if (mSpeakers)
	{
		LLPointer<LLSpeaker> speakerp = mSpeakers->findSpeaker(speaker_id);
		return speakerp && speakerp->mIsModerator;
	}
	return false;
}
BOOL LLFloaterIMPanel::focusFirstItem(BOOL prefer_text_fields, BOOL focus_flash )
{
	if (getVisible() && mInputEditor->getVisible())
	{
		setInputFocus(true);
		return TRUE;
	}
	return LLUICtrl::focusFirstItem(prefer_text_fields, focus_flash);
}
void LLFloaterIMPanel::onFocusReceived()
{
	mNumUnreadMessages = 0;
	if (LLMultiFloater* hostp = getHost())
	{
		hostp->setFloaterUnreadCount(this, 0);
		hostp->setFloaterFlashing(this, FALSE);
	}
	if (getVisible() && mInputEditor->getVisible())
	{
		setInputFocus(true);
	}
	LLFloater::onFocusReceived();
}
