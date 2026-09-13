/** 
 * @file lltreeparams.h
 * @brief Implementation of the LLTreeParams class
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
#ifndef LL_LLTREEPARAMS_H
#define LL_LLTREEPARAMS_H
enum EShapeRatio { SR_CONICAL, SR_SPHERICAL, SR_HEMISPHERICAL,
				SR_CYLINDRICAL, SR_TAPERED_CYLINDRICAL, SR_FLAME,
				SR_INVERSE_CONICAL, SR_TEND_FLAME, SR_ENVELOPE};
const U32 TREE_BLOCK_SIZE = 16;
const U8 MAX_NUM_LEVELS = 4;
class LLTreeParams
{
public:
	LLTreeParams();
	virtual ~LLTreeParams();
	static F32 ShapeRatio(EShapeRatio shape, F32 ratio);
public:
	EShapeRatio mShape;
	U8 mLevels;
	F32 mBaseSize;
	F32 mScale, mScaleV;
	F32 mScale0, mScaleV0;
	U8 mLobes;
	F32 mLobeDepth;
	F32 mFlare;
	F32 mFlarePercentage;
	U8 mFlareRes;
	U8 mLeaves;
	F32 mLeafScaleX, mLeafScaleY;
	F32 mLeafQuality;
	F32 mDownAngle[MAX_NUM_LEVELS - 1];
	F32 mDownAngleV[MAX_NUM_LEVELS - 1];
	F32 mRotate[MAX_NUM_LEVELS - 1];
	F32 mRotateV[MAX_NUM_LEVELS - 1];
	U8 mBranches[MAX_NUM_LEVELS - 1];
	F32 mLength[MAX_NUM_LEVELS];
	F32 mLengthV[MAX_NUM_LEVELS];
	F32 mRatio, mRatioPower;
	F32 mTaper[MAX_NUM_LEVELS];
	U8 mBaseSplits;
	F32 mSegSplits[MAX_NUM_LEVELS];
	F32 mSplitAngle[MAX_NUM_LEVELS];
	F32 mSplitAngleV[MAX_NUM_LEVELS];
	F32 mCurve[MAX_NUM_LEVELS];
	F32 mCurveV[MAX_NUM_LEVELS];
	U8 mCurveRes[MAX_NUM_LEVELS];
	F32 mCurveBack[MAX_NUM_LEVELS];
	U8 mVertices[MAX_NUM_LEVELS];
};
#endif
