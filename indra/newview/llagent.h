/** 
 * @file llagent.h
 * @brief LLAgent class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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
#ifndef LL_LLAGENT_H
#define LL_LLAGENT_H
#include "indra_constants.h"
#include "llevent.h"
#include "llagentconstants.h"
#include "llagentdata.h"
#include "llcharacter.h"
#include "llcoordframe.h"
#include "llavatarappearancedefines.h"
#include "llviewerinventory.h"
#include "llinventorymodel.h"
#include "v3dmath.h"
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
extern const BOOL 	ANIMATE;
extern const U8 	AGENT_STATE_TYPING;
extern const U8 	AGENT_STATE_EDITING;
class LLChat;
class LLVOAvatar;
class LLViewerRegion;
class LLMotion;
class LLToolset;
class LLMessageSystem;
class LLPermissions;
class LLHost;
class LLFriendObserver;
class LLPickInfo;
class LLViewerObject;
class LLAgentDropGroupViewerNode;
class LLAgentAccess;
class LLSLURL;
class LLSimInfo;
class LLTeleportRequest;
struct LLCoroResponder;
typedef boost::shared_ptr<LLTeleportRequest> LLTeleportRequestPtr;
enum EAnimRequest
{
	ANIM_REQUEST_START,
	ANIM_REQUEST_STOP
};
struct LLGroupData
{
	LLUUID mID;
	LLUUID mInsigniaID;
	U64 mPowers;
	BOOL mAcceptNotices;
	BOOL mListInProfile;
	S32 mContribution;
	std::string mName;
};
class LLAgent final : public LLOldEvents::LLObservable
{
	LOG_CLASS(LLAgent);
public:
	friend class LLAgentDropGroupViewerNode;
public:
	LLAgent();
	virtual 		~LLAgent();
	void			init();
	void			cleanup();
private:
public:
	void			onAppFocusGained();
	void			setFirstLogin(BOOL b) 	{ mFirstLogin = b; }
	BOOL 			isFirstLogin() const 	{ return mFirstLogin; }
	BOOL 			isInitialized() const 	{ return mInitialized; }
public:
	std::string		mMOTD;
private:
	BOOL			mInitialized;
	BOOL			mFirstLogin;
public:
	const LLUUID&	getID() const				{ return gAgentID; }
	const LLUUID&	getSessionID() const		{ return gAgentSessionID; }
	const LLUUID&	getSecureSessionID() const	{ return mSecureSessionID; }
public:
	LLUUID			mSecureSessionID;
public:
	void			buildFullnameAndTitle(std::string &name) const;
public:
	BOOL 			isOutfitChosen() const 	{ return mOutfitChosen; }
	void			setOutfitChosen(BOOL b)	{ mOutfitChosen = b; }
private:
	BOOL			mOutfitChosen;
public:
	LLVector3		getPosAgentFromGlobal(const LLVector3d &pos_global) const;
	LLVector3d		getPosGlobalFromAgent(const LLVector3 &pos_agent)	const;
	const LLVector3d	&getPositionGlobal() const;
	const LLVector3	&getPositionAgent();
	void			updateAgentPosition(const F32 dt, const F32 yaw, const S32 mouse_x, const S32 mouse_y);
	void			setPositionAgent(const LLVector3 &center);
protected:
	void			propagate(const F32 dt);
private:
	mutable LLVector3d mPositionGlobal;
public:
	LLVector3		getVelocity() const;
	F32				getVelocityZ() const 	{ return getVelocity().mV[VZ]; }
public:
	const LLCoordFrame&	getFrameAgent()	const	{ return mFrameAgent; }
	void			initOriginGlobal(const LLVector3d &origin_global);
	void			resetAxes();
	void			resetAxes(const LLVector3 &look_at);
	const LLVector3& getAtAxis() const		{ return mFrameAgent.getAtAxis(); }
	const LLVector3& getUpAxis() const		{ return mFrameAgent.getUpAxis(); }
	const LLVector3& getLeftAxis() const	{ return mFrameAgent.getLeftAxis(); }
	LLQuaternion	getQuat() const;
private:
	LLVector3d		mAgentOriginGlobal;
	LLCoordFrame	mFrameAgent;
public:
	void			setStartPosition(U32 location_id);
	void			setHomePosRegion(const U64& region_handle, const LLVector3& pos_region);
	BOOL			getHomePosGlobal(LLVector3d* pos_global);
private:
	BOOL 			mHaveHomePosition;
	U64				mHomeRegionHandle;
	LLVector3		mHomePosRegion;
public:
	void changeParcels();
	typedef std::function<void()> parcel_changed_callback_t;
	boost::signals2::connection     addParcelChangedCallback(parcel_changed_callback_t);
private:
	typedef boost::signals2::signal<void()> parcel_changed_signal_t;
	parcel_changed_signal_t		mParcelChangedSignal;
public:
	void			setRegion(LLViewerRegion *regionp);
	LLViewerRegion	*getRegion() const;
	const LLHost&	getRegionHost() const;
	BOOL			inPrelude();
	std::string     getRegionCapability(const std::string &name);
	typedef boost::signals2::signal<void()> region_changed_signal_t;
	boost::signals2::connection     addRegionChangedCallback(const region_changed_signal_t::slot_type& cb);
	void                            removeRegionChangedCallback(boost::signals2::connection callback);
	struct SHLureRequest
	{
		SHLureRequest(const std::string& avatar_name, U64& handle, const U32 x, const U32 y, const U32 z) :
			mAvatarName(avatar_name), mRegionHandle(handle), mPosLocal(x,y,z) {}
		const std::string mAvatarName;
		const U64 mRegionHandle;
		LLVector3 mPosLocal;
	};
	SHLureRequest *mPendingLure;
	void showLureDestination(const std::string fromname, U64& handle, U32 x, U32 y, U32 z);
	void onFoundLureDestination(LLSimInfo *siminfo = NULL);
private:
	LLViewerRegion	*mRegionp;
	region_changed_signal_t		            mRegionChangedSignal;
public:
	S32				getRegionsVisited() const;
	F64				getDistanceTraveled() const;
	void			setDistanceTraveled(F64 dist) { mDistanceTraveled = dist; }
	const LLVector3d &getLastPositionGlobal() const { return mLastPositionGlobal; }
	void			setLastPositionGlobal(const LLVector3d &pos) { mLastPositionGlobal = pos; }
private:
	std::set<U64>	mRegionsVisited;
	F64				mDistanceTraveled;
	LLVector3d		mLastPositionGlobal;
public:
	void			fidget();
	static void		stopFidget();
private:
	LLFrameTimer	mFidgetTimer;
	LLFrameTimer	mFocusObjectFadeTimer;
	LLFrameTimer	mMoveTimer;
	F32				mNextFidgetTime;
	S32				mCurrentFidget;
public:
	bool isCrouching() const;
	void toggleCrouch() { mCrouch = !mCrouch; }
private:
	bool mCrouch;
public:
	BOOL			getFlying() const;
	void			setFlying(BOOL fly);
	static void		toggleFlying();
	static bool		enableFlying();
	BOOL			canFly();
public:
	bool			isVoiceConnected() const { return mVoiceConnected; }
	void			setVoiceConnected(const bool b)	{ mVoiceConnected = b; }
	static void		pressMicrophone(const LLSD& name);
	static void		releaseMicrophone(const LLSD& name);
	static void		toggleMicrophone(const LLSD& name);
	static bool		isMicrophoneOn(const LLSD& sdname);
	static bool		isActionAllowed(const LLSD& sdname);
private:
	bool			mVoiceConnected;
public:
	void			heardChat(const LLUUID& id);
	F32				getTypingTime() 		{ return mTypingTimer.getElapsedTimeF32(); }
	LLUUID			getLastChatter() const 	{ return mLastChatterID; }
	F32				getNearChatRadius() 	{ return mNearChatRadius; }
protected:
	void 			ageChat();
private:
	LLFrameTimer	mChatTimer;
	LLUUID			mLastChatterID;
	F32				mNearChatRadius;
public:
	void			startTyping();
	void			stopTyping();
public:
	static const F32 TYPING_TIMEOUT_SECS;
private:
	LLFrameTimer	mTypingTimer;
public:
	void			setAFK();
	void			clearAFK();
	BOOL			getAFK() const;
public:
	enum EDoubleTapRunMode
	{
		DOUBLETAP_NONE,
		DOUBLETAP_FORWARD,
		DOUBLETAP_BACKWARD,
		DOUBLETAP_SLIDELEFT,
		DOUBLETAP_SLIDERIGHT
	};
	void			setAlwaysRun();
	void			setTempRun();
	void			clearAlwaysRun();
	void			clearTempRun();
	void 			sendWalkRun();
	bool			getTempRun()			{ return mbTempRun; }
	bool			getRunning() const 		{ return (mbAlwaysRun) || (mbTempRun); }
	bool			getAlwaysRun() const 	{ return mbAlwaysRun; }
public:
	LLFrameTimer 	mDoubleTapRunTimer;
	EDoubleTapRunMode mDoubleTapRunMode;
private:
	bool 			mbAlwaysRun;
	bool 			mbTempRun;
	bool			mbTeleportKeepsLookAt;
public:
	void			standUp();
	void			sitDown();
	void			setSitDownAway(bool away);
	bool			isAwaySitting() const { return mIsAwaySitting; }
private:
	bool			mIsAwaySitting;
public:
	void			setDoNotDisturb(bool pIsDoNotDisturb);
	bool			isDoNotDisturb() const;
private:
	bool			mIsDoNotDisturb;
public:
	BOOL 			leftButtonGrabbed() const;
	BOOL 			leftButtonBlocked() const;
	BOOL 			rotateGrabbed() const;
	BOOL 			forwardGrabbed() const;
	BOOL 			backwardGrabbed() const;
	BOOL 			upGrabbed() const;
	BOOL 			downGrabbed() const;
public:
	U32 			getControlFlags();
	void 			setControlFlags(U32 mask);
	void 			clearControlFlags(U32 mask);
	BOOL			controlFlagsDirty() const;
	void			enableControlFlagReset();
	void 			resetControlFlags();
	BOOL			anyControlGrabbed() const;
	BOOL			isControlGrabbed(S32 control_index) const;
	BOOL			isControlBlocked(S32 control_index) const;
	void			forceReleaseControls();
	void			setFlagsDirty() { mbFlagsDirty = TRUE; }
private:
	S32				mControlsTakenCount[TOTAL_CONTROLS];
	S32				mControlsTakenPassedOnCount[TOTAL_CONTROLS];
	U32				mControlFlags;
	BOOL 			mbFlagsDirty;
	BOOL 			mbFlagsNeedReset;
public:
	void            stopCurrentAnimations();
	void			requestStopMotion(LLMotion* motion);
	void			onAnimStop(const LLUUID& id);
	void			sendAnimationRequests(const uuid_vec_t &anim_ids, EAnimRequest request);
	void			sendAnimationRequest(const LLUUID &anim_id, EAnimRequest request);
	void			sendAnimationStateReset();
	void			sendRevokePermissions(const LLUUID & target, U32 permissions);
	void			endAnimationUpdateUI();
	void			unpauseAnimation() { mPauseRequest = NULL; }
	BOOL			getCustomAnim() const { return mCustomAnim; }
	void			setCustomAnim(BOOL anim) { mCustomAnim = anim; }
	typedef boost::signals2::signal<void ()> camera_signal_t;
	boost::signals2::connection setMouselookModeInCallback( const camera_signal_t::slot_type& cb );
	boost::signals2::connection setMouselookModeOutCallback( const camera_signal_t::slot_type& cb );
private:
	camera_signal_t* mMouselookModeInSignal;
	camera_signal_t* mMouselookModeOutSignal;
	BOOL            mCustomAnim;
	LLPointer<LLPauseRequestHandle> mPauseRequest;
	BOOL			mViewsPushed;
public:
	void			moveAt(S32 direction, bool reset_view = true);
	void			moveAtNudge(S32 direction);
	void			moveLeft(S32 direction);
	void			moveLeftNudge(S32 direction);
	void			moveUp(S32 direction);
	void			moveYaw(F32 mag, bool reset_view = true);
	void			movePitch(F32 mag);
	BOOL			isMovementLocked() const				{ return mMovementKeysLocked; }
	void			setMovementLocked(BOOL set_locked)	{ mMovementKeysLocked = set_locked; }
public:
	void			rotate(F32 angle, const LLVector3 &axis);
	void			rotate(F32 angle, F32 x, F32 y, F32 z);
	void			rotate(const LLMatrix3 &matrix);
	void			rotate(const LLQuaternion &quaternion);
	void			pitch(F32 angle);
	void			roll(F32 angle);
	void			yaw(F32 angle);
	LLVector3		getReferenceUpVector();
    F32             clampPitchToLimits(F32 angle);
public:
	BOOL			getAutoPilot() const				{ return mAutoPilot; }
	LLVector3d		getAutoPilotTargetGlobal() const 	{ return mAutoPilotTargetGlobal; }
	const LLUUID&	getAutoPilotLeaderID() const		{ return mLeaderID; }
	F32				getAutoPilotStopDistance() const	{ return mAutoPilotStopDistance; }
	F32				getAutoPilotTargetDist() const		{ return mAutoPilotTargetDist; }
	BOOL			getAutoPilotUseRotation() const		{ return mAutoPilotUseRotation; }
	LLVector3		getAutoPilotTargetFacing() const	{ return mAutoPilotTargetFacing; }
	F32				getAutoPilotRotationThreshold() const	{ return mAutoPilotRotationThreshold; }
	const std::string&	getAutoPilotBehaviorName() const	{ return mAutoPilotBehaviorName; }
	bool			getAutoPilotNoProgress() const;
	void			startAutoPilotGlobal(const LLVector3d &pos_global,
										 const std::string& behavior_name = std::string(),
										 const LLQuaternion *target_rotation = NULL,
										 void (*finish_callback)(BOOL, void *) = NULL, void *callback_data = NULL,
										 F32 stop_distance = 0.f, F32 rotation_threshold = 0.03f,
										 BOOL allow_flying = TRUE);
	void 			startFollowPilot(const LLUUID &leader_id, BOOL allow_flying = TRUE, F32 stop_distance = 0.5f);
	void			stopAutoPilot(BOOL user_cancel = FALSE);
	void 			setAutoPilotTargetGlobal(const LLVector3d &target_global);
	void			autoPilot(F32 *delta_yaw);
	void			renderAutoPilotTarget();
private:
	BOOL			mAutoPilot;
	BOOL			mAutoPilotFlyOnStop;
	BOOL			mAutoPilotAllowFlying;
	LLVector3d		mAutoPilotTargetGlobal;
	F32				mAutoPilotStopDistance;
	BOOL			mAutoPilotUseRotation;
	LLVector3		mAutoPilotTargetFacing;
	F32				mAutoPilotTargetDist;
	U64				mAutoPilotNoProgressFrameCount;
	F32				mAutoPilotRotationThreshold;
	std::string		mAutoPilotBehaviorName;
	void			(*mAutoPilotFinishedCallback)(BOOL, void *);
	void*			mAutoPilotCallbackData;
	LLUUID			mLeaderID;
	BOOL			mMovementKeysLocked;
public:
	enum ETeleportState
	{
		TELEPORT_NONE = 0,
		TELEPORT_START = 1,
		TELEPORT_REQUESTED = 2,
		TELEPORT_MOVING = 3,
		TELEPORT_START_ARRIVAL = 4,
		TELEPORT_ARRIVING = 5,
		TELEPORT_LOCAL = 6,
		TELEPORT_PENDING = 7
	};
public:
	static void 	parseTeleportMessages(const std::string& xml_filename);
	const void getTeleportSourceSLURL(LLSLURL& slurl) const;
public:
	static std::map<std::string, std::string> sTeleportErrorMessages;
	static std::map<std::string, std::string> sTeleportProgressMessages;
private:
	LLSLURL * mTeleportSourceSLURL;
public:
	void 			teleportViaLandmark(const LLUUID& landmark_id);
	void 			teleportHome()	{ teleportViaLandmark(LLUUID::null); }
	void 			teleportViaLure(const LLUUID& lure_id, BOOL godlike);
	void 			teleportViaLocation(const LLVector3d& pos_global);
	void			teleportViaLocationLookAt(const LLVector3d& pos_global);
	void 			teleportCancel();
	void            restoreCanceledTeleportRequest();
	bool			getTeleportKeepsLookAt() { return mbTeleportKeepsLookAt; }
protected:
	bool 			teleportCore(bool is_local = false);
public:
	bool            hasRestartableFailedTeleportRequest();
	void            restartFailedTeleportRequest();
	void            clearTeleportRequest();
	void            setMaturityRatingChangeDuringTeleport(U8 pMaturityRatingChange);
private:
	friend class LLTeleportRequest;
	friend class LLTeleportRequestViaLandmark;
	friend class LLTeleportRequestViaLure;
	friend class LLTeleportRequestViaLocation;
	friend class LLTeleportRequestViaLocationLookAt;
	LLTeleportRequestPtr        mTeleportRequest;
	LLTeleportRequestPtr        mTeleportCanceled;
	boost::signals2::connection mTeleportFinishedSlot;
	boost::signals2::connection mTeleportFailedSlot;
	bool            mIsMaturityRatingChangingDuringTeleport;
	U8              mMaturityRatingChange;
	bool            hasPendingTeleportRequest();
	void            startTeleportRequest();
	void 			teleportRequest(const U64& region_handle,
									const LLVector3& pos_local,
									bool look_at_from_camera = false);
	void 			doTeleportViaLandmark(const LLUUID& landmark_id);
	void 			doTeleportViaLure(const LLUUID& lure_id, BOOL godlike);
	void 			doTeleportViaLocation(const LLVector3d& pos_global);
	void			doTeleportViaLocationLookAt(const LLVector3d& pos_global);
	void            handleTeleportFinished();
	void            handleTeleportFailed();
public:
	void			handleServerBakeRegionTransition(const LLUUID& region_id);
public:
	ETeleportState	getTeleportState() const;
	void			setTeleportState(ETeleportState state);
private:
	ETeleportState	mTeleportState;
public:
	const std::string& getTeleportMessage() const 					{ return mTeleportMessage; }
	void 			setTeleportMessage(const std::string& message) 	{ mTeleportMessage = message; }
private:
	std::string		mTeleportMessage;
public:
	void setIsCrossingRegion(bool is_crossing) { mIsCrossingRegion = is_crossing; }
	bool isCrossingRegion() const { return mIsCrossingRegion; }
private:
	bool mIsCrossingRegion;
public:
	bool			canEditParcel() const { return mCanEditParcel; }
private:
	static void     setCanEditParcel();
	bool			mCanEditParcel;
public:
	BOOL			isGrantedProxy(const LLPermissions& perm);
	BOOL			allowOperation(PermissionBit op,
								   const LLPermissions& perm,
								   U64 group_proxy_power = 0,
								   U8 god_minimum = GOD_MAINTENANCE);
	const LLAgentAccess& getAgentAccess();
	BOOL			canManageEstate() const;
	BOOL			getAdminOverride() const;
private:
	LLAgentAccess * mAgentAccess;
public:
	bool			isGodlike() const;
	bool			isGodlikeWithoutAdminMenuFakery() const;
	U8				getGodLevel() const;
	void			setAdminOverride(BOOL b);
	void			setGodLevel(U8 god_level);
	void			requestEnterGodMode();
	void			requestLeaveGodMode();
	typedef std::function<void (U8)>         god_level_change_callback_t;
	typedef boost::signals2::signal<void (U8)> god_level_change_signal_t;
	typedef boost::signals2::connection        god_level_change_slot_t;
	god_level_change_slot_t registerGodLevelChanageListener(god_level_change_callback_t pGodLevelChangeCallback);
private:
	god_level_change_signal_t mGodLevelChangeSignal;
public:
	bool 			wantsPGOnly() const;
	bool 			canAccessMature() const;
	bool 			canAccessAdult() const;
	bool 			canAccessMaturityInRegion( U64 region_handle ) const;
	bool 			canAccessMaturityAtGlobal( const LLVector3d& pos_global ) const;
	bool 			prefersPG() const;
	bool 			prefersMature() const;
	bool 			prefersAdult() const;
	bool 			isTeen() const;
	bool 			isMature() const;
	bool 			isAdult() const;
	void 			setTeen(bool teen);
	void 			setMaturity(char text);
	static int 		convertTextToMaturity(char text);
private:
	bool                            mIsDoSendMaturityPreferenceToServer;
	unsigned int                    mMaturityPreferenceRequestId;
	unsigned int                    mMaturityPreferenceResponseId;
	unsigned int                    mMaturityPreferenceNumRetries;
	U8                              mLastKnownRequestMaturity;
	U8                              mLastKnownResponseMaturity;
	bool            isMaturityPreferenceSyncedWithServer() const;
	void 			sendMaturityPreferenceToServer(U8 pPreferredMaturity);
	friend class LLMaturityPreferencesResponder;
	void            handlePreferredMaturityResult(U8 pServerMaturity);
	void            handlePreferredMaturityError();
	void            reportPreferredMaturitySuccess();
	void            reportPreferredMaturityError();
	void 			handleMaturity(const LLSD &pNewValue);
	bool 			validateMaturity(const LLSD& newvalue);
public:
	LLQuaternion	getHeadRotation();
	BOOL			needsRenderAvatar();
	BOOL			needsRenderHead();
	void			setShowAvatar(BOOL show) { mShowAvatar = show; }
	BOOL			getShowAvatar() const { return mShowAvatar; }
private:
	BOOL			mShowAvatar;
	U32				mAppearanceSerialNum;
public:
	void			setRenderState(U8 newstate);
	void			clearRenderState(U8 clearstate);
	U8				getRenderState();
private:
	U8				mRenderState;
public:
	const LLColor4	getEffectColor();
	void			setEffectColor(const LLColor4 &color);
private:
	LLColor4 *mEffectColor;
public:
	const LLUUID	&getGroupID() const			{ return mGroupID; }
	BOOL 			getGroupData(const LLUUID& group_id, LLGroupData& data) const;
	S32 			getGroupContribution(const LLUUID& group_id) const;
	BOOL 			setGroupContribution(const LLUUID& group_id, S32 contribution);
	BOOL 			setUserGroupFlags(const LLUUID& group_id, BOOL accept_notices, BOOL list_in_profile);
	const std::string &getGroupName() const 	{ return mGroupName; }
	BOOL			canJoinGroups() const;
private:
	std::string		mGroupName;
	LLUUID			mGroupID;
public:
	BOOL 			isInGroup(const LLUUID& group_id, BOOL ingnore_God_mod = FALSE) const;
protected:
	BOOL			isGroupMember() const 		{ return !mGroupID.isNull(); }
public:
	std::vector<LLGroupData> mGroups;
public:
	void			setHideGroupTitle(BOOL hide)	{ mHideGroupTitle = hide; }
	BOOL			isGroupTitleHidden() const 		{ return mHideGroupTitle; }
	const std::string& getGroupTitle() const		{ return mGroupTitle; }
private:
	std::string		mGroupTitle;
	BOOL			mHideGroupTitle;
public:
	BOOL 			hasPowerInGroup(const LLUUID& group_id, U64 power) const;
	BOOL 			hasPowerInActiveGroup(const U64 power) const;
	U64  			getPowerInGroup(const LLUUID& group_id) const;
 	U64				mGroupPowers;
public:
	void 			observeFriends();
	void 			friendsChanged();
private:
	LLFriendObserver* mFriendObserver;
	uuid_set_t mProxyForAgents;
public:
	void			sendMessage();
	void			sendReliableMessage();
	void 			dumpSentAppearance(const std::string& dump_prefix);
	void			sendAgentSetAppearance();
	void 			sendAgentDataUpdateRequest();
	void 			sendAgentUserInfoRequest();
	void			sendAgentUpdateUserInfo(bool im_to_email, const std::string& directory_visibility);
private:
    void            requestAgentUserInfoCoro(const LLCoroResponder& responder);
    void            updateAgentUserInfoCoro(const LLCoroResponder& responder);
    void 			sendAgentUserInfoRequestMessage();
    void            sendAgentUpdateUserInfoMessage(bool im_via_email, const std::string& directory_visibility);
public:
	static void		processAgentDataUpdate(LLMessageSystem *msg, void **);
	static void		processAgentGroupDataUpdate(LLMessageSystem *msg, void **);
	static void		processAgentDropGroup(LLMessageSystem *msg, void **);
	static void		processScriptControlChange(LLMessageSystem *msg, void **);
	static void		processAgentCachedTextureResponse(LLMessageSystem *mesgsys, void **user_data);
public:
	void			dumpGroupInfo();
	static void		clearVisualParams(void *);
	friend std::ostream& operator<<(std::ostream &s, const LLAgent &sphere);
};
extern LLAgent gAgent;
inline bool operator==(const LLGroupData &a, const LLGroupData &b)
{
	return (a.mID == b.mID);
}
class LLAgentQueryManager
{
	friend class LLAgent;
	friend class LLAgentWearables;
public:
	LLAgentQueryManager();
	virtual ~LLAgentQueryManager();
	BOOL 			hasNoPendingQueries() const 	{ return getNumPendingQueries() == 0; }
	S32 			getNumPendingQueries() const 	{ return mNumPendingQueries; }
private:
	S32				mNumPendingQueries;
	S32				mWearablesCacheQueryID;
	U32				mUpdateSerialNum;
	S32		    	mActiveCacheQueries[LLAvatarAppearanceDefines::BAKED_NUM_INDICES];
};
extern LLAgentQueryManager gAgentQueryManager;
extern std::string gAuthString;
void update_group_floaters(const LLUUID& group_id);
#endif
