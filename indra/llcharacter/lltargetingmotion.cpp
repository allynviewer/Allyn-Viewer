/** 
 * @file lltargetingmotion.cpp
 * @brief Implementation of LLTargetingMotion class.
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
#include "linden_common.h"
#include "lltargetingmotion.h"
#include "llcharacter.h"
#include "v3dmath.h"
#include "llcriticaldamp.h"
const F32 TORSO_TARGET_HALF_LIFE = 0.25f;
const F32 MAX_TIME_DELTA = 2.f;
const F32 TARGET_PLANE_THRESHOLD_DOT = 0.6f;
const F32 TORSO_ROT_FRACTION = 0.5f;
LLTargetingMotion::LLTargetingMotion(LLUUID const& id, LLMotionController* controller) : AIMaskedMotion(id, controller, ANIM_AGENT_TARGET)
{
	mCharacter = NULL;
	mName = "targeting";
	mTorsoState = new LLJointState;
}
LLTargetingMotion::~LLTargetingMotion()
{
}
LLMotion::LLMotionInitStatus LLTargetingMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	mPelvisJoint = mCharacter->getJoint("mPelvis");
	mTorsoJoint = mCharacter->getJoint("mTorso");
	mRightHandJoint = mCharacter->getJoint("mWristRight");
	if (!mPelvisJoint ||
		!mTorsoJoint ||
		!mRightHandJoint)
	{
		LL_WARNS() << "Invalid skeleton for targeting motion!" << LL_ENDL;
		return STATUS_FAILURE;
	}
	mTorsoState->setJoint( mTorsoJoint );
	mTorsoState->setUsage(LLJointState::ROT);
	addJointState( mTorsoState );
	return STATUS_SUCCESS;
}
BOOL LLTargetingMotion::onUpdate(F32 time, U8* joint_mask)
{
	F32 slerp_amt = LLSmoothInterpolation::getInterpolant(TORSO_TARGET_HALF_LIFE);
	LLVector3 target;
	LLVector3* lookAtPoint = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");
	BOOL result = TRUE;
	if (!lookAtPoint)
	{
		return TRUE;
	}
	else
	{
		target = *lookAtPoint;
		target.normVec();
	}
	LLVector3 skyward(0.f, 0.f, 1.f);
	LLVector3 left(skyward % target);
	left.normVec();
	LLVector3 up(target % left);
	up.normVec();
	LLQuaternion target_aim_rot(target, left, up);
	LLQuaternion cur_torso_rot = mTorsoJoint->getWorldRotation();
	LLVector3 right_hand_at = LLVector3(0.f, -1.f, 0.f) * mRightHandJoint->getWorldRotation();
	left.setVec(skyward % right_hand_at);
	left.normVec();
	up.setVec(right_hand_at % left);
	up.normVec();
	LLQuaternion right_hand_rot(right_hand_at, left, up);
	LLQuaternion new_torso_rot = (cur_torso_rot * ~right_hand_rot) * target_aim_rot;
	new_torso_rot = new_torso_rot * ~cur_torso_rot;
	new_torso_rot = nlerp(slerp_amt, mTorsoState->getRotation(), new_torso_rot);
	LLQuaternion total_rot = new_torso_rot * mTorsoJoint->getRotation();
	total_rot.constrain(F_PI_BY_TWO * 0.8f);
	new_torso_rot = total_rot * ~mTorsoJoint->getRotation();
	mTorsoState->setRotation(new_torso_rot);
	return result;
}
