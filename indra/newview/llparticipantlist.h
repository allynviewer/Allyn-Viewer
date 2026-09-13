/**
 * @file llparticipantlist.h
 * @brief LLParticipantList intended to update view(LLAvatarList) according to incoming messages
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
#ifndef LL_PARTICIPANTLIST_H
#define LL_PARTICIPANTLIST_H
#include "lllayoutstack.h"
class LLSpeakerMgr;
class LLNameListCtrl;
class LLUICtrl;
class LLParticipantList : public LLLayoutPanel
{
	LOG_CLASS(LLParticipantList);
public:
	typedef boost::function<bool (const LLUUID& speaker_id)> validate_speaker_callback_t;
	LLParticipantList(LLSpeakerMgr* data_source, bool show_text_chatters);
	BOOL postBuild();
	~LLParticipantList();
	void addAvatarIDExceptAgent(const LLUUID& avatar_id);
	void refreshSpeakers();
	void setValidateSpeakerCallback(validate_speaker_callback_t cb);
	void setVoiceModerationCtrlMode(const bool& moderated_voice);
protected:
	bool onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	bool onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	void onSpeakerBatchBeginEvent();
	void onSpeakerBatchEndEvent();
	void onSpeakerSortingUpdateEvent();
	class BaseSpeakerListener : public LLOldEvents::LLSimpleListener
	{
	public:
		BaseSpeakerListener(LLParticipantList& parent) : mParent(parent) {}
	protected:
		LLParticipantList& mParent;
	};
	class SpeakerAddListener : public BaseSpeakerListener
	{
	public:
		SpeakerAddListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	class SpeakerRemoveListener : public BaseSpeakerListener
	{
	public:
		SpeakerRemoveListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	class SpeakerClearListener : public BaseSpeakerListener
	{
	public:
		SpeakerClearListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	class SpeakerMuteListener : public BaseSpeakerListener
	{
	public:
		SpeakerMuteListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	class SpeakerBatchBeginListener : public BaseSpeakerListener
	{
	public:
		SpeakerBatchBeginListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	class SpeakerBatchEndListener : public BaseSpeakerListener
	{
	public:
		SpeakerBatchEndListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	class SpeakerSortingUpdateListener : public BaseSpeakerListener
	{
	public:
		SpeakerSortingUpdateListener(LLParticipantList& parent) : BaseSpeakerListener(parent) {}
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
	};
	private:
		void toggleAllowTextChat(const LLSD& userdata);
		void toggleMute(const LLSD& userdata, U32 flags);
		void toggleMuteText();
		void toggleMuteVoice();
		void moderateVoice(const LLSD& userdata);
		void moderateVoiceParticipant(const LLSD& param);
		void moderateVoiceAllParticipants(const LLSD& param);
private:
	void onAvatarListDoubleClicked();
	void setupContextMenu();
	void adjustParticipant(const LLUUID& speaker_id);
	void handleSpeakerSelect();
	void onClickProfile();
	void onSortChanged();
	void onVolumeChange(const LLSD& param);
	LLSpeakerMgr*		mSpeakerMgr;
	LLNameListCtrl*		mAvatarList;
	bool				mShowTextChatters;
	LLFrameTimer		mUpdateTimer;
	LLPointer<SpeakerAddListener>				mSpeakerAddListener;
	LLPointer<SpeakerRemoveListener>			mSpeakerRemoveListener;
	LLPointer<SpeakerClearListener>				mSpeakerClearListener;
	LLPointer<SpeakerMuteListener>				mSpeakerMuteListener;
	LLPointer<SpeakerBatchBeginListener>		mSpeakerBatchBeginListener;
	LLPointer<SpeakerBatchEndListener>			mSpeakerBatchEndListener;
	LLPointer<SpeakerSortingUpdateListener>		mSpeakerSortingUpdateListener;
	validate_speaker_callback_t mValidateSpeakerCallback;
};
#endif
