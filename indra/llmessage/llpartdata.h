/** 
 * @file llpartdata.h
 * @brief Particle system data packing
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#ifndef LL_LLPARTDATA_H
#define LL_LLPARTDATA_H
#include "lluuid.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v2math.h"
#include "v4color.h"
class LLMessageSystem;
class LLDataPacker;
const S32 PS_CUR_VERSION = 18;
enum LLPSScriptFlags
{
	LLPS_PART_FLAGS,
	LLPS_PART_START_COLOR,
	LLPS_PART_START_ALPHA,
	LLPS_PART_END_COLOR,
	LLPS_PART_END_ALPHA,
	LLPS_PART_START_SCALE,
	LLPS_PART_END_SCALE,
	LLPS_PART_MAX_AGE,
	LLPS_SRC_ACCEL,
	LLPS_SRC_PATTERN,
	LLPS_SRC_INNERANGLE,
	LLPS_SRC_OUTERANGLE,
	LLPS_SRC_TEXTURE,
	LLPS_SRC_BURST_RATE,
	LLPS_SRC_BURST_DURATION,
	LLPS_SRC_BURST_PART_COUNT,
	LLPS_SRC_BURST_RADIUS,
	LLPS_SRC_BURST_SPEED_MIN,
	LLPS_SRC_BURST_SPEED_MAX,
	LLPS_SRC_MAX_AGE,
	LLPS_SRC_TARGET_UUID,
	LLPS_SRC_OMEGA,
	LLPS_SRC_ANGLE_BEGIN,
	LLPS_SRC_ANGLE_END,
	LLPS_PART_BLEND_FUNC_SOURCE,
	LLPS_PART_BLEND_FUNC_DEST,
	LLPS_PART_START_GLOW,
	LLPS_PART_END_GLOW
};
class LLPartData
{
public:
	LLPartData() :
		mFlags(0),
		mMaxAge(0.f),
		mParameter(0.f)
	{
	}
	BOOL unpackLegacy(LLDataPacker &dp);
	BOOL unpack(LLDataPacker &dp);
	BOOL pack(LLDataPacker &dp);
	bool hasGlow() const;
	bool hasBlendFunc() const;
	enum
	{
		LL_PART_INTERP_COLOR_MASK =		0x01,
		LL_PART_INTERP_SCALE_MASK =		0x02,
		LL_PART_BOUNCE_MASK =			0x04,
		LL_PART_WIND_MASK =				0x08,
		LL_PART_FOLLOW_SRC_MASK =		0x10,
		LL_PART_FOLLOW_VELOCITY_MASK =	0x20,
		LL_PART_TARGET_POS_MASK =		0x40,
		LL_PART_TARGET_LINEAR_MASK =	0x80,
		LL_PART_EMISSIVE_MASK =			0x100,
		LL_PART_BEAM_MASK =				0x200,
		LL_PART_RIBBON_MASK =			0x400,
		LL_PART_DATA_GLOW =				0x10000,
		LL_PART_DATA_BLEND =			0x20000,
		LL_PART_HUD =					0x40000000,
		LL_PART_DEAD_MASK =				0x80000000,
	};
	enum
	{
		LL_PART_BF_ONE = 0,
		LL_PART_BF_ZERO = 1,
		LL_PART_BF_DEST_COLOR = 2,
		LL_PART_BF_SOURCE_COLOR = 3,
		LL_PART_BF_ONE_MINUS_DEST_COLOR = 4,
		LL_PART_BF_ONE_MINUS_SOURCE_COLOR = 5,
		UNSUPPORTED_DEST_ALPHA = 6,
		LL_PART_BF_SOURCE_ALPHA = 7,
		UNSUPPORTED_ONE_MINUS_DEST_ALPHA = 8,
		LL_PART_BF_ONE_MINUS_SOURCE_ALPHA = 9,
		LL_PART_BF_COUNT = 10
	};
	static bool validBlendFunc(S32 func);
	void setFlags(const U32 flags);
	void setMaxAge(const F32 max_age);
	void setStartScale(const F32 xs, F32 ys);
	void setEndScale(const F32 xs, F32 ys);
	void setStartColor(const LLVector3 &rgb);
	void setEndColor(const LLVector3 &rgb);
	void setStartAlpha(const F32 alpha);
	void setEndAlpha(const F32 alpha);
	friend class LLPartSysData;
	friend class LLViewerPartSourceScript;
private:
	S32 getSize() const;
public:
	U32					mFlags;
	F32					mMaxAge;
	LLColor4			mStartColor;
	LLColor4			mEndColor;
	LLVector2			mStartScale;
	LLVector2			mEndScale;
	LLVector3			mPosOffset;
	F32					mParameter;
	F32					mStartGlow;
	F32					mEndGlow;
	U8					mBlendFuncSource;
	U8					mBlendFuncDest;
};
class LLPartSysData
{
public:
	LLPartSysData();
	BOOL unpack(LLDataPacker &dp);
	BOOL unpackLegacy(LLDataPacker &dp);
	BOOL unpackBlock(const S32 block_num);
	static BOOL isNullPS(const S32 block_num);
	bool isLegacyCompatible() const;
	enum
	{
		LL_PART_SRC_OBJ_REL_MASK		=	0x01,
		LL_PART_USE_NEW_ANGLE			=	0x02,
	};
	enum
	{
		LL_PART_SRC_PATTERN_DROP =				0x01,
		LL_PART_SRC_PATTERN_EXPLODE =			0x02,
		LL_PART_SRC_PATTERN_ANGLE =				0x04,
		LL_PART_SRC_PATTERN_ANGLE_CONE =		0x08,
		LL_PART_SRC_PATTERN_ANGLE_CONE_EMPTY =	0x10,
	};
	void setBurstSpeedMin(const F32 spd) {	mBurstSpeedMin = llclamp(spd, -100.f, 100.f); }
	void setBurstSpeedMax(const F32 spd) {	mBurstSpeedMax = llclamp(spd, -100.f, 100.f); }
	void setBurstRadius(const F32 rad)	 {	mBurstRadius = llclamp(rad, 0.f, 50.f); }
	void setPartAccel(const LLVector3 &accel);
	void setUseNewAngle()	{ mFlags |=  LL_PART_USE_NEW_ANGLE; }
	void unsetUseNewAngle()	{ mFlags &= ~LL_PART_USE_NEW_ANGLE; }
	void clampSourceParticleRate();
	friend std::ostream&	 operator<<(std::ostream& s, const LLPartSysData &data);
	S32 getdataBlockSize() const;
private:
	BOOL unpackSystem(LLDataPacker &dp);
public:
	U32		mCRC;
	U32		mFlags;
	U8		mPattern;
	F32		mInnerAngle;
	F32		mOuterAngle;
	LLVector3 mAngularVelocity;
	F32		mBurstRate;
	U8		mBurstPartCount;
	F32		mBurstRadius;
	F32		mBurstSpeedMin;
	F32		mBurstSpeedMax;
	F32		mMaxAge;
	LLUUID	mTargetUUID;
	F32		mStartAge;
	LLVector3	mPartAccel;
	LLUUID		mPartImageID;
	LLPartData	mPartData;
protected:
	S32		mNumParticles;
};
#endif
