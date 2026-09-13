/** 
 * @file llkeyframestandmotion.cpp
 * @brief Implementation of LLKeyframeStandMotion class.
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
#include "llkeyframestandmotion.h"
#include "llcharacter.h"
#define GO_TO_KEY_POSE	1
#define MIN_TRACK_SPEED 0.01f
const F32 ROTATION_THRESHOLD = 0.6f;
const F32 POSITION_THRESHOLD = 0.1f;
LLKeyframeStandMotion::LLKeyframeStandMotion(LLUUID const& id, LLMotionController* controller) : LLKeyframeMotion(id, controller)
{
	mFlipFeet = FALSE;
	mCharacter = NULL;
	mPelvisJoint.addChild( &mHipLeftJoint );
		mHipLeftJoint.addChild( &mKneeLeftJoint );
			mKneeLeftJoint.addChild( &mAnkleLeftJoint );
	mPelvisJoint.addChild( &mHipRightJoint );
		mHipRightJoint.addChild( &mKneeRightJoint );
			mKneeRightJoint.addChild( &mAnkleRightJoint );
	mPelvisState = NULL;
	mHipLeftState =  NULL;
	mKneeLeftState =  NULL;
	mAnkleLeftState =  NULL;
	mHipRightState =  NULL;
	mKneeRightState =  NULL;
	mAnkleRightState =  NULL;
	mTrackAnkles = TRUE;
	mFrameNum = 0;
}
LLKeyframeStandMotion::~LLKeyframeStandMotion()
{
}
LLMotion::LLMotionInitStatus LLKeyframeStandMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	mFlipFeet = FALSE;
	LLMotion::LLMotionInitStatus status = LLKeyframeMotion::onInitialize(character);
	if ( status == STATUS_FAILURE )
	{
		return status;
	}
	LLPose *pose = getPose();
	mPelvisState = pose->findJointState("mPelvis");
	mHipLeftState = pose->findJointState("mHipLeft");
	mKneeLeftState = pose->findJointState("mKneeLeft");
	mAnkleLeftState = pose->findJointState("mAnkleLeft");
	mHipRightState = pose->findJointState("mHipRight");
	mKneeRightState = pose->findJointState("mKneeRight");
	mAnkleRightState = pose->findJointState("mAnkleRight");
	if (	!mPelvisState ||
			!mHipLeftState ||
			!mKneeLeftState ||
			!mAnkleLeftState ||
			!mHipRightState ||
			!mKneeRightState ||
			!mAnkleRightState )
	{
		LL_INFOS() << getName() << ": Can't find necessary joint states" << LL_ENDL;
		return STATUS_FAILURE;
	}
	return STATUS_SUCCESS;
}
BOOL LLKeyframeStandMotion::onActivate()
{
	mIKLeft.setPoleVector( LLVector3(1.0f, 0.0f, 0.0f));
	mIKRight.setPoleVector( LLVector3(1.0f, 0.0f, 0.0f));
	mIKLeft.setBAxis( LLVector3(0.05f, 1.0f, 0.0f));
	mIKRight.setBAxis( LLVector3(-0.05f, 1.0f, 0.0f));
	mLastGoodPelvisRotation.loadIdentity();
	mLastGoodPosition.clearVec();
	mFrameNum = 0;
	return LLKeyframeMotion::onActivate();
}
void LLKeyframeStandMotion::onDeactivate()
{
	LLKeyframeMotion::onDeactivate();
}
BOOL LLKeyframeStandMotion::onUpdate(F32 time, U8* joint_mask)
{
	BOOL status = LLKeyframeMotion::onUpdate(time, joint_mask);
	if (!status)
	{
		return FALSE;
	}
	LLVector3 root_world_pos = mPelvisState->getJoint()->getParent()->getWorldPosition();
	if (root_world_pos.isExactlyZero())
	{
		return TRUE;
	}
	if (dot(mPelvisState->getJoint()->getWorldRotation(), mLastGoodPelvisRotation) < ROTATION_THRESHOLD)
	{
		mLastGoodPelvisRotation = mPelvisState->getJoint()->getWorldRotation();
		mLastGoodPelvisRotation.normalize();
		mTrackAnkles = TRUE;
	}
	else if ((mCharacter->getCharacterPosition() - mLastGoodPosition).magVecSquared() > POSITION_THRESHOLD)
	{
		mLastGoodPosition = mCharacter->getCharacterPosition();
		mTrackAnkles = TRUE;
	}
	else if (mPose.getWeight() < 1.f)
	{
		mTrackAnkles = TRUE;
	}
	mPelvisJoint.setPosition(
			root_world_pos +
			mPelvisState->getPosition() );
	mHipLeftJoint.setPosition( mHipLeftState->getJoint()->getPosition() );
	mKneeLeftJoint.setPosition( mKneeLeftState->getJoint()->getPosition() );
	mAnkleLeftJoint.setPosition( mAnkleLeftState->getJoint()->getPosition() );
	mHipLeftJoint.setScale( mHipLeftState->getJoint()->getScale() );
	mKneeLeftJoint.setScale( mKneeLeftState->getJoint()->getScale() );
	mAnkleLeftJoint.setScale( mAnkleLeftState->getJoint()->getScale() );
	mHipRightJoint.setPosition( mHipRightState->getJoint()->getPosition() );
	mKneeRightJoint.setPosition( mKneeRightState->getJoint()->getPosition() );
	mAnkleRightJoint.setPosition( mAnkleRightState->getJoint()->getPosition() );
	mHipRightJoint.setScale( mHipRightState->getJoint()->getScale() );
	mKneeRightJoint.setScale( mKneeRightState->getJoint()->getScale() );
	mAnkleRightJoint.setScale( mAnkleRightState->getJoint()->getScale() );
	mPelvisJoint.setRotation( mPelvisState->getJoint()->getWorldRotation() );
#if GO_TO_KEY_POSE
	mHipLeftJoint.setRotation( mHipLeftState->getRotation() );
	mKneeLeftJoint.setRotation( mKneeLeftState->getRotation() );
	mAnkleLeftJoint.setRotation( mAnkleLeftState->getRotation() );
	mHipRightJoint.setRotation( mHipRightState->getRotation() );
	mKneeRightJoint.setRotation( mKneeRightState->getRotation() );
	mAnkleRightJoint.setRotation( mAnkleRightState->getRotation() );
#else
	mHipLeftJoint.setRotation( mHipLeftState->getJoint()->getRotation() );
	mKneeLeftJoint.setRotation( mKneeLeftState->getJoint()->getRotation() );
	mAnkleLeftJoint.setRotation( mAnkleLeftState->getJoint()->getRotation() );
	mHipRightJoint.setRotation( mHipRightState->getJoint()->getRotation() );
	mKneeRightJoint.setRotation( mKneeRightState->getJoint()->getRotation() );
	mAnkleRightJoint.setRotation( mAnkleRightState->getJoint()->getRotation() );
#endif
	if (mFrameNum == 2)
	{
		mIKLeft.setupJoints( &mHipLeftJoint, &mKneeLeftJoint, &mAnkleLeftJoint, &mTargetLeft );
		mIKRight.setupJoints( &mHipRightJoint, &mKneeRightJoint, &mAnkleRightJoint, &mTargetRight );
	}
	else if (mFrameNum < 2)
	{
		mFrameNum++;
		return TRUE;
	}
	mFrameNum++;
	if ( mTrackAnkles )
	{
		mCharacter->getGround( mAnkleLeftJoint.getWorldPosition(), mPositionLeft, mNormalLeft);
		mCharacter->getGround( mAnkleRightJoint.getWorldPosition(), mPositionRight, mNormalRight);
		mTargetLeft.setPosition( mPositionLeft );
		mTargetRight.setPosition( mPositionRight );
	}
	mIKLeft.solve();
	mIKRight.solve();
	if ( mTrackAnkles )
	{
		const LLVector4a& dirLeft4 = mAnkleLeftJoint.getWorldMatrix().getRow<LLMatrix4a::ROW_FWD>();
		const LLVector4a& dirRight4 = mAnkleRightJoint.getWorldMatrix().getRow<LLMatrix4a::ROW_FWD>();
		LLVector4a up;
		LLVector4a dir;
		LLVector4a left;
		up.load3(mNormalLeft.mV);
		up.normalize3fast();
		if (mFlipFeet)
		{
			up.negate();
		}
		dir = dirLeft4;
		dir.normalize3fast();
		left.setCross3(up,dir);
		left.normalize3fast();
		dir.setCross3(left,up);
		mRotationLeft = LLQuaternion( LLVector3(dir.getF32ptr()), LLVector3(left.getF32ptr()), LLVector3(up.getF32ptr()));
		up.load3(mNormalRight.mV);
		up.normalize3fast();
		if (mFlipFeet)
		{
			up.negate();
		}
		dir = dirRight4;
		dir.normalize3fast();
		left.setCross3(up,dir);
		left.normalize3fast();
		dir.setCross3(left,up);
		mRotationRight = LLQuaternion( LLVector3(dir.getF32ptr()), LLVector3(left.getF32ptr()), LLVector3(up.getF32ptr()));
	}
	mAnkleLeftJoint.setWorldRotation( mRotationLeft );
	mAnkleRightJoint.setWorldRotation( mRotationRight );
	mHipLeftState->setRotation( mHipLeftJoint.getRotation() );
	mKneeLeftState->setRotation( mKneeLeftJoint.getRotation() );
	mAnkleLeftState->setRotation( mAnkleLeftJoint.getRotation() );
	mHipRightState->setRotation( mHipRightJoint.getRotation() );
	mKneeRightState->setRotation( mKneeRightJoint.getRotation() );
	mAnkleRightState->setRotation( mAnkleRightJoint.getRotation() );
	return TRUE;
}
