/** 
 * @file lljointsolverrp3.cpp
 * @brief Implementation of LLJointSolverRP3 class.
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
#include "lljointsolverrp3.h"
#include "llmath.h"
#define F_EPSILON 0.00001f
LLJointSolverRP3::LLJointSolverRP3()
{
	mJointA = NULL;
	mJointB = NULL;
	mJointC = NULL;
	mJointGoal = NULL;
	mLengthAB = 1.0f;
	mLengthBC = 1.0f;
	mPoleVector.setVec( 1.0f, 0.0f, 0.0f );
	mbUseBAxis = FALSE;
	mTwist = 0.0f;
	mFirstTime = TRUE;
}
LLJointSolverRP3::~LLJointSolverRP3()
{
}
void LLJointSolverRP3::setupJoints(	LLJoint* jointA,
									LLJoint* jointB,
									LLJoint* jointC,
									LLJoint* jointGoal )
{
	mJointA = jointA;
	mJointB = jointB;
	mJointC = jointC;
	mJointGoal = jointGoal;
	mLengthAB = mJointB->getPosition().magVec();
	mLengthBC = mJointC->getPosition().magVec();
	mJointABaseRotation = jointA->getRotation();
	mJointBBaseRotation = jointB->getRotation();
}
const LLVector3& LLJointSolverRP3::getPoleVector()
{
	return mPoleVector;
}
void LLJointSolverRP3::setPoleVector( const LLVector3& poleVector )
{
	mPoleVector = poleVector;
	mPoleVector.normVec();
}
void LLJointSolverRP3::setBAxis( const LLVector3& bAxis )
{
	mBAxis = bAxis;
	mBAxis.normVec();
	mbUseBAxis = TRUE;
}
F32 LLJointSolverRP3::getTwist()
{
	return mTwist;
}
void LLJointSolverRP3::setTwist( F32 twist )
{
	mTwist = twist;
}
void LLJointSolverRP3::solve()
{
	mJointA->setRotation( mJointABaseRotation );
	mJointB->setRotation( mJointBBaseRotation );
	LLVector3 aPos = mJointA->getWorldPosition();
	LLVector3 bPos = mJointB->getWorldPosition();
	LLVector3 cPos = mJointC->getWorldPosition();
	LLVector3 gPos = mJointGoal->getWorldPosition();
	LLVector3 poleVec = mPoleVector;
	if ( mJointA->getParent() )
	{
		LLVector4a pole_veca;
		pole_veca.load3(mPoleVector.mV);
		mJointA->getParent()->getWorldMatrix().rotate(pole_veca,pole_veca);
		poleVec.set(pole_veca.getF32ptr());
	}
	LLVector3 abVec = bPos - aPos;
	LLVector3 bcVec = cPos - bPos;
	LLVector3 acVec = cPos - aPos;
	LLVector3 agVec = gPos - aPos;
	F32 abLen = abVec.magVec();
	F32 bcLen = bcVec.magVec();
	F32 agLen = agVec.magVec();
	LLVector3 abacCompOrthoVec = abVec - acVec * ((abVec * acVec)/(acVec * acVec));
	LLVector3 abcNorm;
	if (!mbUseBAxis)
	{
		if( are_parallel(abVec, bcVec, 0.001f) )
		{
			if ( are_parallel(poleVec, abVec, 0.001f) )
			{
				if ( are_parallel(poleVec, agVec, 0.001f) )
				{
					return;
				}
				else
				{
					abcNorm = poleVec % agVec;
				}
			}
			else
			{
				abcNorm = poleVec % abVec;
			}
		}
		else
		{
			abcNorm = abVec % bcVec;
		}
	}
	else
	{
		abcNorm = mBAxis * mJointB->getWorldRotation();
	}
	F32 abbcAng = angle_between(abVec, bcVec);
	LLVector3 abbcOrthoVec = abVec % bcVec;
	if (abbcOrthoVec.magVecSquared() < 0.001f)
	{
		abbcOrthoVec = poleVec % abVec;
		abacCompOrthoVec = poleVec;
	}
	abbcOrthoVec.normVec();
	F32 agLenSq = agLen * agLen;
	F32 cosTheta =	(agLenSq - abLen*abLen - bcLen*bcLen) / (2.0f * abLen * bcLen);
	if (cosTheta > 1.0f)
		cosTheta = 1.0f;
	else if (cosTheta < -1.0f)
		cosTheta = -1.0f;
	F32 theta = acos(cosTheta);
	LLQuaternion bRot(theta - abbcAng, abbcOrthoVec);
	bcVec = bcVec * bRot;
	acVec = abVec + bcVec;
	LLQuaternion cgRot;
	cgRot.shortestArc( acVec, agVec );
	abVec = abVec * cgRot;
	bcVec = bcVec * cgRot;
	abcNorm = abcNorm * cgRot;
	acVec = abVec + bcVec;
	if (are_parallel(agVec, poleVec, 0.001f))
	{
		return;
	}
	LLVector3 apgNorm = poleVec % agVec;
	apgNorm.normVec();
	if (!mbUseBAxis)
	{
		if( are_parallel(abVec, bcVec, 0.001f) )
		{
		}
		else
		{
			abcNorm = abVec % bcVec;
		}
		abcNorm.normVec();
	}
	LLQuaternion pRot;
	if ( are_parallel( abcNorm, apgNorm, 0.001f) )
	{
		if (abcNorm * apgNorm < 0.0f)
		{
			pRot.setQuat(F_PI, agVec);
		}
		else
		{
		}
	}
	else
	{
		pRot.shortestArc( abcNorm, apgNorm );
	}
	LLQuaternion twistRot( mTwist, agVec );
	LLQuaternion aRot = cgRot * pRot * twistRot;
	mJointB->setWorldRotation( mJointB->getWorldRotation() * bRot );
	mJointA->setWorldRotation( mJointA->getWorldRotation() * aRot );
}
