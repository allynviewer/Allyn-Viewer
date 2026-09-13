/** 
 * @file llheadrotmotion.cpp
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
#include "linden_common.h"
#include "llheadrotmotion.h"
#include "llcharacter.h"
#include "llrand.h"
#include "m3math.h"
#include "v3dmath.h"
#include "llcriticaldamp.h"
const F32 TORSO_LAG	= 0.35f;
const F32 NECK_LAG = 0.5f;
const F32 HEAD_LOOKAT_LAG_HALF_LIFE	= 0.15f;
const F32 TORSO_LOOKAT_LAG_HALF_LIFE	= 0.27f;
const F32 EYE_LOOKAT_LAG_HALF_LIFE = 0.06f;
const F32 HEAD_ROTATION_CONSTRAINT = F_PI_BY_TWO * 0.8f;
const F32 MIN_HEAD_LOOKAT_DISTANCE = 0.3f;
const F32 MAX_TIME_DELTA = 2.f;
const F32 EYE_JITTER_MIN_TIME = 0.3f;
const F32 EYE_JITTER_MAX_TIME = 2.5f;
const F32 EYE_JITTER_MAX_YAW = 0.08f;
const F32 EYE_JITTER_MAX_PITCH = 0.015f;
const F32 EYE_LOOK_AWAY_MIN_TIME = 5.f;
const F32 EYE_LOOK_AWAY_MAX_TIME = 15.f;
const F32 EYE_LOOK_BACK_MIN_TIME = 1.f;
const F32 EYE_LOOK_BACK_MAX_TIME = 5.f;
const F32 EYE_LOOK_AWAY_MAX_YAW = 0.15f;
const F32 EYE_LOOK_AWAY_MAX_PITCH = 0.12f;
const F32 EYE_ROT_LIMIT_ANGLE = F_PI_BY_TWO * 0.3f;
const F32 EYE_BLINK_MIN_TIME = 0.5f;
const F32 EYE_BLINK_MAX_TIME = 8.f;
const F32 EYE_BLINK_CLOSE_TIME = 0.03f;
const F32 EYE_BLINK_SPEED = 0.015f;
const F32 EYE_BLINK_TIME_DELTA = 0.005f;
LLHeadRotMotion::LLHeadRotMotion(LLUUID const& id, LLMotionController* controller) :
	AIMaskedMotion(id, controller, ANIM_AGENT_HEAD_ROT),
	mCharacter(NULL),
	mTorsoJoint(NULL),
	mHeadJoint(NULL),
	mRootJoint(NULL),
	mPelvisJoint(NULL),
	mHeadState(new LLJointState),
	mTorsoState(new LLJointState),
	mNeckState(new LLJointState)
{
	mName = "head_rot";
}
LLHeadRotMotion::~LLHeadRotMotion()
{
}
LLMotion::LLMotionInitStatus LLHeadRotMotion::onInitialize(LLCharacter *character)
{
	if (!character)
	{
		LL_WARNS() << "character is NULL." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mCharacter = character;
	mPelvisJoint = character->getJoint("mPelvis");
	if ( ! mPelvisJoint )
	{
		LL_INFOS() << getName() << ": Can't get pelvis joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mRootJoint = character->getJoint("mRoot");
	if ( ! mRootJoint )
	{
		LL_INFOS() << getName() << ": Can't get root joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mTorsoJoint = character->getJoint("mTorso");
	if ( ! mTorsoJoint )
	{
		LL_INFOS() << getName() << ": Can't get torso joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mHeadJoint = character->getJoint("mHead");
	if ( ! mHeadJoint )
	{
		LL_INFOS() << getName() << ": Can't get head joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mTorsoState->setJoint( character->getJoint("mTorso") );
	if ( ! mTorsoState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get torso joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mNeckState->setJoint( character->getJoint("mNeck") );
	if ( ! mNeckState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get neck joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mHeadState->setJoint( character->getJoint("mHead") );
	if ( ! mHeadState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get head joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mTorsoState->setUsage(LLJointState::ROT);
	mNeckState->setUsage(LLJointState::ROT);
	mHeadState->setUsage(LLJointState::ROT);
	addJointState( mTorsoState );
	addJointState( mNeckState );
	addJointState( mHeadState );
	mLastHeadRot.loadIdentity();
	return STATUS_SUCCESS;
}
BOOL LLHeadRotMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLQuaternion	targetHeadRotWorld;
	LLQuaternion	currentRootRotWorld = mRootJoint->getWorldRotation();
	LLQuaternion	currentInvRootRotWorld = ~currentRootRotWorld;
	F32 head_slerp_amt = LLSmoothInterpolation::getInterpolant(HEAD_LOOKAT_LAG_HALF_LIFE);
	F32 torso_slerp_amt = LLSmoothInterpolation::getInterpolant(TORSO_LOOKAT_LAG_HALF_LIFE);
	LLVector3* targetPos = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");
	if (targetPos)
	{
		LLVector3 headLookAt = *targetPos;
		F32 lookatDistance = headLookAt.normVec();
		if (lookatDistance < MIN_HEAD_LOOKAT_DISTANCE)
		{
			targetHeadRotWorld = mPelvisJoint->getWorldRotation();
		}
		else
		{
			LLVector3 root_up = LLVector3(0.f, 0.f, 1.f) * currentRootRotWorld;
			LLVector3 left(root_up % headLookAt);
			if (left.magVecSquared() < 0.15f)
			{
				LLVector3 root_at = LLVector3(1.f, 0.f, 0.f) * currentRootRotWorld;
				root_at.mV[VZ] = 0.f;
				root_at.normVec();
				headLookAt = lerp(headLookAt, root_at, 0.4f);
				headLookAt.normVec();
				left = root_up % headLookAt;
			}
			LLVector3 up(headLookAt % left);
			targetHeadRotWorld = LLQuaternion(headLookAt, left, up);
		}
	}
	else
	{
		targetHeadRotWorld = currentRootRotWorld;
	}
	LLQuaternion head_rot_local = targetHeadRotWorld * currentInvRootRotWorld;
	head_rot_local.constrain(HEAD_ROTATION_CONSTRAINT);
	LLQuaternion torso_rot_local = nlerp(TORSO_LAG, LLQuaternion::DEFAULT, head_rot_local );
	mTorsoState->setRotation( nlerp(torso_slerp_amt, mTorsoState->getRotation(), torso_rot_local) );
	head_rot_local = nlerp(head_slerp_amt, mLastHeadRot, head_rot_local);
	mLastHeadRot = head_rot_local;
	if(mNeckState->getJoint() && mNeckState->getJoint()->getParent())
	{
		LLQuaternion torsoRotLocal =  mNeckState->getJoint()->getParent()->getWorldRotation() * currentInvRootRotWorld;
		head_rot_local = head_rot_local * ~torsoRotLocal;
		mNeckState->setRotation( nlerp(NECK_LAG, LLQuaternion::DEFAULT, head_rot_local) );
		mHeadState->setRotation( nlerp(1.f - NECK_LAG, LLQuaternion::DEFAULT, head_rot_local));
	}
	return TRUE;
}
LLEyeMotion::LLEyeMotion(LLUUID const& id, LLMotionController* controller) : AIMaskedMotion(id, controller, ANIM_AGENT_EYE)
{
	mCharacter = NULL;
	mEyeJitterTime = 0.f;
	mEyeJitterYaw = 0.f;
	mEyeJitterPitch = 0.f;
	mEyeLookAwayTime = 0.f;
	mEyeLookAwayYaw = 0.f;
	mEyeLookAwayPitch = 0.f;
	mEyeBlinkTime = 0.f;
	mEyesClosed = FALSE;
	mHeadJoint = NULL;
	mName = "eye_rot";
	mLeftEyeState = new LLJointState;
	mAltLeftEyeState = new LLJointState;
	mRightEyeState = new LLJointState;
	mAltRightEyeState = new LLJointState;
}
LLEyeMotion::~LLEyeMotion()
{
}
LLMotion::LLMotionInitStatus LLEyeMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	mHeadJoint = character->getJoint("mHead");
	if ( ! mHeadJoint )
	{
		LL_INFOS() << getName() << ": Can't get head joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mLeftEyeState->setJoint( character->getJoint("mEyeLeft") );
	if ( ! mLeftEyeState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get left eyeball joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mAltLeftEyeState->setJoint( character->getJoint("mFaceEyeAltLeft") );
	if ( ! mAltLeftEyeState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get alt left eyeball joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mRightEyeState->setJoint( character->getJoint("mEyeRight") );
	if ( ! mRightEyeState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get right eyeball joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mAltRightEyeState->setJoint( character->getJoint("mFaceEyeAltRight") );
	if ( ! mAltRightEyeState->getJoint() )
	{
		LL_INFOS() << getName() << ": Can't get alt right eyeball joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mLeftEyeState->setUsage(LLJointState::ROT);
	mAltLeftEyeState->setUsage(LLJointState::ROT);
	mRightEyeState->setUsage(LLJointState::ROT);
	mAltRightEyeState->setUsage(LLJointState::ROT);
	addJointState( mLeftEyeState );
	addJointState( mAltLeftEyeState );
	addJointState( mRightEyeState );
	addJointState( mAltRightEyeState );
	return STATUS_SUCCESS;
}
void LLEyeMotion::adjustEyeTarget(LLVector3* targetPos, LLJointState& left_eye_state, LLJointState& right_eye_state)
{
	BOOL has_eye_target = FALSE;
	LLQuaternion	target_eye_rot;
	LLVector3		eye_look_at;
	F32				vergence;
	if (targetPos)
	{
		LLVector3		skyward(0.f, 0.f, 1.f);
		LLVector3		left;
		LLVector3		up;
		eye_look_at = *targetPos;
		has_eye_target = TRUE;
		F32 lookAtDistance = eye_look_at.normVec();
		left.setVec(skyward % eye_look_at);
		up.setVec(eye_look_at % left);
		target_eye_rot = LLQuaternion(eye_look_at, left, up);
		target_eye_rot *= ~mHeadJoint->getWorldRotation();
		F32 roll, pitch, yaw;
		target_eye_rot.getEulerAngles(&roll, &pitch, &yaw);
		target_eye_rot.setQuat(0.0f, pitch, yaw);
		target_eye_rot.constrain(EYE_ROT_LIMIT_ANGLE);
		F32 interocular_dist = (left_eye_state.getJoint()->getWorldPosition() - right_eye_state.getJoint()->getWorldPosition()).magVec();
		vergence = -atan2((interocular_dist / 2.f), lookAtDistance);
		vergence = llclamp(vergence, -F_PI_BY_TWO, 0.f);
	}
	else
	{
		target_eye_rot = LLQuaternion::DEFAULT;
		vergence = 0.f;
	}
	vergence += 4.f * DEG_TO_RAD;
	LLQuaternion eye_jitter_rot;
	if (vergence > -0.05f)
	{
		eye_jitter_rot.setQuat(0.f, mEyeJitterPitch + mEyeLookAwayPitch, mEyeJitterYaw + mEyeLookAwayYaw);
	}
	else
	{
		eye_jitter_rot.loadIdentity();
	}
	LLQuaternion vergence_quat;
	if (has_eye_target)
	{
		vergence_quat.setQuat(vergence, LLVector3(0.f, 0.f, 1.f));
	}
	else
	{
		vergence_quat.loadIdentity();
	}
	LLQuaternion left_eye_rot = target_eye_rot;
	left_eye_rot = vergence_quat * eye_jitter_rot * left_eye_rot;
	LLQuaternion right_eye_rot = target_eye_rot;
	vergence_quat.transQuat();
	right_eye_rot = vergence_quat * eye_jitter_rot * right_eye_rot;
	left_eye_state.setRotation( left_eye_rot );
	right_eye_state.setRotation( right_eye_rot );
}
BOOL LLEyeMotion::onUpdate(F32 time, U8* joint_mask)
{
	if (mEyeJitterTimer.getElapsedTimeF32() > mEyeJitterTime)
	{
		mEyeJitterTime = EYE_JITTER_MIN_TIME + ll_frand(EYE_JITTER_MAX_TIME - EYE_JITTER_MIN_TIME);
		mEyeJitterYaw = (ll_frand(2.f) - 1.f) * EYE_JITTER_MAX_YAW;
		mEyeJitterPitch = (ll_frand(2.f) - 1.f) * EYE_JITTER_MAX_PITCH;
		mEyeLookAwayTime -= llmax(0.f, mEyeJitterTimer.getElapsedTimeF32());
		mEyeJitterTimer.reset();
	}
	else if (mEyeJitterTimer.getElapsedTimeF32() > mEyeLookAwayTime)
	{
		if (ll_frand() > 0.1f)
		{
			mEyeBlinkTime = mEyeBlinkTimer.getElapsedTimeF32();
		}
		if (mEyeLookAwayYaw == 0.f && mEyeLookAwayPitch == 0.f)
		{
			mEyeLookAwayYaw = (ll_frand(2.f) - 1.f) * EYE_LOOK_AWAY_MAX_YAW;
			mEyeLookAwayPitch = (ll_frand(2.f) - 1.f) * EYE_LOOK_AWAY_MAX_PITCH;
			mEyeLookAwayTime = EYE_LOOK_BACK_MIN_TIME + ll_frand(EYE_LOOK_BACK_MAX_TIME - EYE_LOOK_BACK_MIN_TIME);
		}
		else
		{
			mEyeLookAwayYaw = 0.f;
			mEyeLookAwayPitch = 0.f;
			mEyeLookAwayTime = EYE_LOOK_AWAY_MIN_TIME + ll_frand(EYE_LOOK_AWAY_MAX_TIME - EYE_LOOK_AWAY_MIN_TIME);
		}
	}
	if (!mEyesClosed && mEyeBlinkTimer.getElapsedTimeF32() >= mEyeBlinkTime)
	{
		F32 leftEyeBlinkMorph = mEyeBlinkTimer.getElapsedTimeF32() - mEyeBlinkTime;
		F32 rightEyeBlinkMorph = leftEyeBlinkMorph - EYE_BLINK_TIME_DELTA;
		leftEyeBlinkMorph = llclamp(leftEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
		rightEyeBlinkMorph = llclamp(rightEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
		mCharacter->setVisualParamWeight("Blink_Left", leftEyeBlinkMorph);
		mCharacter->setVisualParamWeight("Blink_Right", rightEyeBlinkMorph);
		mCharacter->updateVisualParams();
		if (rightEyeBlinkMorph == 1.f)
		{
			mEyesClosed = TRUE;
			mEyeBlinkTime = EYE_BLINK_CLOSE_TIME;
			mEyeBlinkTimer.reset();
		}
	}
	else if (mEyesClosed)
	{
		if (mEyeBlinkTimer.getElapsedTimeF32() >= mEyeBlinkTime)
		{
			F32 leftEyeBlinkMorph = mEyeBlinkTimer.getElapsedTimeF32() - mEyeBlinkTime;
			F32 rightEyeBlinkMorph = leftEyeBlinkMorph - EYE_BLINK_TIME_DELTA;
			leftEyeBlinkMorph = 1.f - llclamp(leftEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
			rightEyeBlinkMorph = 1.f - llclamp(rightEyeBlinkMorph / EYE_BLINK_SPEED, 0.f, 1.f);
			mCharacter->setVisualParamWeight("Blink_Left", leftEyeBlinkMorph);
			mCharacter->setVisualParamWeight("Blink_Right", rightEyeBlinkMorph);
			mCharacter->updateVisualParams();
			if (rightEyeBlinkMorph == 0.f)
			{
				mEyesClosed = FALSE;
				mEyeBlinkTime = EYE_BLINK_MIN_TIME + ll_frand(EYE_BLINK_MAX_TIME - EYE_BLINK_MIN_TIME);
				mEyeBlinkTimer.reset();
			}
		}
	}
	LLVector3* targetPos = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");
	adjustEyeTarget(targetPos, *mLeftEyeState, *mRightEyeState);
	adjustEyeTarget(targetPos, *mAltLeftEyeState, *mAltRightEyeState);
	return TRUE;
}
void LLEyeMotion::onDeactivate()
{
	LLJoint* joint = mLeftEyeState->getJoint();
	if (joint)
	{
		joint->setRotation(LLQuaternion::DEFAULT);
	}
	joint = mAltLeftEyeState->getJoint();
	if (joint)
	{
		joint->setRotation(LLQuaternion::DEFAULT);
	}
	joint = mRightEyeState->getJoint();
	if (joint)
	{
		joint->setRotation(LLQuaternion::DEFAULT);
	}
	joint = mAltRightEyeState->getJoint();
	if (joint)
	{
		joint->setRotation(LLQuaternion::DEFAULT);
	}
	AIMaskedMotion::onDeactivate();
}
