/** 
 * @file llagent.h
 * @brief LLAgent class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_LLAGENTCAMERA_H
#define LL_LLAGENTCAMERA_H
#include "llfollowcam.h"
#include "llhudeffectlookat.h"
#include "llhudeffectpointat.h"
class LLPickInfo;
class LLVOAvatarSelf;
class LLControlVariable;
enum ECameraMode
{
	CAMERA_MODE_THIRD_PERSON,
	CAMERA_MODE_MOUSELOOK,
	CAMERA_MODE_CUSTOMIZE_AVATAR,
	CAMERA_MODE_FOLLOW
};
enum ECameraPreset
{
	CAMERA_PRESET_REAR_VIEW,
	CAMERA_PRESET_FRONT_VIEW,
	CAMERA_PRESET_GROUP_VIEW
};
class LLAgentCamera
{
	LOG_CLASS(LLAgentCamera);
public:
public:
	LLAgentCamera();
	virtual 		~LLAgentCamera();
	void			init();
	void			cleanup();
	void		    setAvatarObject(LLVOAvatarSelf* avatar);
	bool			isInitialized() { return mInitialized; }
private:
	bool			mInitialized;
public:
	void			changeCameraToDefault();
	void			changeCameraToMouselook(BOOL animate = TRUE);
	void			changeCameraToThirdPerson(BOOL animate = TRUE);
	void			changeCameraToCustomizeAvatar();
	F32				calcCustomizeAvatarUIOffset( const LLVector3d& camera_pos_global );
	void			changeCameraToFollow(BOOL animate = TRUE);
	BOOL			cameraThirdPerson() const		{ return (mCameraMode == CAMERA_MODE_THIRD_PERSON && mLastCameraMode == CAMERA_MODE_THIRD_PERSON); }
	BOOL			cameraMouselook() const			{ return (mCameraMode == CAMERA_MODE_MOUSELOOK && mLastCameraMode == CAMERA_MODE_MOUSELOOK); }
	BOOL			cameraCustomizeAvatar() const	{ return (mCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR ); }
	BOOL			cameraFollow() const			{ return (mCameraMode == CAMERA_MODE_FOLLOW && mLastCameraMode == CAMERA_MODE_FOLLOW); }
	ECameraMode		getCameraMode() const 			{ return mCameraMode; }
	ECameraMode		getLastCameraMode() const 		{ return mLastCameraMode; }
	void			updateCamera();
	void			resetCamera();
	void			updateLastCamera();
private:
	ECameraMode		mCameraMode;
	ECameraMode		mLastCameraMode;
	F32				mUIOffset;
public:
	void switchCameraPreset(ECameraPreset preset);
	void resetPresetOffsets();
private:
	LLVector3 getCameraOffsetInitial();
	ECameraPreset mCameraPreset;
	std::map<ECameraPreset, LLPointer<LLControlVariable> > mCameraOffsetInitial;
	std::map<ECameraPreset, LLPointer<LLControlVariable> > mFocusOffsetInitial;
public:
	LLVector3d		getCameraPositionGlobal() const;
	const LLVector3 &getCameraPositionAgent() const;
	LLVector3d		calcCameraPositionTargetGlobal(BOOL *hit_limit = NULL);
	F32				getCameraMinOffGround();
	void			setCameraCollidePlane(const LLVector4 &plane) { mCameraCollidePlane = plane; }
	BOOL			calcCameraMinDistance(F32 &obj_min_distance);
	F32				getCurrentCameraBuildOffset() 	{ return (F32)mCameraFocusOffset.length(); }
	void			clearCameraLag() { mCameraLag.clearVec(); }
private:
	F32				mCurrentCameraDistance;
	F32				mTargetCameraDistance;
	F32				mCameraFOVZoomFactor;
	F32				mCameraCurrentFOVZoomFactor;
	F32				mCameraFOVDefault;
	LLVector4		mCameraCollidePlane;
	F32				mCameraZoomFraction;
	LLVector3		mCameraPositionAgent;
	LLVector3		mCameraVirtualPositionAgent;
	LLVector3d      mCameraSmoothingLastPositionGlobal;
	LLVector3d      mCameraSmoothingLastPositionAgent;
	bool            mCameraSmoothingStop;
	LLVector3		mCameraLag;
	LLVector3		mCameraUpVector;
public:
	void			setUsingFollowCam(bool using_follow_cam);
	bool 			isfollowCamLocked();
private:
	LLFollowCam 	mFollowCam;
public:
	void			setupSitCamera();
	BOOL			sitCameraEnabled() 		{ return mSitCameraEnabled; }
	void			setSitCamera(const LLUUID &object_id,
								 const LLVector3 &camera_pos = LLVector3::zero, const LLVector3 &camera_focus = LLVector3::zero);
private:
	LLPointer<LLViewerObject> mSitCameraReferenceObject;
	BOOL			mSitCameraEnabled;
	LLVector3		mSitCameraPos;
	LLVector3		mSitCameraFocus;
public:
	void			setCameraAnimating(BOOL b)			{ mCameraAnimating = b; }
	BOOL			getCameraAnimating()				{ return mCameraAnimating; }
	void			setAnimationDuration(F32 seconds);
	void			startCameraAnimation();
	void			stopCameraAnimation();
private:
	LLFrameTimer	mAnimationTimer;
	F32				mAnimationDuration;
	BOOL			mCameraAnimating;
	LLVector3d		mAnimationCameraStartGlobal;
	LLVector3d		mAnimationFocusStartGlobal;
public:
	LLVector3d		calcFocusPositionTargetGlobal();
	LLVector3		calcFocusOffset(LLViewerObject *object, LLVector3 pos_agent, S32 x, S32 y);
	BOOL			getFocusOnAvatar() const		{ return mFocusOnAvatar; }
	LLPointer<LLViewerObject>&	getFocusObject() 	{ return mFocusObject; }
	F32				getFocusObjectDist() const		{ return mFocusObjectDist; }
	void			updateFocusOffset();
	void			validateFocusObject();
	void			setFocusGlobal(const LLPickInfo& pick);
	void			setFocusGlobal(const LLVector3d &focus, const LLUUID &object_id = LLUUID::null);
	void			setFocusOnAvatar(BOOL focus, BOOL animate);
	void			setCameraPosAndFocusGlobal(const LLVector3d& pos, const LLVector3d& focus, const LLUUID &object_id);
	void			clearFocusObject();
	void			setFocusObject(LLViewerObject* object);
	void			setAllowChangeToFollow(BOOL focus) 	{ mAllowChangeToFollow = focus; }
	void			setObjectTracking(BOOL track) 	{ mTrackFocusObject = track; }
	const LLVector3d &getFocusGlobal() const		{ return mFocusGlobal; }
	const LLVector3d &getFocusTargetGlobal() const	{ return mFocusTargetGlobal; }
private:
	LLVector3d		mCameraFocusOffset;
	LLVector3d		mCameraFocusOffsetTarget;
	BOOL			mFocusOnAvatar;
	BOOL			mAllowChangeToFollow;
	LLVector3d		mFocusGlobal;
	LLVector3d		mFocusTargetGlobal;
	LLPointer<LLViewerObject> mFocusObject;
	F32				mFocusObjectDist;
	LLVector3		mFocusObjectOffset;
	F32				mFocusDotRadius;
	BOOL			mTrackFocusObject;
public:
	void			updateLookAt(const S32 mouse_x, const S32 mouse_y);
	BOOL			setLookAt(ELookAtType target_type, LLViewerObject *object = NULL, LLVector3 position = LLVector3::zero);
	ELookAtType		getLookAtType();
	void			lookAtLastChat();
	bool			lookAtObject(const LLUUID &object_id, bool self=true);
	void 			slamLookAt(const LLVector3 &look_at);
	BOOL			setPointAt(EPointAtType target_type, LLViewerObject *object = NULL, LLVector3 position = LLVector3::zero);
	EPointAtType	getPointAtType();
public:
	LLPointer<LLHUDEffectLookAt> mLookAt;
	LLPointer<LLHUDEffectPointAt> mPointAt;
public:
	LLVector3d		calcThirdPersonFocusOffset();
	void			setThirdPersonHeadOffset(LLVector3 offset) 	{ mThirdPersonHeadOffset = offset; }
private:
	LLVector3		mThirdPersonHeadOffset;
public:
	void			cameraOrbitAround(const F32 radians);
	void			cameraOrbitOver(const F32 radians);
	void			cameraOrbitIn(const F32 meters);
public:
	void			handleScrollWheel(S32 clicks);
	void			cameraZoomIn(const F32 factor);
	F32				getCameraZoomFraction();
	void			setCameraZoomFraction(F32 fraction);
	F32				calcCameraFOVZoomFactor();
public:
	void			cameraPanIn(const F32 meters);
	void			cameraPanLeft(const F32 meters);
	void			cameraPanUp(const F32 meters);
public:
	void			resetView(BOOL reset_camera = TRUE, BOOL change_camera = FALSE);
	void			unlockView();
public:
	F32				mDrawDistance;
public:
	BOOL			getForceMouselook() const 			{ return mForceMouselook; }
	void			setForceMouselook(BOOL mouselook) 	{ mForceMouselook = mouselook; }
private:
	BOOL			mForceMouselook;
public:
	F32				mHUDTargetZoom;
	F32				mHUDCurZoom;
public:
	S32				getAtKey() const		{ return mAtKey; }
	S32				getWalkKey() const		{ return mWalkKey; }
	S32				getLeftKey() const		{ return mLeftKey; }
	S32				getUpKey() const		{ return mUpKey; }
	F32				getYawKey() const		{ return mYawKey; }
	F32				getPitchKey() const		{ return mPitchKey; }
	void			setAtKey(S32 mag)		{ mAtKey = mag; }
	void			setWalkKey(S32 mag)		{ mWalkKey = mag; }
	void			setLeftKey(S32 mag)		{ mLeftKey = mag; }
	void			setUpKey(S32 mag)		{ mUpKey = mag; }
	void			setYawKey(F32 mag)		{ mYawKey = mag; }
	void			setPitchKey(F32 mag)	{ mPitchKey = mag; }
	void			clearGeneralKeys();
	static S32		directionToKey(S32 direction);
private:
	S32 			mAtKey;
	S32				mWalkKey;
	S32 			mLeftKey;
	S32				mUpKey;
	F32				mYawKey;
	F32				mPitchKey;
public:
	F32				getOrbitLeftKey() const		{ return mOrbitLeftKey; }
	F32				getOrbitRightKey() const	{ return mOrbitRightKey; }
	F32				getOrbitUpKey() const		{ return mOrbitUpKey; }
	F32				getOrbitDownKey() const		{ return mOrbitDownKey; }
	F32				getOrbitInKey() const		{ return mOrbitInKey; }
	F32				getOrbitOutKey() const		{ return mOrbitOutKey; }
	void			setOrbitLeftKey(F32 mag)	{ mOrbitLeftKey = mag; }
	void			setOrbitRightKey(F32 mag)	{ mOrbitRightKey = mag; }
	void			setOrbitUpKey(F32 mag)		{ mOrbitUpKey = mag; }
	void			setOrbitDownKey(F32 mag)	{ mOrbitDownKey = mag; }
	void			setOrbitInKey(F32 mag)		{ mOrbitInKey = mag; }
	void			setOrbitOutKey(F32 mag)		{ mOrbitOutKey = mag; }
	void			clearOrbitKeys();
private:
	F32				mOrbitLeftKey;
	F32				mOrbitRightKey;
	F32				mOrbitUpKey;
	F32				mOrbitDownKey;
	F32				mOrbitInKey;
	F32				mOrbitOutKey;
public:
	F32				getPanLeftKey() const		{ return mPanLeftKey; }
	F32				getPanRightKey() const	{ return mPanRightKey; }
	F32				getPanUpKey() const		{ return mPanUpKey; }
	F32				getPanDownKey() const		{ return mPanDownKey; }
	F32				getPanInKey() const		{ return mPanInKey; }
	F32				getPanOutKey() const		{ return mPanOutKey; }
	void			setPanLeftKey(F32 mag)		{ mPanLeftKey = mag; }
	void			setPanRightKey(F32 mag)		{ mPanRightKey = mag; }
	void			setPanUpKey(F32 mag)		{ mPanUpKey = mag; }
	void			setPanDownKey(F32 mag)		{ mPanDownKey = mag; }
	void			setPanInKey(F32 mag)		{ mPanInKey = mag; }
	void			setPanOutKey(F32 mag)		{ mPanOutKey = mag; }
	void			clearPanKeys();
private:
	F32				mPanUpKey;
	F32				mPanDownKey;
	F32				mPanLeftKey;
	F32				mPanRightKey;
	F32				mPanInKey;
	F32				mPanOutKey;
public:
	F32				getMouseLookDuration()	const { return mMouselookTimer.getElapsedTimeF32(); }
private:
	LLTimer			mMouselookTimer;
};
extern LLAgentCamera gAgentCamera;
#endif
