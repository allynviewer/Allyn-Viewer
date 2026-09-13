/** 
 * @file llkeyframewalkmotion.cpp
 * @brief Implementation of LLKeyframeWalkMotion class.
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
#include "linden_common.h"
#include "llkeyframewalkmotion.h"
#include "llcharacter.h"
#include "llmath.h"
#include "m3math.h"
#include "llcriticaldamp.h"
const F32 MAX_WALK_PLAYBACK_SPEED = 8.f;
const F32 MIN_WALK_SPEED = 0.1f;
const F32 TIME_EPSILON = 0.001f;
const F32 MAX_TIME_DELTA = 2.f;
const F32 SPEED_ADJUST_MAX_SEC = 3.f;
F32 ANIM_SPEED_MAX = 1.5f;
const F32 DRIFT_COMP_MAX_TOTAL = 0.1f;
const F32 DRIFT_COMP_MAX_SPEED = 4.f;
const F32 MAX_ROLL = 0.6f;
const F32 PELVIS_COMPENSATION_WIEGHT = 0.7f;
const F32 SPEED_ADJUST_TIME_CONSTANT = 0.1f;
LLKeyframeWalkMotion::LLKeyframeWalkMotion(LLUUID const& id, LLMotionController* controller)
:	LLKeyframeMotion(id, controller),
    mCharacter(NULL),
    mCyclePhase(0.0f),
    mRealTimeLast(0.0f),
    mAdjTimeLast(0.0f),
    mDownFoot(0)
{}
LLKeyframeWalkMotion::~LLKeyframeWalkMotion()
{}
LLMotion::LLMotionInitStatus LLKeyframeWalkMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	return LLKeyframeMotion::onInitialize(character);
}
BOOL LLKeyframeWalkMotion::onActivate()
{
	mRealTimeLast = 0.0f;
	mAdjTimeLast = 0.0f;
	return LLKeyframeMotion::onActivate();
}
void LLKeyframeWalkMotion::onDeactivate()
{
	mCharacter->removeAnimationData("Down Foot");
	LLKeyframeMotion::onDeactivate();
}
BOOL LLKeyframeWalkMotion::onUpdate(F32 time, U8* joint_mask)
{
	F32 deltaTime = time - mRealTimeLast;
	void* speed_ptr = mCharacter->getAnimationData("Walk Speed");
	F32 speed = (speed_ptr) ? *((F32 *)speed_ptr) : 1.f;
	F32 adjusted_time = mAdjTimeLast + (deltaTime * speed);
	mRealTimeLast = time;
	mAdjTimeLast = adjusted_time;
	if (adjusted_time < 0.0f)
	{
		adjusted_time = getDuration() + fmod(adjusted_time, getDuration());
	}
	return LLKeyframeMotion::onUpdate( adjusted_time, joint_mask );
}
LLWalkAdjustMotion::LLWalkAdjustMotion(LLUUID const& id, LLMotionController* controller) :
	AIMaskedMotion(id, controller, ANIM_AGENT_WALK_ADJUST),
	mCharacter(NULL),
	mLastTime(0.f),
	mAnimSpeed(0.f),
	mAdjustedSpeed(0.f),
	mRelativeDir(0.f),
	mAnkleOffset(0.f),
	mLeftAnkleJoint(NULL),
	mRightAnkleJoint(NULL),
	mPelvisJoint(NULL),
	mPelvisState(new LLJointState)
{
	mName = "walk_adjust";
}
LLMotion::LLMotionInitStatus LLWalkAdjustMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	mLeftAnkleJoint = mCharacter->getJoint("mAnkleLeft");
	mRightAnkleJoint = mCharacter->getJoint("mAnkleRight");
	mPelvisJoint = mCharacter->getJoint("mPelvis");
	mPelvisState->setJoint( mPelvisJoint );
	if ( !mPelvisJoint )
	{
		LL_WARNS() << getName() << ": Can't get pelvis joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mPelvisState->setUsage(LLJointState::POS);
	addJointState( mPelvisState );
	return STATUS_SUCCESS;
}
BOOL LLWalkAdjustMotion::onActivate()
{
	mAnimSpeed = 0.f;
	mAdjustedSpeed = 0.f;
	mRelativeDir = 1.f;
	mPelvisState->setPosition(LLVector3::zero);
	mLastLeftFootGlobalPos = mCharacter->getPosGlobalFromAgent(mLeftAnkleJoint->getWorldPosition());
	mLastLeftFootGlobalPos.mdV[VZ] = 0.0;
	mLastRightFootGlobalPos = mCharacter->getPosGlobalFromAgent(mRightAnkleJoint->getWorldPosition());
	mLastRightFootGlobalPos.mdV[VZ] = 0.0;
	F32 leftAnkleOffset = (mLeftAnkleJoint->getWorldPosition() - mCharacter->getCharacterPosition()).magVec();
	F32 rightAnkleOffset = (mRightAnkleJoint->getWorldPosition() - mCharacter->getCharacterPosition()).magVec();
	mAnkleOffset = llmax(leftAnkleOffset, rightAnkleOffset);
	return AIMaskedMotion::onActivate();
}
BOOL LLWalkAdjustMotion::onUpdate(F32 time, U8* joint_mask)
{
	F32 delta_time = llclamp(time - mLastTime, TIME_EPSILON, MAX_TIME_DELTA);
	mLastTime = time;
	LLVector3 avatar_velocity = mCharacter->getCharacterVelocity() * mCharacter->getTimeDilation();
	avatar_velocity.mV[VZ] = 0.f;
	F32 speed = llclamp(avatar_velocity.magVec(), 0.f, MAX_WALK_PLAYBACK_SPEED);
	LLQuaternion avatar_to_world_rot = mCharacter->getRootJoint()->getWorldRotation();
	LLQuaternion world_to_avatar_rot(avatar_to_world_rot);
	world_to_avatar_rot.conjugate();
	LLVector3 foot_slip_vector;
	if (speed > MIN_WALK_SPEED)
	{
		LLVector3d leftFootGlobalPosition = mCharacter->getPosGlobalFromAgent(mLeftAnkleJoint->getWorldPosition());
		leftFootGlobalPosition.mdV[VZ] = 0.0;
		LLVector3 leftFootDelta(leftFootGlobalPosition - mLastLeftFootGlobalPos);
		mLastLeftFootGlobalPos = leftFootGlobalPosition;
		LLVector3d rightFootGlobalPosition = mCharacter->getPosGlobalFromAgent(mRightAnkleJoint->getWorldPosition());
		rightFootGlobalPosition.mdV[VZ] = 0.0;
		LLVector3 rightFootDelta(rightFootGlobalPosition - mLastRightFootGlobalPos);
		mLastRightFootGlobalPos = rightFootGlobalPosition;
		F32 left_foot_slip_amt = leftFootDelta * avatar_velocity;
		F32 right_foot_slip_amt = rightFootDelta * avatar_velocity;
		if (right_foot_slip_amt < left_foot_slip_amt)
		{
			foot_slip_vector = rightFootDelta;
		}
		else
		{
			foot_slip_vector = leftFootDelta;
		}
		LLVector3 avatar_movement_dir = avatar_velocity;
		avatar_movement_dir.normalize();
		F32 foot_speed = llmax(0.f, speed - ((foot_slip_vector * avatar_movement_dir) / delta_time));
		F32 min_speed_multiplier = clamp_rescale(speed, 0.f, 1.f, 0.f, 0.1f);
		F32 desired_speed_multiplier = llclamp(speed / foot_speed, min_speed_multiplier, ANIM_SPEED_MAX);
		F32 new_speed_adjust = LLSmoothInterpolation::lerp(mAdjustedSpeed, desired_speed_multiplier, SPEED_ADJUST_TIME_CONSTANT);
		F32 speedDelta = llclamp(new_speed_adjust - mAdjustedSpeed, -SPEED_ADJUST_MAX_SEC * delta_time, SPEED_ADJUST_MAX_SEC * delta_time);
		mAdjustedSpeed += speedDelta;
		F32 directional_factor = (avatar_movement_dir * world_to_avatar_rot).mV[VX];
		mAnimSpeed = mAdjustedSpeed * directional_factor;
	}
	else
	{
		mAnimSpeed = LLSmoothInterpolation::lerp(mAnimSpeed, 1.f, 0.2f);
	}
 	mCharacter->setAnimationData("Walk Speed", &mAnimSpeed);
	mPelvisState->setPosition(mPelvisOffset);
	return TRUE;
}
void LLWalkAdjustMotion::onDeactivate()
{
	mCharacter->removeAnimationData("Walk Speed");
	AIMaskedMotion::onDeactivate();
}
LLFlyAdjustMotion::LLFlyAdjustMotion(LLUUID const& id, LLMotionController* controller)
	: AIMaskedMotion(id, controller, ANIM_AGENT_FLY_ADJUST),
	  mCharacter(NULL),
	  mRoll(0.f)
{
	mName = "fly_adjust";
	mPelvisState = new LLJointState;
}
LLMotion::LLMotionInitStatus LLFlyAdjustMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	LLJoint* pelvisJoint = mCharacter->getJoint("mPelvis");
	mPelvisState->setJoint( pelvisJoint );
	if ( !pelvisJoint )
	{
		LL_WARNS() << getName() << ": Can't get pelvis joint." << LL_ENDL;
		return STATUS_FAILURE;
	}
	mPelvisState->setUsage(LLJointState::POS | LLJointState::ROT);
	addJointState( mPelvisState );
	return STATUS_SUCCESS;
}
BOOL LLFlyAdjustMotion::onActivate()
{
	mPelvisState->setPosition(LLVector3::zero);
	mPelvisState->setRotation(LLQuaternion::DEFAULT);
	mRoll = 0.f;
	return AIMaskedMotion::onActivate();
}
BOOL LLFlyAdjustMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLVector3 ang_vel = mCharacter->getCharacterAngularVelocity() * mCharacter->getTimeDilation();
	F32 speed = mCharacter->getCharacterVelocity().magVec();
	F32 roll_factor = clamp_rescale(speed, 7.f, 15.f, 0.f, -MAX_ROLL);
	F32 target_roll = llclamp(ang_vel.mV[VZ], -4.f, 4.f) * roll_factor;
	mRoll = LLSmoothInterpolation::lerp(mRoll, target_roll, U32Milliseconds(100));
	LLQuaternion roll(mRoll, LLVector3(0.f, 0.f, 1.f));
	mPelvisState->setRotation(roll);
	return TRUE;
}
