/** 
 * @file lleditingmotion.h
 * @brief Implementation of LLEditingMotion class.
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
#ifndef LL_LLEDITINGMOTION_H
#define LL_LLEDITINGMOTION_H
#include "llmotion.h"
#include "lljointsolverrp3.h"
#include "v3dmath.h"
#define EDITING_EASEIN_DURATION	0.0f
#define EDITING_EASEOUT_DURATION 0.5f
#define EDITING_PRIORITY LLJoint::HIGH_PRIORITY
#define MIN_REQUIRED_PIXEL_AREA_EDITING 500.f
class LLEditingMotion :
	public AIMaskedMotion
{
public:
	LLEditingMotion(LLUUID const& id, LLMotionController* controller);
	virtual ~LLEditingMotion();
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLEditingMotion(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return EDITING_EASEIN_DURATION; }
	virtual F32 getEaseOutDuration() { return EDITING_EASEOUT_DURATION; }
	virtual LLJoint::JointPriority getPriority() { return EDITING_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EDITING; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onActivate();
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
public:
	LLCharacter			*mCharacter;
	LLVector3			mWristOffset;
	LLPointer<LLJointState> mParentState;
	LLPointer<LLJointState> mShoulderState;
	LLPointer<LLJointState> mElbowState;
	LLPointer<LLJointState> mWristState;
	LLPointer<LLJointState> mTorsoState;
	LLJoint				mParentJoint;
	LLJoint				mShoulderJoint;
	LLJoint				mElbowJoint;
	LLJoint				mWristJoint;
	LLJoint				mTarget;
	LLJointSolverRP3	mIKSolver;
	static S32			sHandPose;
	static S32			sHandPosePriority;
	LLVector3			mLastSelectPt;
};
#endif
