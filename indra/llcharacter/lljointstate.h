/** 
 * @file lljointstate.h
 * @brief Implementation of LLJointState class.
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
#ifndef LL_LLJOINTSTATE_H
#define LL_LLJOINTSTATE_H
#include "lljoint.h"
#include "llrefcount.h"
class LLJointState : public LLRefCount
{
public:
	enum BlendPhase
	{
		INACTIVE,
		EASE_IN,
		ACTIVE,
		EASE_OUT
	};
protected:
	LLJoint	*mJoint;
	U32		mUsage;
	F32		mWeight;
	LLVector3		mPosition;
	LLQuaternion	mRotation;
	LLVector3		mScale;
	LLJoint::JointPriority	mPriority;
public:
	LLJointState()
		: mJoint(NULL)
		, mUsage(0)
		, mWeight(0.f)
		, mPriority(LLJoint::USE_MOTION_PRIORITY)
	{}
	LLJointState(LLJoint* joint)
		: mJoint(joint)
		, mUsage(0)
		, mWeight(0.f)
		, mPriority(LLJoint::USE_MOTION_PRIORITY)
	{}
	LLJoint* getJoint()				{ return mJoint; }
	const LLJoint* getJoint() const	{ return mJoint; }
	BOOL setJoint( LLJoint *joint )	{ mJoint = joint; return mJoint != NULL; }
	enum Usage
	{
		POS		= 1,
		ROT		= 2,
		SCALE	= 4,
	};
	U32 getUsage() const			{ return mUsage; }
	void setUsage( U32 usage )		{ mUsage = usage; }
	F32 getWeight() const			{ return mWeight; }
	void setWeight( F32 weight )	{ mWeight = weight; }
	const LLVector3& getPosition() const		{ return mPosition; }
	void setPosition( const LLVector3& pos )	{ llassert(mUsage & POS); mPosition = pos; }
	const LLQuaternion& getRotation() const		{ return mRotation; }
	void setRotation( const LLQuaternion& rot )	{ llassert(mUsage & ROT); mRotation = rot; }
	const LLVector3& getScale() const			{ return mScale; }
	void setScale( const LLVector3& scale )		{ llassert(mUsage & SCALE); mScale = scale; }
	LLJoint::JointPriority getPriority() const	{ return mPriority; }
	void setPriority( LLJoint::JointPriority priority ) { mPriority = priority; }
protected:
	virtual ~LLJointState()
	{
	}
};
#endif
