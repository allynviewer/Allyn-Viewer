/**
 * @file llspeakers.cpp
 * @brief Management interface for muting and controlling volume of residents currently speaking
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "llspeakers.h"
#include "llagent.h"
#include "llavatarnamecache.h"
#include "llimpanel.h"
#include "llimview.h"
#include "llgroupmgr.h"
#include "llsdutil.h"
#include "llui.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llvoicechannel.h"
#include "llworld.h"
#include "rlvhandler.h"
const LLColor4 INACTIVE_COLOR(0.3f, 0.3f, 0.3f, 0.5f);
const LLColor4 ACTIVE_COLOR(0.5f, 0.5f, 0.5f, 1.f);
LLSpeaker::LLSpeaker(const speaker_entry_t& entry) :
	mID(entry.id),
	mStatus(entry.status),
	mType(entry.type),
	mIsModerator(entry.moderator != boost::none && *entry.moderator),
	mModeratorMutedText(entry.moderator_muted_text != boost::none && *entry.moderator_muted_text),
	mModeratorMutedVoice(FALSE),
	mLastSpokeTime(0.f),
	mSpeechVolume(0.f),
	mHasSpoken(FALSE),
	mHasLeftCurrentCall(FALSE),
	mDotColor(LLColor4::white),
	mTyping(FALSE),
	mSortIndex(0),
	mDisplayName(entry.name),
	mNeedsResort(true)
{
	if (mType == SPEAKER_AGENT)
	{
		lookupName();
	}
}
void LLSpeaker::update(const speaker_entry_t& entry)
{
	setStatus(llmin(mStatus, entry.status));
	if (entry.moderator != boost::none)
	{
		mIsModerator = *entry.moderator;
	}
	if (entry.moderator_muted_text != boost::none)
	{
		mModeratorMutedText = *entry.moderator_muted_text;
	};
	if (entry.type == LLSpeaker::SPEAKER_AGENT)
	{
		mType = LLSpeaker::SPEAKER_AGENT;
		lookupName();
	}
	if (mDisplayName.empty())
		setName(entry.name);
}
void LLSpeaker::lookupName()
{
	if (mDisplayName.empty())
	{
		if (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES) && gAgentID != mID)
			setName(RlvStrings::getAnonym(mDisplayName));
		else
			LLAvatarNameCache::get(mID, boost::bind(&LLSpeaker::onNameCache, this, _2));
	}
}
void LLSpeaker::onNameCache(const LLAvatarName& full_name)
{
	static const LLCachedControl<S32> name_system("SpeakerNameSystem");
	if (!name_system)
		setName(gCacheName->cleanFullName(full_name.getLegacyName()));
	else
		setName(full_name.getNSName(name_system));
}
bool LLSpeaker::isInVoiceChannel()
{
	return mStatus <= LLSpeaker::STATUS_VOICE_ACTIVE || mStatus == LLSpeaker::STATUS_MUTED;
}
LLSpeakerUpdateSpeakerEvent::LLSpeakerUpdateSpeakerEvent(LLSpeaker* source)
: LLEvent(source, "Speaker update speaker event"),
  mSpeakerID (source->mID)
{
}
LLSD LLSpeakerUpdateSpeakerEvent::getValue()
{
	LLSD ret;
	ret["id"] = mSpeakerID;
	return ret;
}
LLSpeakerUpdateModeratorEvent::LLSpeakerUpdateModeratorEvent(LLSpeaker* source)
: LLEvent(source, "Speaker add moderator event"),
  mSpeakerID (source->mID),
  mIsModerator (source->mIsModerator)
{
}
LLSD LLSpeakerUpdateModeratorEvent::getValue()
{
	LLSD ret;
	ret["id"] = mSpeakerID;
	ret["is_moderator"] = mIsModerator;
	return ret;
}
LLSpeakerTextModerationEvent::LLSpeakerTextModerationEvent(LLSpeaker* source)
: LLEvent(source, "Speaker text moderation event")
{
}
LLSD LLSpeakerTextModerationEvent::getValue()
{
	return std::string("text");
}
LLSpeakerVoiceModerationEvent::LLSpeakerVoiceModerationEvent(LLSpeaker* source)
: LLEvent(source, "Speaker voice moderation event")
{
}
LLSD LLSpeakerVoiceModerationEvent::getValue()
{
	return std::string("voice");
}
LLSpeakerListChangeEvent::LLSpeakerListChangeEvent(LLSpeakerMgr* source, const LLUUID& speaker_id)
: LLEvent(source, "Speaker added/removed from speaker mgr"),
  mSpeakerID(speaker_id)
{
}
LLSD LLSpeakerListChangeEvent::getValue()
{
	return mSpeakerID;
}
struct LLSortRecentSpeakers
{
	bool operator()(const LLPointer<LLSpeaker> lhs, const LLPointer<LLSpeaker> rhs) const;
};
bool LLSortRecentSpeakers::operator()(const LLPointer<LLSpeaker> lhs, const LLPointer<LLSpeaker> rhs) const
{
	if (lhs->mStatus != rhs->mStatus)
	{
		return (lhs->mStatus < rhs->mStatus);
	}
	if(lhs->mLastSpokeTime != rhs->mLastSpokeTime)
	{
		return (lhs->mLastSpokeTime > rhs->mLastSpokeTime);
	}
	return(	lhs->mDisplayName.compare(rhs->mDisplayName) < 0 );
}
LLSpeakerActionTimer::LLSpeakerActionTimer(action_callback_t action_cb, F32 action_period, const LLUUID& speaker_id)
: LLEventTimer(action_period)
, mActionCallback(action_cb)
, mSpeakerId(speaker_id)
{
}
BOOL LLSpeakerActionTimer::tick()
{
	if (mActionCallback)
	{
		return (BOOL)mActionCallback(mSpeakerId);
	}
	return TRUE;
}
void LLSpeakerActionTimer::unset()
{
	mActionCallback = nullptr;
}
LLSpeakersDelayActionsStorage::LLSpeakersDelayActionsStorage(LLSpeakerActionTimer::action_callback_t action_cb, F32 action_delay)
: mActionCallback(action_cb)
, mActionDelay(action_delay)
{
}
LLSpeakersDelayActionsStorage::~LLSpeakersDelayActionsStorage()
{
	removeAllTimers();
}
void LLSpeakersDelayActionsStorage::setActionTimer(const LLUUID& speaker_id)
{
	bool not_found = true;
	if (!mActionTimersMap.empty())
	{
		not_found = mActionTimersMap.find(speaker_id) == mActionTimersMap.end();
	}
	if (not_found)
	{
		mActionTimersMap.insert(LLSpeakerActionTimer::action_value_t(speaker_id,
			new LLSpeakerActionTimer(
				boost::bind(&LLSpeakersDelayActionsStorage::onTimerActionCallback, this, _1),
				mActionDelay, speaker_id)));
	}
}
void LLSpeakersDelayActionsStorage::unsetActionTimer(const LLUUID& speaker_id)
{
	if (mActionTimersMap.empty()) return;
	LLSpeakerActionTimer::action_timer_iter_t it_speaker = mActionTimersMap.find(speaker_id);
	if (it_speaker != mActionTimersMap.end())
	{
		it_speaker->second->unset();
		mActionTimersMap.erase(it_speaker);
	}
}
void LLSpeakersDelayActionsStorage::removeAllTimers()
{
	LLSpeakerActionTimer::action_timer_iter_t iter = mActionTimersMap.begin();
	for (; iter != mActionTimersMap.end(); ++iter)
	{
		delete iter->second;
	}
	mActionTimersMap.clear();
}
bool LLSpeakersDelayActionsStorage::onTimerActionCallback(const LLUUID& speaker_id)
{
	unsetActionTimer(speaker_id);
	if (mActionCallback)
	{
		mActionCallback(speaker_id);
	}
	return true;
}
bool LLSpeakersDelayActionsStorage::isTimerStarted(const LLUUID& speaker_id)
{
	return (mActionTimersMap.size() > 0) && (mActionTimersMap.find(speaker_id) != mActionTimersMap.end());
}
class ModerationResponder : public LLHTTPClient::ResponderIgnoreBody
{
public:
	ModerationResponder(const LLUUID& session_id)
	{
		mSessionID = session_id;
	}
protected:
	virtual void httpFailure()
	{
		LL_WARNS() << dumpResponse() << LL_ENDL;
		if ( gIMMgr )
		{
			LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(mSessionID);
			if (!floaterp) return;
			if ( 403 == mStatus )
			{
				floaterp->showSessionEventError(
					"mute",
					"not_a_mod_error");
			}
			else
			{
				floaterp->showSessionEventError(
					"mute",
					"generic_request_error");
			}
		}
	}
	char const* getName(void) const { return "ModerationResponder"; }
private:
	LLUUID mSessionID;
};
LLSpeakerMgr::LLSpeakerMgr(LLVoiceChannel* channelp) :
	mVoiceChannel(channelp),
	mVoiceModerated(false),
	mModerateModeHandledFirstTime(false),
	mSpeakerListUpdated(false)
{
	mGetListTime.reset();
	static LLUICachedControl<F32> remove_delay ("SpeakerParticipantRemoveDelay", 10.0);
	mSpeakerDelayRemover = new LLSpeakersDelayActionsStorage(boost::bind(&LLSpeakerMgr::removeSpeaker, this, _1), remove_delay);
}
LLSpeakerMgr::~LLSpeakerMgr()
{
	delete mSpeakerDelayRemover;
}
void LLSpeakerMgr::setSpeakers(const std::vector<speaker_entry_t>& speakers)
{
	if (!speakers.empty())
	{
		fireEvent(new LLOldEvents::LLEvent(this), "batch_begin");
		for (auto entry : speakers)
		{
			setSpeaker(entry);
		}
		fireEvent(new LLOldEvents::LLEvent(this), "batch_end");
	}
}
LLPointer<LLSpeaker> LLSpeakerMgr::setSpeaker(const speaker_entry_t& entry)
{
	LLUUID session_id = getSessionID();
	const LLUUID& id = entry.id;
	if (id.isNull() || (id == session_id))
	{
		return NULL;
	}
	LLPointer<LLSpeaker> speakerp;
	auto it = mSpeakers.find(id);
	if (it == mSpeakers.end() || it->second.isNull())
	{
		mSpeakersSorted.emplace_back(new LLSpeaker(entry));
		mSpeakers.emplace(id, mSpeakersSorted.back());
		fireEvent(new LLSpeakerListChangeEvent(this, id), "add");
	}
	else
	{
		it->second->update(entry);
	}
	mSpeakerDelayRemover->unsetActionTimer(entry.id);
	return speakerp;
}
void LLSpeakerMgr::initVoiceModerateMode()
{
	if (!mModerateModeHandledFirstTime && (mVoiceChannel && mVoiceChannel->isActive()))
	{
		LLPointer<LLSpeaker> speakerp;
		if (mSpeakers.find(gAgentID) != mSpeakers.end())
		{
			speakerp = mSpeakers[gAgentID];
		}
		if (speakerp.notNull())
		{
			mVoiceModerated = speakerp->mModeratorMutedVoice;
			mModerateModeHandledFirstTime = true;
		}
	}
}
void LLSpeakerMgr::update(BOOL resort_ok)
{
	if (!LLVoiceClient::instanceExists())
	{
		return;
	}
	static const LLCachedControl<LLColor4> speaking_color(gSavedSettings, "SpeakingColor");
	static const LLCachedControl<LLColor4> overdriven_color(gSavedSettings, "OverdrivenColor");
	if(resort_ok)
	{
		updateSpeakerList();
	}
	BOOL voice_channel_active = (!mVoiceChannel && LLVoiceClient::getInstance()->inProximalChannel()) || (mVoiceChannel && mVoiceChannel->isActive());
	bool re_sort = false;
	for (auto& speaker : mSpeakers)
	{
		LLUUID speaker_id = speaker.first;
		LLSpeaker* speakerp = speaker.second;
		if (voice_channel_active && LLVoiceClient::getInstance()->getVoiceEnabled(speaker_id))
		{
			speakerp->mSpeechVolume = LLVoiceClient::getInstance()->getCurrentPower(speaker_id);
			bool moderator_muted_voice = LLVoiceClient::getInstance()->getIsModeratorMuted(speaker_id);
			if (moderator_muted_voice != speakerp->mModeratorMutedVoice)
			{
				speakerp->mModeratorMutedVoice = moderator_muted_voice;
				LL_DEBUGS("Speakers") << (speakerp->mModeratorMutedVoice? "Muted" : "Umuted") << " speaker " << speaker_id<< LL_ENDL;
				speakerp->fireEvent(new LLSpeakerVoiceModerationEvent(speakerp));
			}
			if (LLVoiceClient::getInstance()->getOnMuteList(speaker_id) || speakerp->mModeratorMutedVoice)
			{
				speakerp->setStatus(LLSpeaker::STATUS_MUTED);
			}
			else if (LLVoiceClient::getInstance()->getIsSpeaking(speaker_id))
			{
				if (speakerp->mStatus != LLSpeaker::STATUS_SPEAKING)
				{
					speakerp->setSpokenTime(mSpeechTimer.getElapsedTimeF32());
					speakerp->mHasSpoken = TRUE;
					fireEvent(new LLSpeakerUpdateSpeakerEvent(speakerp), "update_speaker");
				}
				speakerp->setStatus(LLSpeaker::STATUS_SPEAKING);
				speakerp->mDotColor = speaking_color;
				if (speakerp->mSpeechVolume > LLVoiceClient::OVERDRIVEN_POWER_LEVEL)
				{
					speakerp->mDotColor = overdriven_color;
				}
			}
			else
			{
				speakerp->mSpeechVolume = 0.f;
				speakerp->mDotColor = ACTIVE_COLOR;
				if (speakerp->mHasSpoken)
				{
					speakerp->setStatus(LLSpeaker::STATUS_HAS_SPOKEN);
				}
				else
				{
					speakerp->setStatus(LLSpeaker::STATUS_VOICE_ACTIVE);
				}
			}
		}
		else if (speakerp->mStatus != LLSpeaker::STATUS_NOT_IN_CHANNEL)
		{
			if(speakerp->mType == LLSpeaker::SPEAKER_EXTERNAL)
			{
				setSpeakerNotInChannel(speakerp);
			}
			else
			{
				speakerp->setStatus(LLSpeaker::STATUS_TEXT_ONLY);
				speakerp->mSpeechVolume = 0.f;
				speakerp->mDotColor = ACTIVE_COLOR;
			}
		}
		if (speakerp->mNeedsResort)
		{
			re_sort = true;
			speakerp->mNeedsResort = false;
		}
	}
	if (resort_ok && re_sort)
	{
		std::sort(mSpeakersSorted.begin(), mSpeakersSorted.end(), LLSortRecentSpeakers());
		bool index_changed = false;
		S32 recent_speaker_count = 0;
		S32 sort_index = 0;
		for (auto speakerp : mSpeakersSorted)
		{
			if (speakerp->mStatus == LLSpeaker::STATUS_HAS_SPOKEN)
			{
				speakerp->mDotColor = lerp(speaking_color, ACTIVE_COLOR, clamp_rescale((F32)recent_speaker_count, -2.f, 3.f, 0.f, 1.f));
				recent_speaker_count++;
			}
			if (speakerp->mSortIndex != sort_index++)
			{
				speakerp->mSortIndex = sort_index-1;
				index_changed = true;
			}
		}
		if (index_changed)
		{
			fireEvent(new LLOldEvents::LLEvent(this), "update_sorting");
		}
	}
}
void LLSpeakerMgr::updateSpeakerList()
{
	if ((!mVoiceChannel && LLVoiceClient::getInstance()->inProximalChannel()) || (mVoiceChannel && mVoiceChannel->isActive()))
	{
		uuid_set_t participants;
		LLVoiceClient::getInstance()->getParticipantList(participants);
		std::vector<speaker_entry_t> speakers;
		speakers.reserve(participants.size());
		for (auto participant : participants)
		{
			speakers.emplace_back(participant,
				(LLVoiceClient::getInstance()->isParticipantAvatar(participant) ? LLSpeaker::SPEAKER_AGENT : LLSpeaker::SPEAKER_EXTERNAL),
				LLSpeaker::STATUS_VOICE_ACTIVE,
				boost::none,
				boost::none,
				LLVoiceClient::getInstance()->getDisplayName(participant));
		}
		setSpeakers(speakers);
	}
	else
	{
		LLUUID const& session_id = getSessionID();
		if (!session_id.isNull() && !mSpeakerListUpdated)
		{
			LLFloaterIMPanel* floater = gIMMgr->findFloaterBySession(session_id);
			if (floater && floater->getSessionType() == LLFloaterIMPanel::GROUP_SESSION && (mSpeakers.size() <= 1))
			{
				const F32 load_group_timeout = gSavedSettings.getF32("ChatLoadGroupTimeout");
				LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(session_id);
				if (!gdatap && (mGetListTime.getElapsedTimeF32() >= load_group_timeout))
				{
					LLGroupMgr::getInstance()->sendCapGroupMembersRequest(session_id);
				}
				else if (gdatap && gdatap->isMemberDataComplete() && !gdatap->mMembers.empty())
				{
					std::vector<speaker_entry_t> speakers;
					speakers.reserve(gdatap->mMembers.size());
					LLGroupMgrGroupData::member_list_t::iterator member_it = gdatap->mMembers.begin();
					while (member_it != gdatap->mMembers.end())
					{
						LLGroupMemberData* member = member_it->second;
						LLUUID id = member_it->first;
						const std::string& localized_online();
						if ((member->getOnlineStatus() == localized_online()) && (mSpeakers.find(id) == mSpeakers.end()))
						{
							speakers.emplace_back(
								id,
								LLSpeaker::SPEAKER_AGENT,
								LLSpeaker::STATUS_VOICE_ACTIVE,
								(member->getAgentPowers() & GP_SESSION_MODERATOR) == GP_SESSION_MODERATOR);
						}
						++member_it;
					}
					setSpeakers(speakers);
					mSpeakerListUpdated = true;
				}
			}
			else if (floater && mSpeakers.empty())
			{
				for (const auto& target_id : floater->mInitialTargetIDs)
				{
					if (!LLAvatarTracker::instance().isBuddy(target_id) || LLAvatarTracker::instance().isBuddyOnline(
                        target_id))
					{
						setSpeaker({target_id, LLSpeaker::SPEAKER_AGENT, LLSpeaker::STATUS_VOICE_ACTIVE });
					}
				}
				mSpeakerListUpdated = true;
			}
			else
			{
				mSpeakerListUpdated = true;
			}
		}
	}
	setSpeaker({ gAgentID, LLSpeaker::SPEAKER_AGENT, LLSpeaker::STATUS_VOICE_ACTIVE });
}
void LLSpeakerMgr::setSpeakerNotInChannel(LLPointer<LLSpeaker> speakerp)
{
	if  (speakerp.notNull())
	{
		speakerp->setStatus(LLSpeaker::STATUS_NOT_IN_CHANNEL);
		speakerp->mDotColor = INACTIVE_COLOR;
		mSpeakerDelayRemover->setActionTimer(speakerp->mID);
	}
}
bool LLSpeakerMgr::removeSpeaker(const LLUUID& speaker_id)
{
	mSpeakers.erase(speaker_id);
	speaker_list_t::iterator sorted_speaker_it = mSpeakersSorted.begin();
	for(; sorted_speaker_it != mSpeakersSorted.end(); ++sorted_speaker_it)
	{
		if (speaker_id == (*sorted_speaker_it)->mID)
		{
			mSpeakersSorted.erase(sorted_speaker_it);
			break;
		}
	}
	LL_DEBUGS("Speakers") << "Removed speaker " << speaker_id << LL_ENDL;
	fireEvent(new LLSpeakerListChangeEvent(this, speaker_id), "remove");
	update(TRUE);
	return false;
}
LLPointer<LLSpeaker> LLSpeakerMgr::findSpeaker(const LLUUID& speaker_id)
{
	if (mSpeakers.empty())
		return nullptr;
	speaker_map_t::iterator found_it = mSpeakers.find(speaker_id);
	if (found_it == mSpeakers.end())
	{
		return nullptr;
	}
	return found_it->second;
}
void LLSpeakerMgr::getSpeakerList(speaker_list_t* speaker_list, BOOL include_text)
{
	speaker_list->clear();
	for (auto& speaker : mSpeakers)
	{
		LLPointer<LLSpeaker> speakerp = speaker.second;
		if (include_text || speakerp->mStatus != LLSpeaker::STATUS_TEXT_ONLY)
		{
			speaker_list->push_back(speakerp);
		}
	}
}
const LLUUID LLSpeakerMgr::getSessionID() const
{
	return mVoiceChannel->getSessionID();
}
bool LLSpeakerMgr::isSpeakerToBeRemoved(const LLUUID& speaker_id) const
{
	return mSpeakerDelayRemover && mSpeakerDelayRemover->isTimerStarted(speaker_id);
}
void LLSpeakerMgr::setSpeakerTyping(const LLUUID& speaker_id, BOOL typing)
{
	LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);
	if (speakerp.notNull())
	{
		speakerp->mTyping = typing;
	}
}
void LLSpeakerMgr::speakerChatted(const LLUUID& speaker_id)
{
	LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);
	if (speakerp.notNull())
	{
		speakerp->setSpokenTime(mSpeechTimer.getElapsedTimeF32());
		speakerp->mHasSpoken = TRUE;
		fireEvent(new LLSpeakerUpdateSpeakerEvent(speakerp), "update_speaker");
	}
}
BOOL LLSpeakerMgr::isVoiceActive() const
{
	return LLVoiceClient::getInstance()->voiceEnabled() && mVoiceChannel && mVoiceChannel->isActive();
}
LLIMSpeakerMgr::LLIMSpeakerMgr(LLVoiceChannel* channel) : LLSpeakerMgr(channel)
{
}
void LLIMSpeakerMgr::updateSpeakerList()
{
	LLSpeakerMgr::updateSpeakerList();
	return;
}
void LLIMSpeakerMgr::setSpeakers(const LLSD& speakers)
{
	if ( !speakers.isMap() ) return;
	std::vector<speaker_entry_t> speakerentries;
	if ( speakers.has("agent_info") && speakers["agent_info"].isMap() )
	{
		for (const auto& speaker : speakers["agent_info"].map())
		{
			boost::optional<bool> moderator;
			boost::optional<bool> moderator_muted;
			if (speaker.second.isMap())
			{
				moderator = speaker.second["is_moderator"];
				moderator_muted = speaker.second["mutes"]["text"];
			}
			speakerentries.emplace_back(
				LLUUID(speaker.first),
				LLSpeaker::SPEAKER_AGENT,
				LLSpeaker::STATUS_TEXT_ONLY,
				moderator,
				moderator_muted
				);
		}
	}
	else if ( speakers.has("agents" ) && speakers["agents"].isArray() )
	{
		for (auto const& entry : speakers["agents"].array())
		{
			speakerentries.emplace_back(entry.asUUID());
		}
	}
	LLSpeakerMgr::setSpeakers(speakerentries);
}
void LLIMSpeakerMgr::updateSpeakers(const LLSD& update)
{
	if ( !update.isMap() ) return;
	std::vector<speaker_entry_t> speakerentries;
	if ( update.has("agent_updates") && update["agent_updates"].isMap() )
	{
		for (const auto& update_it : update["agent_updates"].map())
		{
			LLUUID agent_id(update_it.first);
			LLPointer<LLSpeaker> speakerp = findSpeaker(agent_id);
			bool new_speaker = false;
			boost::optional<bool> moderator;
			boost::optional<bool> moderator_muted_text;
			LLSD agent_data = update_it.second;
			if (agent_data.isMap() && agent_data.has("transition"))
			{
				if (agent_data["transition"].asString() == "LEAVE")
				{
					setSpeakerNotInChannel(speakerp);
				}
				else if (agent_data["transition"].asString() == "ENTER")
				{
					new_speaker = true;
				}
				else
				{
					LL_WARNS() << "bad membership list update from 'agent_updates' for agent " << agent_id << ", transition " << ll_print_sd(agent_data["transition"]) << LL_ENDL;
				}
			}
			if (speakerp.isNull() && !new_speaker)
			{
				continue;
			}
			if (agent_data.isMap() && agent_data.has("info"))
			{
				LLSD agent_info = agent_data["info"];
				if (agent_info.has("is_moderator"))
				{
					moderator = agent_info["is_moderator"];
				}
				if (agent_info.has("mutes"))
				{
					moderator_muted_text = agent_info["mutes"]["text"];
				}
			}
			speakerentries.emplace_back(
				agent_id,
				LLSpeaker::SPEAKER_AGENT,
				LLSpeaker::STATUS_TEXT_ONLY,
				moderator,
				moderator_muted_text
			);
		}
	}
	else if ( update.has("updates") && update["updates"].isMap() )
	{
		for (const auto& update_it : update["updates"].map())
		{
			LLUUID agent_id(update_it.first);
			LLPointer<LLSpeaker> speakerp = findSpeaker(agent_id);
			std::string agent_transition = update_it.second.asString();
			if (agent_transition == "LEAVE")
			{
				setSpeakerNotInChannel(speakerp);
			}
			else if ( agent_transition == "ENTER")
			{
				speakerentries.emplace_back(agent_id);
			}
			else
			{
				LL_WARNS() << "bad membership list update from 'updates' for agent " << agent_id << ", transition " << agent_transition << LL_ENDL;
			}
		}
	}
	LLSpeakerMgr::setSpeakers(speakerentries);
}
void LLIMSpeakerMgr::toggleAllowTextChat(const LLUUID& speaker_id)
{
	LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);
	if (!speakerp) return;
	std::string url = gAgent.getRegionCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "mute update";
	data["session-id"] = getSessionID();
	data["params"] = LLSD::emptyMap();
	data["params"]["agent_id"] = speaker_id;
	data["params"]["mute_info"] = LLSD::emptyMap();
	data["params"]["mute_info"]["text"] = !speakerp->mModeratorMutedText;
	LLHTTPClient::post(url, data, new ModerationResponder(getSessionID()));
}
void LLIMSpeakerMgr::moderateVoiceParticipant(const LLUUID& avatar_id, bool unmute)
{
	LLPointer<LLSpeaker> speakerp = findSpeaker(avatar_id);
	if (!speakerp) return;
	bool is_in_voice = speakerp->mStatus <= LLSpeaker::STATUS_VOICE_ACTIVE || speakerp->mStatus == LLSpeaker::STATUS_MUTED;
	if (!is_in_voice) return;
	std::string url = gAgent.getRegionCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "mute update";
	data["session-id"] = getSessionID();
	data["params"] = LLSD::emptyMap();
	data["params"]["agent_id"] = avatar_id;
	data["params"]["mute_info"] = LLSD::emptyMap();
	data["params"]["mute_info"]["voice"] = !unmute;
	LLHTTPClient::post(
		url,
		data,
		new ModerationResponder(getSessionID()));
}
void LLIMSpeakerMgr::moderateVoiceAllParticipants( bool unmute_everyone )
{
	if (mVoiceModerated == !unmute_everyone)
	{
		forceVoiceModeratedMode(mVoiceModerated);
	}
	else
	{
		moderateVoiceSession(getSessionID(), !unmute_everyone);
	}
}
void LLIMSpeakerMgr::processSessionUpdate(const LLSD& session_update)
{
	if (session_update.has("moderated_mode") &&
		session_update["moderated_mode"].has("voice"))
	{
		mVoiceModerated = session_update["moderated_mode"]["voice"];
	}
}
void LLIMSpeakerMgr::moderateVoiceSession(const LLUUID& session_id, bool disallow_voice)
{
	std::string url = gAgent.getRegionCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "session update";
	data["session-id"] = session_id;
	data["params"] = LLSD::emptyMap();
	data["params"]["update_info"] = LLSD::emptyMap();
	data["params"]["update_info"]["moderated_mode"] = LLSD::emptyMap();
	data["params"]["update_info"]["moderated_mode"]["voice"] = disallow_voice;
	LLHTTPClient::post(url, data, new ModerationResponder(session_id));
}
void LLIMSpeakerMgr::forceVoiceModeratedMode(bool should_be_muted)
{
	for (auto& speaker : mSpeakers)
	{
		LLUUID speaker_id = speaker.first;
		LLSpeaker* speakerp = speaker.second;
		if (should_be_muted != static_cast<bool>(speakerp->mModeratorMutedVoice))
		{
			moderateVoiceParticipant(speaker_id, !should_be_muted);
		}
	}
}
LLActiveSpeakerMgr::LLActiveSpeakerMgr() : LLSpeakerMgr(NULL)
{
}
void LLActiveSpeakerMgr::updateSpeakerList()
{
	const auto old_channel = mVoiceChannel;
	mVoiceChannel = LLVoiceChannel::getCurrentVoiceChannel();
	if (mVoiceChannel != old_channel)
	{
		LL_DEBUGS("Speakers") << "Removed all speakers" << LL_ENDL;
		fireEvent(new LLSpeakerListChangeEvent(this, LLUUID::null), "clear");
		mSpeakers.clear();
		mSpeakersSorted.clear();
		mVoiceChannel = LLVoiceChannel::getCurrentVoiceChannel();
		mSpeakerDelayRemover->removeAllTimers();
	}
	LLSpeakerMgr::updateSpeakerList();
	for (auto& speaker : mSpeakers)
	{
		LLSpeaker* speakerp = speaker.second;
		if (speakerp->mStatus == LLSpeaker::STATUS_TEXT_ONLY)
		{
			setSpeakerNotInChannel(speakerp);
		}
	}
}
LLLocalSpeakerMgr::LLLocalSpeakerMgr() : LLSpeakerMgr(LLVoiceChannelProximal::getInstance())
{
}
LLLocalSpeakerMgr::~LLLocalSpeakerMgr ()
{
}
void LLLocalSpeakerMgr::updateSpeakerList()
{
	LLSpeakerMgr::updateSpeakerList();
	if (gDisconnected)
	{
		return ;
	}
	uuid_vec_t avatar_ids;
	LLWorld::getInstance()->getAvatars(&avatar_ids, nullptr, gAgent.getPositionGlobal(), CHAT_NORMAL_RADIUS);
	std::vector<speaker_entry_t> speakers;
	speakers.reserve(avatar_ids.size());
	for (const auto& id : avatar_ids)
	{
		speakers.emplace_back(id);
	}
	setSpeakers(speakers);
	for (auto& speaker : mSpeakers)
	{
		LLUUID speaker_id = speaker.first;
		LLPointer<LLSpeaker> speakerp = speaker.second;
		if (speakerp.notNull() && speakerp->mStatus == LLSpeaker::STATUS_TEXT_ONLY)
		{
			LLVOAvatar* avatarp = gObjectList.findAvatar(speaker_id);
			if (!avatarp || dist_vec_squared(avatarp->getPositionAgent(), gAgent.getPositionAgent()) > CHAT_NORMAL_RADIUS * CHAT_NORMAL_RADIUS)
			{
				setSpeakerNotInChannel(speakerp);
			}
		}
	}
}
