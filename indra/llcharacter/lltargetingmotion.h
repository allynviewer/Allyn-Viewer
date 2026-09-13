/** 
 * @file lltargetingmotion.h
 * @brief Implementation of LLTargetingMotion class.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LL_LLTARGETINGMOTION_H
#define LL_LLTARGETINGMOTION_H
#include "llmotion.h"
#define TARGETING_EASEIN_DURATION	0.3f
#define TARGETING_EASEOUT_DURATION 0.5f
#define TARGETING_PRIORITY LLJoint::HIGH_PRIORITY
#define MIN_REQUIRED_PIXEL_AREA_TARGETING 1000.f;
class LLTargetingMotion :
	public AIMaskedMotion
{
public:
	LLTargetingMotion(LLUUID const& id, LLMotionController* controller);
	virtual ~LLTargetingMotion();
public:
	static LLMotion* create(LLUUID const& id, LLMotionController* controller) { return new LLTargetingMotion(id, controller); }
public:
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.0; }
	virtual F32 getEaseInDuration() { return TARGETING_EASEIN_DURATION; }
	virtual F32 getEaseOutDuration() { return TARGETING_EASEOUT_DURATION; }
	virtual LLJoint::JointPriority getPriority() { return TARGETING_PRIORITY; }
	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_TARGETING; }
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
public:
	LLCharacter			*mCharacter;
	LLPointer<LLJointState>	mTorsoState;
	LLJoint*			mPelvisJoint;
	LLJoint*			mTorsoJoint;
	LLJoint*			mRightHandJoint;
};
#endif
