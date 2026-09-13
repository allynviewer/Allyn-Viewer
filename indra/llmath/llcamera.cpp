/** 
 * @file llcamera.cpp
 * @brief Implementation of the LLCamera class.
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
#include "linden_common.h"
#include "llmath.h"
#include "llcamera.h"
LLCamera::LLCamera() :
	LLCoordFrame(),
	mView(DEFAULT_FIELD_OF_VIEW),
	mAspect(DEFAULT_ASPECT_RATIO),
	mViewHeightInPixels( -1 ),
	mNearPlane(DEFAULT_NEAR_PLANE),
	mFarPlane(DEFAULT_FAR_PLANE),
	mFixedDistance(-1.f),
	mPlaneCount(6),
	mFrustumCornerDist(0.f)
{
	for (U32 i = 0; i < PLANE_MASK_NUM; i++)
	{
		mPlaneMask[i] = PLANE_MASK_NONE;
	}
	calculateFrustumPlanes();
}
LLCamera::LLCamera(F32 vertical_fov_rads, F32 aspect_ratio, S32 view_height_in_pixels, F32 near_plane, F32 far_plane) :
	LLCoordFrame(),
	mViewHeightInPixels(view_height_in_pixels),
	mFixedDistance(-1.f),
	mPlaneCount(6),
	mFrustumCornerDist(0.f)
{
	for (U32 i = 0; i < PLANE_MASK_NUM; i++)
	{
		mPlaneMask[i] = PLANE_MASK_NONE;
	}
	mAspect = llclamp(aspect_ratio, MIN_ASPECT_RATIO, MAX_ASPECT_RATIO);
	mNearPlane = llclamp(near_plane, MIN_NEAR_PLANE, MAX_NEAR_PLANE);
	if(far_plane < 0) far_plane = DEFAULT_FAR_PLANE;
	mFarPlane = llclamp(far_plane, MIN_FAR_PLANE, MAX_FAR_PLANE);
	setView(vertical_fov_rads);
}
LLCamera::~LLCamera()
{
}
F32 LLCamera::getMinView() const
{
	return mAspect > 1
		? MIN_FIELD_OF_VIEW
		: MIN_FIELD_OF_VIEW * 1/mAspect;
}
F32 LLCamera::getMaxView() const
{
	return mAspect > 1
		? MAX_FIELD_OF_VIEW / mAspect
		: MAX_FIELD_OF_VIEW;
}
LLPlane LLCamera::getUserClipPlane() const
{
    return mAgentPlanes[AGENT_PLANE_USER_CLIP];
}
void LLCamera::setUserClipPlane(const LLPlane& plane)
{
	mPlaneCount = AGENT_PLANE_USER_CLIP_NUM;
	mAgentPlanes[AGENT_PLANE_USER_CLIP] = plane;
	mPlaneMask[AGENT_PLANE_USER_CLIP] = plane.calcPlaneMask();
}
void LLCamera::disableUserClipPlane()
{
	mPlaneCount = AGENT_PLANE_NO_USER_CLIP_NUM;
}
void LLCamera::setView(F32 vertical_fov_rads)
{
	mView = llclamp(vertical_fov_rads, MIN_FIELD_OF_VIEW, MAX_FIELD_OF_VIEW);
	calculateFrustumPlanes();
}
void LLCamera::setViewHeightInPixels(S32 height)
{
	mViewHeightInPixels = height;
	calculateFrustumPlanes();
}
void LLCamera::setAspect(F32 aspect_ratio)
{
	mAspect = llclamp(aspect_ratio, MIN_ASPECT_RATIO, MAX_ASPECT_RATIO);
	calculateFrustumPlanes();
}
void LLCamera::setNear(F32 near_plane)
{
	mNearPlane = llclamp(near_plane, MIN_NEAR_PLANE, MAX_NEAR_PLANE);
	calculateFrustumPlanes();
}
void LLCamera::setFar(F32 far_plane)
{
	mFarPlane = llclamp(far_plane, MIN_FAR_PLANE, MAX_FAR_PLANE);
	calculateFrustumPlanes();
}
size_t LLCamera::writeFrustumToBuffer(char *buffer) const
{
	memcpy(buffer, &mView, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(buffer, &mAspect, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(buffer, &mNearPlane, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(buffer, &mFarPlane, sizeof(F32));
	return 4*sizeof(F32);
}
size_t LLCamera::readFrustumFromBuffer(const char *buffer)
{
	memcpy(&mView, buffer, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(&mAspect, buffer, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(&mNearPlane, buffer, sizeof(F32));
	buffer += sizeof(F32);
	memcpy(&mFarPlane, buffer, sizeof(F32));
	return 4*sizeof(F32);
}
static	const LLVector4a sFrustumScaler[] =
{
	LLVector4a(-1,-1,-1),
	LLVector4a( 1,-1,-1),
	LLVector4a(-1, 1,-1),
	LLVector4a( 1, 1,-1),
	LLVector4a(-1,-1, 1),
	LLVector4a( 1,-1, 1),
	LLVector4a(-1, 1, 1),
	LLVector4a( 1, 1, 1)
};
bool LLCamera::isChanged()
{
	bool changed = false;
	for (U32 i = 0; i < mPlaneCount; i++)
{
		U8 mask = mPlaneMask[i];
		if (mask != 0xff && !changed)
		{
			changed = !mAgentPlanes[i].equal(mLastAgentPlanes[i]);
		}
		mLastAgentPlanes[i].set(mAgentPlanes[i]);
	}
	return changed;
}
S32 LLCamera::AABBInFrustum(const LLVector4a &center, const LLVector4a& radius, const LLPlane* planes)
{
	if(!planes)
	{
		planes = mAgentPlanes;
	}
	U8 mask = 0;
	bool result = false;
	LLVector4a rscale, maxp, minp;
	LLSimdScalar d;
	U32 max_planes = llmin(mPlaneCount, (U32) AGENT_PLANE_USER_CLIP_NUM);
	for (U32 i = 0; i < max_planes; i++)
	{
		mask = mPlaneMask[i];
		if (mask < PLANE_MASK_NUM)
		{
			const LLPlane& p(planes[i]);
			p.getAt<3>(d);
			rscale.setMul(radius, sFrustumScaler[mask]);
			minp.setSub(center, rscale);
			d = -d;
			if (p.dot3(minp).getF32() > d)
			{
				return 0;
			}
			if(!result)
			{
				maxp.setAdd(center, rscale);
				result = (p.dot3(maxp).getF32() > d);
			}
		}
	}
	return result?1:2;
}
S32 LLCamera::AABBInRegionFrustum(const LLVector4a& center, const LLVector4a& radius)
{
	return AABBInFrustum(center, radius, mRegionPlanes);
}
S32 LLCamera::AABBInFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius, const LLPlane* planes)
{
	if(!planes)
	{
		planes = mAgentPlanes;
	}
	U8 mask = 0;
	bool result = false;
	LLVector4a rscale, maxp, minp;
	LLSimdScalar d;
	U32 max_planes = llmin(mPlaneCount, (U32) AGENT_PLANE_USER_CLIP_NUM);
	for (U32 i = 0; i < max_planes; i++)
	{
		mask = mPlaneMask[i];
		if ((i != 5) && (mask < PLANE_MASK_NUM))
		{
			const LLPlane& p(planes[i]);
			p.getAt<3>(d);
			rscale.setMul(radius, sFrustumScaler[mask]);
			minp.setSub(center, rscale);
			d = -d;
			if (p.dot3(minp).getF32() > d)
			{
				return 0;
			}
			if(!result)
			{
				maxp.setAdd(center, rscale);
				result = (p.dot3(maxp).getF32() > d);
			}
		}
	}
	return result?1:2;
}
S32 LLCamera::AABBInRegionFrustumNoFarClip(const LLVector4a& center, const LLVector4a& radius)
{
	return AABBInFrustumNoFarClip(center, radius, mRegionPlanes);
}
int LLCamera::sphereInFrustumQuick(const LLVector3 &sphere_center, const F32 radius)
{
	LLVector3 dist = sphere_center-mFrustCenter;
	float dsq = dist * dist;
	float rsq = mFarPlane*0.5f + radius;
	rsq *= rsq;
	if (dsq < rsq)
	{
		return 1;
	}
	return 0;
}
int LLCamera::sphereInFrustumOld(const LLVector3 &sphere_center, const F32 radius) const
{
	F32 x, y, z, rightDist, leftDist, topDist, bottomDist;
	LLVector3 rel_center(sphere_center);
	rel_center -= mOrigin;
	bool all_in = TRUE;
	x = mXAxis * rel_center;
	if (x < MIN_NEAR_PLANE - radius)
	{
		return 0;
	}
	else if (x < MIN_NEAR_PLANE + radius)
	{
		all_in = FALSE;
	}
	if (x > mFarPlane + radius)
	{
		return 0;
	}
	else if (x > mFarPlane - radius)
	{
		all_in = FALSE;
	}
	y = mYAxis * rel_center;
	rightDist = x * mLocalPlanes[PLANE_RIGHT][VX] + y * mLocalPlanes[PLANE_RIGHT][VY];
	if (rightDist < -radius)
	{
		return 0;
	}
	else if (rightDist < radius)
	{
		all_in = FALSE;
	}
	leftDist = x * mLocalPlanes[PLANE_LEFT][VX] + y * mLocalPlanes[PLANE_LEFT][VY];
	if (leftDist < -radius)
	{
		return 0;
	}
	else if (leftDist < radius)
	{
		all_in = FALSE;
	}
	z = mZAxis * rel_center;
	topDist = x * mLocalPlanes[PLANE_TOP][VX] + z * mLocalPlanes[PLANE_TOP][VZ];
	if (topDist < -radius)
	{
		return 0;
	}
	else if (topDist < radius)
	{
		all_in = FALSE;
	}
	bottomDist = x * mLocalPlanes[PLANE_BOTTOM][VX] + z * mLocalPlanes[PLANE_BOTTOM][VZ];
	if (bottomDist < -radius)
	{
		return 0;
	}
	else if (bottomDist < radius)
	{
		all_in = FALSE;
	}
	if (all_in)
	{
		return 2;
	}
	return 1;
}
int LLCamera::sphereInFrustum(const LLVector3 &sphere_center, const F32 radius) const
{
	bool res = false;
	for (int i = 0; i < 6; i++)
	{
		if (mPlaneMask[i] != PLANE_MASK_NONE)
		{
			float d = mAgentPlanes[i].dist(sphere_center);
			if (d > radius)
			{
				return 0;
			}
			res = res || (d > -radius);
		}
	}
	return res?1:2;
}
F32 LLCamera::heightInPixels(const LLVector3 &center, F32 radius ) const
{
	if (radius == 0.f) return 0.f;
	if (mViewHeightInPixels > -1)
	{
		LLVector3 vec = center - mOrigin;
		F32 dist = vec.magVec();
		F32 angle = 2.0f * (F32) atan2(radius, dist);
		F32 fraction_of_fov = angle / mView;
		return (fraction_of_fov * mViewHeightInPixels);
	}
	else
	{
		return -1.0f;
	}
}
F32 LLCamera::visibleDistance(const LLVector3 &pos, F32 rad, F32 fudgedist, U32 planemask) const
{
	if (mFixedDistance > 0)
	{
		return mFixedDistance;
	}
	LLVector3 dvec = pos - mOrigin;
	F32 dist = dvec.magVec();
	if (dist > rad)
	{
 		F32 dp,tdist;
 		dp = dvec * mXAxis;
  		if (dp < -rad)
  			return -dist;
		rad *= fudgedist;
		LLVector3 tvec(pos);
		for (int p=0; p<PLANE_NUM; p++)
		{
			if (!(planemask & (1<<p)))
				continue;
			tdist = -(mWorldPlanes[p].dist(tvec));
			if (tdist > rad)
				return -dist;
		}
	}
	return dist;
}
F32 LLCamera::visibleHorizDistance(const LLVector3 &pos, F32 rad, F32 fudgedist, U32 planemask) const
{
	if (mFixedDistance > 0)
	{
		return mFixedDistance;
	}
	LLVector3 dvec = pos - mOrigin;
	F32 dist = dvec.magVec();
	if (dist > rad)
	{
		rad *= fudgedist;
		LLVector3 tvec(pos);
		for (int p=0; p<HORIZ_PLANE_NUM; p++)
		{
			if (!(planemask & (1<<p)))
				continue;
			F32 tdist = -(mHorizPlanes[p].dist(tvec));
			if (tdist > rad)
				return -dist;
		}
	}
	return dist;
}
std::ostream& operator<<(std::ostream &s, const LLCamera &C)
{
	s << "{ \n";
	s << "  Center = " << C.getOrigin() << "\n";
	s << "  AtAxis = " << C.getXAxis() << "\n";
	s << "  LeftAxis = " << C.getYAxis() << "\n";
	s << "  UpAxis = " << C.getZAxis() << "\n";
	s << "  View = " << C.getView() << "\n";
	s << "  Aspect = " << C.getAspect() << "\n";
	s << "  NearPlane   = " << C.mNearPlane << "\n";
	s << "  FarPlane    = " << C.mFarPlane << "\n";
	s << "  TopPlane    = " << C.mLocalPlanes[LLCamera::PLANE_TOP][VX] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_TOP][VY] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_TOP][VZ] << "\n";
	s << "  BottomPlane = " << C.mLocalPlanes[LLCamera::PLANE_BOTTOM][VX] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_BOTTOM][VY] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_BOTTOM][VZ] << "\n";
	s << "  LeftPlane   = " << C.mLocalPlanes[LLCamera::PLANE_LEFT][VX] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_LEFT][VY] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_LEFT][VZ] << "\n";
	s << "  RightPlane  = " << C.mLocalPlanes[LLCamera::PLANE_RIGHT][VX] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_RIGHT][VY] << "  "
							<< C.mLocalPlanes[LLCamera::PLANE_RIGHT][VZ] << "\n";
	s << "}";
	return s;
}
void LLCamera::calculateFrustumPlanes()
{
	F32 left,right,top,bottom;
	top = mFarPlane * (F32)tanf(0.5f * mView);
	bottom = -top;
	left = top * mAspect;
	right = -left;
	calculateFrustumPlanes(left, right, top, bottom);
}
LLPlane planeFromPoints(LLVector3 p1, LLVector3 p2, LLVector3 p3)
{
	LLVector3 n = ((p2-p1)%(p3-p1));
	n.normVec();
	return LLPlane(p1, n);
}
void LLCamera::ignoreAgentFrustumPlane(S32 idx)
{
	if (idx < 0 || idx > (S32) mPlaneCount)
	{
		return;
	}
	mPlaneMask[idx] = PLANE_MASK_NONE;
	mAgentPlanes[idx].clear();
}
void LLCamera::calcAgentFrustumPlanes(LLVector3* frust)
{
	for (int i = 0; i < AGENT_FRUSTRUM_NUM; i++)
	{
		mAgentFrustum[i] = frust[i];
	}
	mFrustumCornerDist = (frust[5] - getOrigin()).magVec();
	mAgentPlanes[AGENT_PLANE_NEAR] = planeFromPoints(frust[0], frust[1], frust[2]);
	mAgentPlanes[AGENT_PLANE_FAR] = planeFromPoints(frust[5], frust[4], frust[6]);
	mAgentPlanes[AGENT_PLANE_LEFT] = planeFromPoints(frust[4], frust[0], frust[7]);
	mAgentPlanes[AGENT_PLANE_RIGHT] = planeFromPoints(frust[1], frust[5], frust[6]);
	mAgentPlanes[AGENT_PLANE_TOP] = planeFromPoints(frust[3], frust[2], frust[6]);
	mAgentPlanes[AGENT_PLANE_BOTTOM] = planeFromPoints(frust[1], frust[0], frust[4]);
	for (U32 i = 0; i < mPlaneCount; i++)
	{
		mPlaneMask[i] = mAgentPlanes[i].calcPlaneMask();
	}
}
void LLCamera::calcRegionFrustumPlanes(const LLVector3& shift, F32 far_clip_distance)
{
	F32 far_w;
	{
		LLVector3 p = getOrigin();
		LLVector3 n(mAgentPlanes[5][0], mAgentPlanes[5][1], mAgentPlanes[5][2]);
		F32 dd = n * p;
		if(dd + mAgentPlanes[5][3] < 0)
		{
			far_w = -far_clip_distance - dd;
		}
		else
		{
			far_w = far_clip_distance - dd;
		}
		far_w += n * shift;
	}
	F32 d;
	LLVector3 n;
	for(S32 i = 0 ; i < 7; i++)
	{
		if (mPlaneMask[i] != 0xff)
		{
			n.setVec(mAgentPlanes[i][0], mAgentPlanes[i][1], mAgentPlanes[i][2]);
			if(i != 5)
			{
				d = mAgentPlanes[i][3] + n * shift;
			}
			else
			{
				d = far_w;
			}
			mRegionPlanes[i].setVec(n, d);
		}
	}
}
void LLCamera::calculateFrustumPlanes(F32 left, F32 right, F32 top, F32 bottom)
{
	LLVector3 a, b, c;
	a.setVec(0.0f, 0.0f, 0.0f);
	b.setVec(mFarPlane, right, top);
	c.setVec(mFarPlane, right, bottom);
	mLocalPlanes[PLANE_RIGHT].setVec(a, b, c);
	c.setVec(mFarPlane, left, top);
	mLocalPlanes[PLANE_TOP].setVec(a, c, b);
	b.setVec(mFarPlane, left, bottom);
	mLocalPlanes[PLANE_LEFT].setVec(a, b, c);
	c.setVec(mFarPlane, right, bottom);
	mLocalPlanes[PLANE_BOTTOM].setVec( a, c, b);
	static LLVector3 const X_AXIS(1.f, 0.f, 0.f);
	mFrustCenter = X_AXIS*mFarPlane*0.5f;
	mFrustCenter = transformToAbsolute(mFrustCenter);
	mFrustRadiusSquared = mFarPlane*0.5f;
	mFrustRadiusSquared *= mFrustRadiusSquared * 1.05f;
}
void LLCamera::calculateFrustumPlanesFromWindow(F32 x1, F32 y1, F32 x2, F32 y2)
{
	F32 bottom, top, left, right;
	F32 view_height = (F32)tanf(0.5f * mView) * mFarPlane;
	F32 view_width = view_height * mAspect;
	left = 	 x1 * -2.f * view_width;
	right =  x2 * -2.f * view_width;
	bottom = y1 * 2.f * view_height;
	top = 	 y2 * 2.f * view_height;
	calculateFrustumPlanes(left, right, top, bottom);
}
void LLCamera::calculateWorldFrustumPlanes()
{
	F32 d;
	LLVector3 center = mOrigin - mXAxis*mNearPlane;
	mWorldPlanePos = center;
	LLVector3 pnorm;
	for (int p = 0; p < PLANE_NUM; p++)
	{
		mLocalPlanes[p].getVector3(pnorm);
		LLVector3 norm = rotateToAbsolute(pnorm);
		norm.normVec();
		d = -(center * norm);
		mWorldPlanes[p] = LLPlane(norm, d);
	}
	LLVector3 zaxis(0, 0, 1.0f);
	F32 yaw = getYaw();
	{
		LLVector3 tnorm;
		mLocalPlanes[PLANE_LEFT].getVector3(tnorm);
		tnorm.rotVec(yaw, zaxis);
		d = -(mOrigin * tnorm);
		mHorizPlanes[HORIZ_PLANE_LEFT] = LLPlane(tnorm, d);
	}
	{
		LLVector3 tnorm;
		mLocalPlanes[PLANE_RIGHT].getVector3(tnorm);
		tnorm.rotVec(yaw, zaxis);
		d = -(mOrigin * tnorm);
		mHorizPlanes[HORIZ_PLANE_RIGHT] = LLPlane(tnorm, d);
	}
}
