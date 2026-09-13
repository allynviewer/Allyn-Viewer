/**
 * @file llvoicevivox.h
 * @brief Declaration of LLDiamondwareVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#ifndef LL_VOICE_VIVOX_H
#define LL_VOICE_VIVOX_H
#include "llvoiceclient.h"
class LLAvatarName;
class LLVivoxVoiceClient :	public LLSingleton<LLVivoxVoiceClient>,
							virtual public LLVoiceModuleInterface,
							virtual public LLVoiceEffectInterface
{
	LOG_CLASS(LLVivoxVoiceClient);
public:
	LLVivoxVoiceClient();
	virtual ~LLVivoxVoiceClient();
	virtual void init(LLPumpIO *pump);
	virtual void terminate();
	virtual const LLVoiceVersionInfo& getVersion();
	virtual void updateSettings();
	virtual bool isVoiceWorking() const;
	virtual void tuningStart();
	virtual void tuningStop();
	virtual bool inTuningMode();
	virtual void tuningSetMicVolume(float volume);
	virtual void tuningSetSpeakerVolume(float volume);
	virtual float tuningGetEnergy(void);
	virtual bool deviceSettingsAvailable();
	virtual void refreshDeviceLists(bool clearCurrentList = true);
	virtual void setCaptureDevice(const std::string& name);
	virtual void setRenderDevice(const std::string& name);
	virtual LLVoiceDeviceList& getCaptureDevices();
	virtual LLVoiceDeviceList& getRenderDevices();
	virtual void getParticipantList(uuid_set_t &participants);
	virtual bool isParticipant(const LLUUID& speaker_id);
	virtual BOOL sendTextMessage(const LLUUID& participant_id, const std::string& message);
	virtual void endUserIMSession(const LLUUID &uuid);
	virtual BOOL isSessionCallBackPossible(const LLUUID &session_id);
	virtual BOOL isSessionTextIMPossible(const LLUUID &session_id);
	virtual bool inProximalChannel();
	virtual void setNonSpatialChannel(const std::string &uri,
									  const std::string &credentials);
	virtual void setSpatialChannel(const std::string &uri,
								   const std::string &credentials);
	virtual void leaveNonSpatialChannel();
	virtual void leaveChannel(void);
	virtual std::string getCurrentChannel();
	virtual void callUser(const LLUUID &uuid);
	virtual bool isValidChannel(std::string &channelHandle);
	virtual bool answerInvite(std::string &channelHandle);
	virtual void declineInvite(std::string &channelHandle);
	virtual void setVoiceVolume(F32 volume);
	virtual void setMicGain(F32 volume);
	virtual bool voiceEnabled();
	virtual void setVoiceEnabled(bool enabled);
	virtual BOOL lipSyncEnabled();
	virtual void setLipSyncEnabled(BOOL enabled);
	virtual void setMuteMic(bool muted);
	virtual BOOL getVoiceEnabled(const LLUUID& id);
	virtual std::string getDisplayName(const LLUUID& id);
	virtual BOOL isParticipantAvatar(const LLUUID &id);
	virtual BOOL getIsSpeaking(const LLUUID& id);
	virtual BOOL getIsModeratorMuted(const LLUUID& id);
	virtual F32 getCurrentPower(const LLUUID& id);
	virtual BOOL getOnMuteList(const LLUUID& id);
	virtual F32 getUserVolume(const LLUUID& id);
	virtual void setUserVolume(const LLUUID& id, F32 volume);
	virtual void userAuthorized(const std::string& user_id,
								const LLUUID &agentID);
	virtual void addObserver(LLVoiceClientStatusObserver* observer);
	virtual void removeObserver(LLVoiceClientStatusObserver* observer);
	virtual void addObserver(LLFriendObserver* observer);
	virtual void removeObserver(LLFriendObserver* observer);
	virtual void addObserver(LLVoiceClientParticipantObserver* observer);
	virtual void removeObserver(LLVoiceClientParticipantObserver* observer);
	virtual std::string sipURIFromID(const LLUUID &id);
	virtual bool setVoiceEffect(const LLUUID& id);
	virtual const LLUUID getVoiceEffect();
	virtual LLSD getVoiceEffectProperties(const LLUUID& id);
	virtual void refreshVoiceEffectLists(bool clear_lists);
	virtual const voice_effect_list_t& getVoiceEffectList() const;
	virtual const voice_effect_list_t& getVoiceEffectTemplateList() const;
	virtual void addObserver(LLVoiceEffectObserver* observer);
	virtual void removeObserver(LLVoiceEffectObserver* observer);
	virtual void enablePreviewBuffer(bool enable);
	virtual void recordPreviewBuffer();
	virtual void playPreviewBuffer(const LLUUID& effect_id = LLUUID::null);
	virtual void stopPreviewBuffer();
	virtual bool isPreviewRecording();
	virtual bool isPreviewPlaying();
protected:
	friend class LLVivoxVoiceAccountProvisionResponder;
	friend class LLVivoxVoiceClientMuteListObserver;
	friend class LLVivoxVoiceClientFriendsObserver;
	enum streamState
	{
		streamStateUnknown = 0,
		streamStateIdle = 1,
		streamStateConnected = 2,
		streamStateRinging = 3,
	};
	struct participantState
	{
	public:
		participantState(const std::string &uri, const LLUUID& id, bool isAv);
		bool updateMuteState();
		bool isAvatar();
		std::string mURI;
		LLUUID mAvatarID;
		std::string mAccountName;
		std::string mDisplayName;
		LLFrameTimer mSpeakingTimeout;
		F32	mLastSpokeTimestamp;
		F32 mPower;
		F32 mVolume;
		std::string mGroupID;
		int mUserVolume;
		bool mPTT;
		bool mIsSpeaking;
		bool mIsModeratorMuted;
		bool mOnMuteList;
		bool mVolumeSet;
		bool mVolumeDirty;
		bool mAvatarIDValid;
		bool mIsSelf;
	};
	typedef std::vector<participantState> participantList;
	struct sessionState
	{
	public:
		sessionState();
		~sessionState();
		participantState *addParticipant(const std::string &uri);
		void removeParticipant(const std::string& uri);
		void removeAllParticipants();
		participantState *findParticipant(const std::string &uri);
		participantState *findParticipantByID(const LLUUID& id);
		bool isCallBackPossible();
		bool isTextIMPossible();
		std::string mHandle;
		std::string mGroupHandle;
		std::string mSIPURI;
		std::string mAlias;
		std::string mName;
		std::string mAlternateSIPURI;
		std::string mHash;
		std::string mErrorStatusString;
		std::queue<std::string> mTextMsgQueue;
		LLUUID		mIMSessionID;
		LLUUID		mCallerID;
		int			mErrorStatusCode;
		int			mMediaStreamState;
		int			mTextStreamState;
		bool		mCreateInProgress;
		bool		mMediaConnectInProgress;
		bool		mVoiceInvitePending;
		bool		mTextInvitePending;
		bool		mSynthesizedCallerID;
		bool		mIsChannel;
		bool		mIsSpatial;
		bool		mIsP2P;
		bool		mIncoming;
		bool		mVoiceEnabled;
		bool		mReconnect;
		bool		mVolumeDirty;
		bool		mMuteDirty;
		bool		mParticipantsChanged;
		participantList mParticipantList;
		LLUUID		mVoiceFontID;
	};
	enum state
	{
		stateDisableCleanup,
		stateDisabled,
		stateStart,
		stateDaemonLaunched,
		stateConnecting,
		stateConnected,
		stateIdle,
		stateMicTuningStart,
		stateMicTuningRunning,
		stateMicTuningStop,
		stateCaptureBufferPaused,
		stateCaptureBufferRecStart,
		stateCaptureBufferRecording,
		stateCaptureBufferPlayStart,
		stateCaptureBufferPlaying,
		stateConnectorStart,
		stateConnectorStarting,
		stateConnectorStarted,
		stateLoginRetry,
		stateLoginRetryWait,
		stateNeedsLogin,
		stateLoggingIn,
		stateLoggedIn,
		stateVoiceFontsWait,
		stateVoiceFontsReceived,
		stateCreatingSessionGroup,
		stateNoChannel,
		stateRetrievingParcelVoiceInfo,
		stateJoiningSession,
		stateSessionJoined,
		stateRunning,
		stateLeavingSession,
		stateSessionTerminated,
		stateLoggingOut,
		stateLoggedOut,
		stateConnectorStopping,
		stateConnectorStopped,
		stateConnectorFailed,
		stateConnectorFailedWaiting,
		stateLoginFailed,
		stateLoginFailedWaiting,
		stateJoinSessionFailed,
		stateJoinSessionFailedWaiting,
		stateJail
	};
	typedef std::map<std::string, sessionState*> sessionMap;
	void daemonDied();
	void giveUp();
	bool writeString(const std::string &str);
	void connectorCreate();
	void connectorShutdown();
	void closeSocket(void);
	void requestVoiceAccountProvision(S32 retries = 3);
	void login(
			   const std::string& account_name,
			   const std::string& password,
			   const std::string& voice_sip_uri_hostname,
			   const std::string& voice_account_server_uri);
	void loginSendMessage();
	void logout();
	void logoutSendMessage();
	void tuningRenderStartSendMessage(const std::string& name, bool loop);
	void tuningRenderStopSendMessage();
	void tuningCaptureStartSendMessage(int duration);
	void tuningCaptureStopSendMessage();
	void clearCaptureDevices();
	void addCaptureDevice(const std::string& name);
	void clearRenderDevices();
	void addRenderDevice(const std::string& name);
	void buildSetAudioDevices(std::ostringstream &stream);
	void getCaptureDevicesSendMessage();
	void getRenderDevicesSendMessage();
	void buildLocalAudioUpdates(std::ostringstream &stream);
	void connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle, std::string &versionID);
	void loginResponse(int statusCode, std::string &statusString, std::string &accountHandle, int numberOfAliases);
	void sessionCreateResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle);
	void sessionGroupAddSessionResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle);
	void sessionConnectResponse(std::string &requestId, int statusCode, std::string &statusString);
	void logoutResponse(int statusCode, std::string &statusString);
	void connectorShutdownResponse(int statusCode, std::string &statusString);
	void accountLoginStateChangeEvent(std::string &accountHandle, int statusCode, std::string &statusString, int state);
	void mediaCompletionEvent(std::string &sessionGroupHandle, std::string &mediaCompletionType);
	void mediaStreamUpdatedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, int statusCode, std::string &statusString, int state, bool incoming);
	void textStreamUpdatedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, bool enabled, int state, bool incoming);
	void sessionAddedEvent(std::string &uriString, std::string &alias, std::string &sessionHandle, std::string &sessionGroupHandle, bool isChannel, bool incoming, std::string &nameString, std::string &applicationString);
	void sessionGroupAddedEvent(std::string &sessionGroupHandle);
	void sessionRemovedEvent(std::string &sessionHandle, std::string &sessionGroupHandle);
	void participantAddedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, std::string &uriString, std::string &alias, std::string &nameString, std::string &displayNameString, int participantType);
	void participantRemovedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, std::string &uriString, std::string &alias, std::string &nameString);
	void participantUpdatedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, std::string &uriString, std::string &alias, bool isModeratorMuted, bool isSpeaking, int volume, F32 energy);
	void auxAudioPropertiesEvent(F32 energy);
	void messageEvent(std::string &sessionHandle, std::string &uriString, std::string &alias, std::string &messageHeader, std::string &messageBody, std::string &applicationString);
	void sessionNotificationEvent(std::string &sessionHandle, std::string &uriString, std::string &notificationType);
	void muteListChanged();
	void updatePosition(void);
	void setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot);
	void setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot);
	bool channelFromRegion(LLViewerRegion *region, std::string &name);
	void setEarLocation(S32 loc);
	BOOL getUsingPTT(const LLUUID& id);
	std::string getGroupID(const LLUUID& id);
	BOOL getAreaVoiceDisabled();
	void recordingLoopStart(int seconds = 3600, int deltaFramesPerControlFrame = 200);
	void recordingLoopSave(const std::string& filename);
	void recordingStop();
	void filePlaybackStart(const std::string& filename);
	void filePlaybackStop();
	void filePlaybackSetPaused(bool paused);
	void filePlaybackSetMode(bool vox = false, float speed = 1.0f);
	participantState *findParticipantByID(const LLUUID& id);
	typedef std::set<sessionState*> sessionSet;
	typedef sessionSet::iterator sessionIterator;
	sessionIterator sessionsBegin(void);
	sessionIterator sessionsEnd(void);
	sessionState *findSession(const std::string &handle);
	sessionState *findSessionBeingCreatedByURI(const std::string &uri);
	sessionState *findSession(const LLUUID &participant_id);
	sessionState *findSessionByCreateID(const std::string &create_id);
	sessionState *addSession(const std::string &uri, const std::string &handle = LLStringUtil::null);
	void setSessionHandle(sessionState *session, const std::string &handle = LLStringUtil::null);
	void setSessionURI(sessionState *session, const std::string &uri);
	void deleteSession(sessionState *session);
	void deleteAllSessions(void);
	void verifySessionState(void);
	void joinedAudioSession(sessionState *session);
	void leftAudioSession(sessionState *session);
	void reapSession(sessionState *session);
	bool sessionNeedsRelog(sessionState *session);
	struct buddyListEntry
	{
		buddyListEntry(const std::string &uri);
		std::string mURI;
		std::string mDisplayName;
		LLUUID	mUUID;
		bool mOnlineSL;
		bool mOnlineSLim;
		bool mCanSeeMeOnline;
		bool mHasBlockListEntry;
		bool mHasAutoAcceptListEntry;
		bool mNameResolved;
		bool mInSLFriends;
		bool mInVivoxBuddies;
	};
	typedef std::map<std::string, buddyListEntry*> buddyListMap;
	void accountListBlockRulesSendMessage();
	void accountListAutoAcceptRulesSendMessage();
	void sessionGroupCreateSendMessage();
	void sessionCreateSendMessage(sessionState *session, bool startAudio = true, bool startText = false);
	void sessionGroupAddSessionSendMessage(sessionState *session, bool startAudio = true, bool startText = false);
	void sessionMediaConnectSendMessage(sessionState *session);
	void sessionTextConnectSendMessage(sessionState *session);
	void sessionTerminateSendMessage(sessionState *session);
	void sessionGroupTerminateSendMessage(sessionState *session);
	void sessionMediaDisconnectSendMessage(sessionState *session);
	void sessionTextDisconnectSendMessage(sessionState *session);
	void sessionTerminate();
	void requestRelog();
	void leaveAudioSession();
	bool parcelVoiceInfoReceived(state requesting_state);
	friend class LLVivoxVoiceClientCapResponder;
	void lookupName(const LLUUID &id);
	void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
	void avatarNameResolved(const LLUUID &id, const std::string &name);
	boost::signals2::connection mAvatarNameCacheConnection;
	void addVoiceFont(const S32 id,
					  const std::string &name,
					  const std::string &description,
					  const LLDate &expiration_date,
					  bool  has_expired,
					  const S32 font_type,
					  const S32 font_status,
					  const bool template_font = false);
	void accountGetSessionFontsResponse(int statusCode, const std::string &statusString);
	void accountGetTemplateFontsResponse(int statusCode, const std::string &statusString);
private:
	LLVoiceVersionInfo mVoiceVersion;
	void cleanUp();
	state mState;
	bool mSessionTerminateRequested;
	bool mRelogRequested;
	int mSpatialJoiningNum;
	void setState(state inState);
	state getState(void)  { return mState; };
	std::string state2string(state inState);
	void stateMachine();
	static void idle(void *user_data);
	LLHost mDaemonHost;
	LLSocket::ptr_t mSocket;
	bool mConnected;
	bool mTerminateDaemon;
	LLPumpIO *mPump;
	friend class LLVivoxProtocolParser;
	std::string mAccountName;
	std::string mAccountPassword;
	std::string mAccountDisplayName;
	bool mTuningMode;
	float mTuningEnergy;
	std::string mTuningAudioFile;
	int mTuningMicVolume;
	bool mTuningMicVolumeDirty;
	int mTuningSpeakerVolume;
	bool mTuningSpeakerVolumeDirty;
	state mTuningExitState;
	std::string mSpatialSessionURI;
	std::string mSpatialSessionCredentials;
	std::string mMainSessionGroupHandle;
	std::string mChannelName;
	bool mAreaVoiceDisabled;
	sessionState *mAudioSession;
	bool mAudioSessionChanged;
	sessionState *mNextAudioSession;
	S32 mCurrentParcelLocalID;
	std::string mCurrentRegionName;
	std::string mConnectorHandle;
	std::string mAccountHandle;
	int 		mNumberOfAliases;
	U32 mCommandCookie;
	std::string mVoiceAccountServerURI;
	std::string mVoiceSIPURIHostName;
	int mLoginRetryCount;
	sessionMap mSessionsByHandle;
	sessionSet mSessions;
	bool mBuddyListMapPopulated;
	bool mBlockRulesListReceived;
	bool mAutoAcceptRulesListReceived;
	buddyListMap mBuddyListMap;
	LLVoiceDeviceList mCaptureDevices;
	LLVoiceDeviceList mRenderDevices;
	std::string mCaptureDevice;
	std::string mRenderDevice;
	bool mCaptureDeviceDirty;
	bool mRenderDeviceDirty;
	bool mIsInitialized;
	bool checkParcelChanged(bool update = false);
	bool requestParcelVoiceInfo();
	void switchChannel(std::string uri = std::string(), bool spatial = true, bool no_reconnect = false, bool is_p2p = false, std::string hash = "");
	void joinSession(sessionState *session);
	std::string nameFromAvatar(LLVOAvatar *avatar);
	std::string nameFromID(const LLUUID &id);
	bool IDFromName(const std::string name, LLUUID &uuid);
	std::string displayNameFromAvatar(LLVOAvatar *avatar);
	std::string sipURIFromAvatar(LLVOAvatar *avatar);
	std::string sipURIFromName(std::string &name);
	std::string nameFromsipURI(const std::string &uri);
	bool inSpatialChannel(void);
	std::string getAudioSessionURI();
	std::string getAudioSessionHandle();
	void sendPositionalUpdate(void);
	void buildSetCaptureDevice(std::ostringstream &stream);
	void buildSetRenderDevice(std::ostringstream &stream);
	void sendFriendsListUpdates();
	sessionState* startUserIMSession(const LLUUID& uuid);
	void sendQueuedTextMessages(sessionState *session);
	void enforceTether(void);
	bool		mSpatialCoordsDirty;
	LLVector3d	mCameraPosition;
	LLVector3d	mCameraRequestedPosition;
	LLVector3	mCameraVelocity;
	LLMatrix3	mCameraRot;
	LLVector3d	mAvatarPosition;
	LLVector3	mAvatarVelocity;
	LLMatrix3	mAvatarRot;
	bool		mMuteMic;
	bool		mMuteMicDirty;
	bool		mFriendsListDirty;
	enum
	{
		earLocCamera = 0,
		earLocAvatar,
		earLocMixed,
		earLocSpeaker
	};
	S32			mEarLocation;
	bool		mSpeakerVolumeDirty;
	bool		mSpeakerMuteDirty;
	int			mSpeakerVolume;
	int			mMicVolume;
	bool		mMicVolumeDirty;
	bool		mVoiceEnabled;
	bool		mWriteInProgress;
	std::string mWriteString;
	size_t		mWriteOffset;
	LLTimer		mUpdateTimer;
	BOOL		mLipSyncEnabled;
	typedef std::set<LLVoiceClientParticipantObserver*> observer_set_t;
	observer_set_t mParticipantObservers;
	void notifyParticipantObservers();
	typedef std::set<LLVoiceClientStatusObserver*> status_observer_set_t;
	status_observer_set_t mStatusObservers;
	void notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status);
	typedef std::set<LLFriendObserver*> friend_observer_set_t;
	friend_observer_set_t mFriendObservers;
	void notifyFriendObservers();
	void expireVoiceFonts();
	void deleteVoiceFont(const LLUUID& id);
	void deleteAllVoiceFonts();
	void deleteVoiceFontTemplates();
	S32 getVoiceFontIndex(const LLUUID& id) const;
	S32 getVoiceFontTemplateIndex(const LLUUID& id) const;
	void accountGetSessionFontsSendMessage();
	void accountGetTemplateFontsSendMessage();
	void sessionSetVoiceFontSendMessage(sessionState *session);
	void notifyVoiceFontObservers();
	typedef enum e_voice_font_type
	{
		VOICE_FONT_TYPE_NONE = 0,
		VOICE_FONT_TYPE_ROOT = 1,
		VOICE_FONT_TYPE_USER = 2,
		VOICE_FONT_TYPE_UNKNOWN
	} EVoiceFontType;
	typedef enum e_voice_font_status
	{
		VOICE_FONT_STATUS_NONE = 0,
		VOICE_FONT_STATUS_FREE = 1,
		VOICE_FONT_STATUS_NOT_FREE = 2,
		VOICE_FONT_STATUS_UNKNOWN
	} EVoiceFontStatus;
	struct voiceFontEntry
	{
		voiceFontEntry(LLUUID& id);
		~voiceFontEntry();
		LLUUID		mID;
		S32			mFontIndex;
		std::string mName;
		LLDate		mExpirationDate;
		S32			mFontType;
		S32			mFontStatus;
		bool		mIsNew;
		LLFrameTimer	mExpiryTimer;
		LLFrameTimer	mExpiryWarningTimer;
	};
	bool mVoiceFontsReceived;
	bool mVoiceFontsNew;
	bool mVoiceFontListDirty;
	voice_effect_list_t	mVoiceFontList;
	voice_effect_list_t	mVoiceFontTemplateList;
	typedef std::map<const LLUUID, voiceFontEntry*> voice_font_map_t;
	voice_font_map_t	mVoiceFontMap;
	voice_font_map_t	mVoiceFontTemplateMap;
	typedef std::set<LLVoiceEffectObserver*> voice_font_observer_set_t;
	voice_font_observer_set_t mVoiceFontObservers;
	LLFrameTimer	mVoiceFontExpiryTimer;
	void captureBufferRecordStartSendMessage();
	void captureBufferRecordStopSendMessage();
	void captureBufferPlayStartSendMessage(const LLUUID& voice_font_id = LLUUID::null);
	void captureBufferPlayStopSendMessage();
	bool mCaptureBufferMode;
	bool mCaptureBufferRecording;
	bool mCaptureBufferRecorded;
	bool mCaptureBufferPlaying;
	bool mShutdownComplete;
	LLTimer	mCaptureTimer;
	LLUUID mPreviewVoiceFont;
	LLUUID mPreviewVoiceFontLast;
	S32 mPlayRequestCount;
};
class LLVivoxProtocolParser : public LLIOPipe
{
	LOG_CLASS(LLVivoxProtocolParser);
public:
	LLVivoxProtocolParser();
	virtual ~LLVivoxProtocolParser();
protected:
	virtual EStatus process_impl(
								 const LLChannelDescriptors& channels,
								 buffer_ptr_t& buffer,
								 bool& eos,
								 LLSD& context,
								 LLPumpIO* pump);
	std::string		mInput;
	XML_Parser		parser;
	int				responseDepth;
	bool			ignoringTags;
	bool			isEvent;
	int				ignoreDepth;
	bool			squelchDebugOutput;
	int				returnCode;
	int				statusCode;
	std::string		statusString;
	std::string		requestId;
	std::string		actionString;
	std::string		connectorHandle;
	std::string		versionID;
	std::string		accountHandle;
	std::string		sessionHandle;
	std::string		sessionGroupHandle;
	std::string		alias;
	std::string		applicationString;
	std::string		eventTypeString;
	int				state;
	std::string		uriString;
	bool			isChannel;
	bool			incoming;
	bool			enabled;
	std::string		nameString;
	std::string		audioMediaString;
	std::string		deviceString;
	std::string		displayNameString;
	int				participantType;
	bool			isLocallyMuted;
	bool			isModeratorMuted;
	bool			isSpeaking;
	int				volume;
	F32				energy;
	std::string		messageHeader;
	std::string		messageBody;
	std::string		notificationType;
	bool			hasText;
	bool			hasAudio;
	bool			hasVideo;
	bool			terminated;
	std::string		blockMask;
	std::string		presenceOnly;
	std::string		autoAcceptMask;
	std::string		autoAddAsBuddy;
	int				numberOfAliases;
	std::string		subscriptionHandle;
	std::string		subscriptionType;
	S32				id;
	std::string		descriptionString;
	LLDate			expirationDate;
	bool			hasExpired;
	S32				fontType;
	S32				fontStatus;
	std::string		mediaCompletionType;
	std::string		textBuffer;
	bool			accumulateText;
	void			reset();
	void			processResponse(std::string tag);
	static void XMLCALL ExpatStartTag(void *data, const char *el, const char **attr);
	static void XMLCALL ExpatEndTag(void *data, const char *el);
	static void XMLCALL ExpatCharHandler(void *data, const XML_Char *s, int len);
	void			StartTag(const char *tag, const char **attr);
	void			EndTag(const char *tag);
	void			CharData(const char *buffer, int length);
	LLDate			expiryTimeStampToLLDate(const std::string& vivox_ts);
};
#endif
