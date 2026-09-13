/** 
 * @file llheadrotmotion.h
 * @brief Implementation of LLHeadRotMotion class.
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
#ifndef LL_LLHEADROTMOTION_H
#define LL_LLHEADROTMOTION_H
#include "llmotion.h"
#include "llframetimer.h"
#define MIN_REQUIRED_PIXEL_AREA_HEAD_ROT 500.f;
#define MIN_REQUIRED_PIXEL_AREA_EYE 25000.f;
class LLHeadRotMotion :
	public AIMaskedMotion
{
public:
	LLHeadRotMotion(LLUUID const& id, LLMotionController* controller);
	virtual ~LLHeadRotMotion();
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLHeadRotMotion(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return 1.f; }
	virtual F32 getEaseOutDuration() { return 1.f; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_HEAD_ROT; }
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
public:
	LLCharacter			*mCharacter;
	LLJoint				*mTorsoJoint;
	LLJoint				*mHeadJoint;
	LLJoint				*mRootJoint;
	LLJoint				*mPelvisJoint;
	LLPointer<LLJointState> mTorsoState;
	LLPointer<LLJointState> mNeckState;
	LLPointer<LLJointState> mHeadState;
	LLQuaternion		mLastHeadRot;
};
class LLEyeMotion :
	public AIMaskedMotion
{
public:
	LLEyeMotion(LLUUID const& id, LLMotionController* controller);
	virtual ~LLEyeMotion();
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLEyeMotion(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return 0.5f; }
	virtual F32 getEaseOutDuration() { return 0.5f; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EYE; }
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	void adjustEyeTarget(LLVector3* targetPos, LLJointState& left_eye_state, LLJointState& right_eye_state);
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
	virtual void onDeactivate();
public:
	LLCharacter			*mCharacter;
	LLJoint				*mHeadJoint;
	LLPointer<LLJointState> mLeftEyeState;
	LLPointer<LLJointState> mRightEyeState;
	LLPointer<LLJointState> mAltLeftEyeState;
	LLPointer<LLJointState> mAltRightEyeState;
	LLFrameTimer		mEyeJitterTimer;
	F32					mEyeJitterTime;
	F32					mEyeJitterYaw;
	F32					mEyeJitterPitch;
	F32					mEyeLookAwayTime;
	F32					mEyeLookAwayYaw;
	F32					mEyeLookAwayPitch;
	LLFrameTimer		mEyeBlinkTimer;
	F32					mEyeBlinkTime;
	BOOL				mEyesClosed;
};
#endif
