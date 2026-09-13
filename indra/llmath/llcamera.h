/** 
 * @file llcamera.h
 * @brief Header file for the LLCamera class.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_CAMERA_H
#define LL_CAMERA_H
#include "llmath.h"
#include "llcoordframe.h"
#include "llplane.h"
#include "llvector4a.h"
const F32 DEFAULT_FIELD_OF_VIEW 	= 60.f * DEG_TO_RAD;
const F32 DEFAULT_ASPECT_RATIO 		= 640.f / 480.f;
const F32 DEFAULT_NEAR_PLANE 		= 0.25f;
const F32 DEFAULT_FAR_PLANE 		= 64.f;
const F32 MAX_ASPECT_RATIO 	= 50.0f;
const F32 MAX_NEAR_PLANE 	= 10.f;
const F32 MAX_FAR_PLANE 	= 100000.0f;
const F32 MAX_FAR_CLIP		= 512.0f;
const F32 MIN_ASPECT_RATIO 	= 0.02f;
const F32 MIN_NEAR_PLANE 	= 0.1f;
const F32 MIN_FAR_PLANE 	= 0.2f;
static const F32 MIN_FIELD_OF_VIEW = 5.0f * DEG_TO_RAD;
static const F32 MAX_FIELD_OF_VIEW = 320.f * DEG_TO_RAD;
LL_ALIGN_PREFIX(16)
class LLCamera
: 	public LLCoordFrame
{
public:
	LLCamera(const LLCamera& rhs)
	{
		*this = rhs;
	}
	enum {
		PLANE_LEFT = 0,
		PLANE_RIGHT = 1,
		PLANE_BOTTOM = 2,
		PLANE_TOP = 3,
		PLANE_NUM = 4,
		PLANE_MASK_NONE = 0xff
	};
	enum {
		PLANE_LEFT_MASK = (1<<PLANE_LEFT),
		PLANE_RIGHT_MASK = (1<<PLANE_RIGHT),
		PLANE_BOTTOM_MASK = (1<<PLANE_BOTTOM),
		PLANE_TOP_MASK = (1<<PLANE_TOP),
		PLANE_ALL_MASK = 0xf,
	};
	enum
	{
		AGENT_PLANE_LEFT = 0,
		AGENT_PLANE_RIGHT = 1,
		AGENT_PLANE_NEAR = 2,
		AGENT_PLANE_BOTTOM = 3,
		AGENT_PLANE_TOP = 4,
		AGENT_PLANE_FAR = 5,
		AGENT_PLANE_USER_CLIP = 6
	};
	enum
	{
		AGENT_PLANE_NO_USER_CLIP_NUM = 6,
		AGENT_PLANE_USER_CLIP_NUM = 7,
		PLANE_MASK_NUM = 8
	};
	enum
	{
		AGENT_FRUSTRUM_NUM = 8
	};
	enum {
		HORIZ_PLANE_LEFT = 0,
		HORIZ_PLANE_RIGHT = 1,
		HORIZ_PLANE_NUM = 2
	};
	enum {
		HORIZ_PLANE_LEFT_MASK = (1<<HORIZ_PLANE_LEFT),
		HORIZ_PLANE_RIGHT_MASK = (1<<HORIZ_PLANE_RIGHT),
		HORIZ_PLANE_ALL_MASK = 0x3
	};
private:
	LL_ALIGN_16(LLPlane mAgentPlanes[AGENT_PLANE_USER_CLIP_NUM]);
	LL_ALIGN_16(LLPlane mRegionPlanes[AGENT_PLANE_USER_CLIP_NUM]);
	LL_ALIGN_16(LLPlane mLastAgentPlanes[AGENT_PLANE_USER_CLIP_NUM]);
	U8 mPlaneMask[PLANE_MASK_NUM];
	F32 mView;
	F32 mAspect;
	S32 mViewHeightInPixels;
	F32 mNearPlane;
	F32 mFarPlane;
	LL_ALIGN_16(LLPlane mLocalPlanes[PLANE_NUM]);
	F32 mFixedDistance;
	LLVector3 mFrustCenter;
	F32 mFrustRadiusSquared;
	LL_ALIGN_16(LLPlane mWorldPlanes[PLANE_NUM]);
	LL_ALIGN_16(LLPlane mHorizPlanes[HORIZ_PLANE_NUM]);
	U32 mPlaneCount;
	LLVector3 mWorldPlanePos;
public:
	LLVector3 mAgentFrustum[AGENT_FRUSTRUM_NUM];
	F32	mFrustumCornerDist;
	LLPlane& getAgentPlane(U32 idx) { return mAgentPlanes[idx]; }
public:
	LLCamera();
	LLCamera(F32 vertical_fov_rads, F32 aspect_ratio, S32 view_height_in_pixels, F32 near_plane, F32 far_plane);
	virtual ~LLCamera();
	bool isChanged();
    LLPlane getUserClipPlane() const;
	void setUserClipPlane(const LLPlane& plane);
	void disableUserClipPlane();
	virtual void setView(F32 vertical_fov_rads);
	void setViewHeightInPixels(S32 height);
	void setAspect(F32 new_aspect);
	void setNear(F32 new_near);
	void setFar(F32 new_far);
	F32 getView() const							{ return mView; }
	S32 getViewHeightInPixels() const			{ return mViewHeightInPixels; }
	F32 getAspect() const						{ return mAspect; }
	F32 getNear() const							{ return mNearPlane; }
	F32 getFar() const							{ return mFarPlane; }
	F32 getMinView() const;
	F32 getMaxView() const;
	F32 getYaw() const
	{
		return atan2f(mXAxis[VY], mXAxis[VX]);
	}
	F32 getPitch() const
	{
		F32 xylen = sqrtf(mXAxis[VX]*mXAxis[VX] + mXAxis[VY]*mXAxis[VY]);
		return atan2f(mXAxis[VZ], xylen);
	}
	const LLPlane& getWorldPlane(S32 index) const	{ return mWorldPlanes[index]; }
	const LLVector3& getWorldPlanePos() const		{ return mWorldPlanePos; }
	size_t writeFrustumToBuffer(char *buffer) const;
	size_t readFrustumFromBuffer(const char *buffer);
	void calcAgentFrustumPlanes(LLVector3* frust);
	void calcRegionFrustumPlanes(const LLVector3& shift, F32 far_clip_distance);
	void ignoreAgentFrustumPlane(S32 idx);
	S32 sphereInFrustumOld(const LLVector3 &center, const F32 radius) const;
	S32 sphereInFrustum(const LLVector3 &center, const F32 radius) const;
	S32 pointInFrustum(const LLVector3 &point) const { return sphereInFrustum(point, 0.0f); }
	S32 sphereInFrustumFull(const LLVector3 &center, const F32 radius) const { return sphereInFrustum(center, radius); }
	S32 AABBInFrustum(const LLVector4a& center, const LLVector4a& radius, const LLPlane* planes = NULL);
	S32 AABBInRegionFrustum(const LLVector4a& center, const LLVector4a& radius);
	S32 AABBInFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius, const LLPlane* planes = NULL);
	S32 AABBInRegionFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius);
	S32 sphereInFrustumQuick(const LLVector3 &sphere_center, const F32 radius);
	F32 heightInPixels(const LLVector3 &center, F32 radius ) const;
	F32 visibleDistance(const LLVector3 &pos, F32 rad, F32 fudgescale = 1.0f, U32 planemask = PLANE_ALL_MASK) const;
	F32 visibleHorizDistance(const LLVector3 &pos, F32 rad, F32 fudgescale = 1.0f, U32 planemask = HORIZ_PLANE_ALL_MASK) const;
	void setFixedDistance(F32 distance) { mFixedDistance = distance; }
	friend std::ostream& operator<<(std::ostream &s, const LLCamera &C);
protected:
	void calculateFrustumPlanes();
	void calculateFrustumPlanes(F32 left, F32 right, F32 top, F32 bottom);
	void calculateFrustumPlanesFromWindow(F32 x1, F32 y1, F32 x2, F32 y2);
	void calculateWorldFrustumPlanes();
} LL_ALIGN_POSTFIX(16);
#endif
