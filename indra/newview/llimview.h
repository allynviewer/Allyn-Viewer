/** 
 * @file LLIMMgr.h
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
#ifndef LL_LLIMVIEW_H
#define LL_LLIMVIEW_H
#include "llmultifloater.h"
#include "llinstantmessage.h"
#include "lluuid.h"
#include "llvoicechannel.h"
class LLFloaterChatterBox;
class LLFloaterIMPanel;
class LLIMMgr final : public LLSingleton<LLIMMgr>
{
public:
	enum EInvitationType
	{
		INVITATION_TYPE_INSTANT_MESSAGE = 0,
		INVITATION_TYPE_VOICE = 1,
		INVITATION_TYPE_IMMEDIATE = 2
	};
	LLIMMgr();
	virtual ~LLIMMgr();
	void addMessage(const LLUUID& session_id,
					const LLUUID& target_id,
					const std::string& from,
					const std::string& msg,
					const std::string& session_name = LLStringUtil::null,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					U32 parent_estate_id = 0,
					const LLUUID& region_id = LLUUID::null,
					const LLVector3& position = LLVector3::zero,
					bool link_name = false);
	void addSystemMessage(const LLUUID& session_id, const std::string& message_name, const LLSD& args);
	BOOL isIMSessionOpen(const LLUUID& uuid);
	LLUUID addSession(const std::string& name,
					  EInstantMessage dialog,
					  const LLUUID& other_participant_id);
	LLUUID addSession(const std::string& name,
					  EInstantMessage dialog,
					  const LLUUID& other_participant_id,
					  const uuid_vec_t& ids);
	LLUUID addP2PSession(const std::string& name,
					  const LLUUID& other_participant_id,
					  const std::string& voice_session_handle,
					  const std::string& caller_uri = LLStringUtil::null);
	void removeSession(const LLUUID& session_id);
	void inviteToSession(
		const LLUUID& session_id,
		const std::string& session_name,
		const LLUUID& caller,
		const std::string& caller_name,
		EInstantMessage type,
		EInvitationType inv_type,
		const std::string& session_handle = LLStringUtil::null,
		const std::string& session_uri = LLStringUtil::null);
	void updateFloaterSessionID(const LLUUID& old_session_id,
								const LLUUID& new_session_id);
	void processIMTypingStart(const LLUUID& from_id, const EInstantMessage im_type);
	void processIMTypingStop(const LLUUID& from_id, const EInstantMessage im_type);
	void clearNewIMNotification();
	void autoStartCallOnStartup(const LLUUID& session_id);
	int getIMUnreadCount();
	void		setFloaterOpen(BOOL open);
	BOOL		getFloaterOpen();
	LLFloaterChatterBox* getFloater();
	void disconnectAllSessions();
	void toggle();
	BOOL hasSession(const LLUUID& session_id);
	LLFloaterIMPanel* findFloaterBySession(const LLUUID& session_id);
	static LLUUID computeSessionID(EInstantMessage dialog, const LLUUID& other_participant_id);
	void clearPendingInvitation(const LLUUID& session_id);
	void processAgentListUpdates(const LLUUID& session_id, const LLSD& body);
	LLSD getPendingAgentListUpdates(const LLUUID& session_id);
	void addPendingAgentListUpdates(
		const LLUUID& sessioN_id,
		const LLSD& updates);
	void clearPendingAgentListUpdates(const LLUUID& session_id);
	const std::set<LLHandle<LLFloater> >& getIMFloaterHandles() { return mFloaters; }
	void loadIgnoreGroup();
	void saveIgnoreGroup();
	void updateIgnoreGroup(const LLUUID& group_id, bool ignore);
	bool getIgnoreGroup(const LLUUID& group_id) const;
	bool startCall(const LLUUID& session_id, LLVoiceChannel::EDirection direction = LLVoiceChannel::OUTGOING_CALL);
	bool endCall(const LLUUID& session_id);
	void addNotifiedNonFriendSessionID(const LLUUID& session_id);
	bool isNonFriendSessionNotified(const LLUUID& session_id);
	static std::string getOfflineMessage(const LLUUID& id);
private:
	LLFloaterIMPanel* createFloater(const LLUUID& session_id,
									const LLUUID& target_id,
									const std::string& name,
									const EInstantMessage& dialog,
									const uuid_vec_t& ids = uuid_vec_t(),
									bool user_initiated = false);
	void noteOfflineUsers(LLFloaterIMPanel* panel, const uuid_vec_t& ids);
	void noteMutedUsers(LLFloaterIMPanel* panel, const uuid_vec_t& ids);
	void processIMTypingCore(const LLUUID& from_id, const EInstantMessage im_type, BOOL typing);
private:
	std::set<LLHandle<LLFloater> > mFloaters;
	typedef uuid_set_t notified_non_friend_sessions_t;
	notified_non_friend_sessions_t mNotifiedNonFriendSessions;
	int		mIMUnreadCount;
	LLSD mPendingInvitations;
	LLSD mPendingAgentListUpdates;
	std::list<LLUUID> mIgnoreGroupList;
public:
	S32 getIgnoreGroupListCount() { return mIgnoreGroupList.size(); }
};
extern LLIMMgr *gIMMgr;
#endif
