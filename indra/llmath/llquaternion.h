/** 
 * @file llquaternion.h
 * @brief LLQuaternion class header file.
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
#ifndef LLQUATERNION_H
#define LLQUATERNION_H
#include <iostream>
#include "llsd.h"
#ifndef LLMATH_H
#error "Please include llmath.h first."
#endif
class LLVector4;
class LLVector3;
class LLVector3d;
class LLMatrix4;
class LLMatrix3;
static const U32 LENGTHOFQUAT = 4;
class LLQuaternion
{
public:
	F32 mQ[LENGTHOFQUAT];
	static const LLQuaternion DEFAULT;
	LLQuaternion();
	explicit LLQuaternion(const LLMatrix4 &mat);
	explicit LLQuaternion(const LLMatrix3 &mat);
	LLQuaternion(F32 x, F32 y, F32 z, F32 w);
	LLQuaternion(F32 angle, const LLVector4 &vec);
	LLQuaternion(F32 angle, const LLVector3 &vec);
	LLQuaternion(const F32 *q);
	LLQuaternion(const LLVector3 &x_axis,
				 const LLVector3 &y_axis,
				 const LLVector3 &z_axis);
    explicit LLQuaternion(const LLSD &sd);
    LLSD getValue() const;
    void setValue(const LLSD& sd);
	BOOL isIdentity() const;
	BOOL isNotIdentity() const;
	BOOL isFinite() const;
	void quantize16(F32 lower, F32 upper);
	void quantize8(F32 lower, F32 upper);
	void loadIdentity();
	bool isEqualEps(const LLQuaternion &quat, F32 epsilon) const;
	bool isNotEqualEps(const LLQuaternion &quat, F32 epsilon) const;
	const LLQuaternion&	set(F32 x, F32 y, F32 z, F32 w);
	const LLQuaternion&	set(const LLQuaternion &quat);
	const LLQuaternion&	set(const F32 *q);
	const LLQuaternion&	set(const LLMatrix3 &mat);
	const LLQuaternion&	set(const LLMatrix4 &mat);
    const LLQuaternion& setFromAzimuthAndAltitude(F32 azimuth, F32 altitude);
	const LLQuaternion&	setAngleAxis(F32 angle, F32 x, F32 y, F32 z);
	const LLQuaternion&	setAngleAxis(F32 angle, const LLVector3 &vec);
	const LLQuaternion&	setAngleAxis(F32 angle, const LLVector4 &vec);
	const LLQuaternion&	setEulerAngles(F32 roll, F32 pitch, F32 yaw);
	const LLQuaternion&	setQuatInit(F32 x, F32 y, F32 z, F32 w);
	const LLQuaternion&	setQuat(const LLQuaternion &quat);
	const LLQuaternion&	setQuat(const F32 *q);
	const LLQuaternion&	setQuat(const LLMatrix3 &mat);
	const LLQuaternion&	setQuat(const LLMatrix4 &mat);
	const LLQuaternion&	setQuat(F32 angle, F32 x, F32 y, F32 z);
	const LLQuaternion&	setQuat(F32 angle, const LLVector3 &vec);
	const LLQuaternion&	setQuat(F32 angle, const LLVector4 &vec);
	const LLQuaternion&	setQuat(F32 roll, F32 pitch, F32 yaw);
	LLMatrix4	getMatrix4(void) const;
	LLMatrix3	getMatrix3(void) const;
	void		getAngleAxis(F32* angle, F32* x, F32* y, F32* z) const;
	void		getAngleAxis(F32* angle, LLVector3 &vec) const;
	void		getEulerAngles(F32 *roll, F32* pitch, F32 *yaw) const;
    void        getAzimuthAndAltitude(F32 &azimuth, F32 &altitude);
	F32	normalize();
	F32	normQuat();
	const LLQuaternion&	conjugate(void);
	const LLQuaternion&	conjQuat(void);
	const LLQuaternion&	transpose();
	const LLQuaternion&	transQuat();
	void			shortestArc(const LLVector3 &a, const LLVector3 &b);
	const LLQuaternion& constrain(F32 radians);
	friend std::ostream& operator<<(std::ostream &s, const LLQuaternion &a);
	friend LLQuaternion operator+(const LLQuaternion &a, const LLQuaternion &b);
	friend LLQuaternion operator-(const LLQuaternion &a, const LLQuaternion &b);
	friend LLQuaternion operator-(const LLQuaternion &a);
	friend LLQuaternion operator*(F32 a, const LLQuaternion &q);
	friend LLQuaternion operator*(const LLQuaternion &q, F32 b);
	friend LLQuaternion operator*(const LLQuaternion &a, const LLQuaternion &b);
	friend LLQuaternion operator~(const LLQuaternion &a);
	bool operator==(const LLQuaternion &b) const;
	bool operator!=(const LLQuaternion &b) const;
	friend const LLQuaternion& operator*=(LLQuaternion &a, const LLQuaternion &b);
	friend LLVector4 operator*(const LLVector4 &a, const LLQuaternion &rot);
	friend LLVector3 operator*(const LLVector3 &a, const LLQuaternion &rot);
	friend LLVector3d operator*(const LLVector3d &a, const LLQuaternion &rot);
	friend F32 dot(const LLQuaternion &a, const LLQuaternion &b);
	friend LLQuaternion lerp(F32 t, const LLQuaternion &p, const LLQuaternion &q);
	friend LLQuaternion lerp(F32 t, const LLQuaternion &q);
	friend LLQuaternion slerp(F32 t, const LLQuaternion &p, const LLQuaternion &q);
	friend LLQuaternion slerp(F32 t, const LLQuaternion &q);
	friend LLQuaternion nlerp(F32 t, const LLQuaternion &p, const LLQuaternion &q);
	friend LLQuaternion nlerp(F32 t, const LLQuaternion &q);
	LLVector3	packToVector3() const;
	void		unpackFromVector3(const LLVector3& vec);
	enum Order {
		XYZ = 0,
		YZX = 1,
		ZXY = 2,
		XZY = 3,
		YXZ = 4,
		ZYX = 5
	};
	friend LLQuaternion mayaQ(F32 x, F32 y, F32 z, Order order);
	friend const char *OrderToString( const Order order );
	friend Order StringToOrder( const char *str );
	static BOOL parseQuat(const std::string& buf, LLQuaternion* value);
};
inline LLSD LLQuaternion::getValue() const
{
    LLSD ret;
    ret[0] = mQ[0];
    ret[1] = mQ[1];
    ret[2] = mQ[2];
    ret[3] = mQ[3];
    return ret;
}
inline void LLQuaternion::setValue(const LLSD& sd)
{
    mQ[0] = sd[0].asReal();
    mQ[1] = sd[1].asReal();
    mQ[2] = sd[2].asReal();
    mQ[3] = sd[3].asReal();
}
static_assert(std::is_trivially_copyable<LLQuaternion>::value, "LLQuaternion must be a trivially copyable type");
inline BOOL	LLQuaternion::isFinite() const
{
	return (std::isfinite(mQ[VX]) && std::isfinite(mQ[VY]) && std::isfinite(mQ[VZ]) && std::isfinite(mQ[VS]));
}
inline BOOL LLQuaternion::isIdentity() const
{
	return
		( mQ[VX] == 0.f ) &&
		( mQ[VY] == 0.f ) &&
		( mQ[VZ] == 0.f ) &&
		( mQ[VS] == 1.f );
}
inline BOOL LLQuaternion::isNotIdentity() const
{
	return
		( mQ[VX] != 0.f ) ||
		( mQ[VY] != 0.f ) ||
		( mQ[VZ] != 0.f ) ||
		( mQ[VS] != 1.f );
}
inline LLQuaternion::LLQuaternion(void)
{
	mQ[VX] = 0.f;
	mQ[VY] = 0.f;
	mQ[VZ] = 0.f;
	mQ[VS] = 1.f;
}
inline LLQuaternion::LLQuaternion(F32 x, F32 y, F32 z, F32 w)
{
	mQ[VX] = x;
	mQ[VY] = y;
	mQ[VZ] = z;
	mQ[VS] = w;
}
inline LLQuaternion::LLQuaternion(const F32 *q)
{
	mQ[VX] = q[VX];
	mQ[VY] = q[VY];
	mQ[VZ] = q[VZ];
	mQ[VS] = q[VW];
	normalize();
}
inline void LLQuaternion::loadIdentity()
{
	mQ[VX] = 0.0f;
	mQ[VY] = 0.0f;
	mQ[VZ] = 0.0f;
	mQ[VW] = 1.0f;
}
inline bool LLQuaternion::isEqualEps(const LLQuaternion &quat, F32 epsilon) const
{
	return ( fabs(mQ[VX] - quat.mQ[VX]) < epsilon
		&&	 fabs(mQ[VY] - quat.mQ[VY]) < epsilon
		&&	 fabs(mQ[VZ] - quat.mQ[VZ]) < epsilon
		&&	 fabs(mQ[VS] - quat.mQ[VS]) < epsilon );
}
inline bool LLQuaternion::isNotEqualEps(const LLQuaternion &quat, F32 epsilon) const
{
	return (  fabs(mQ[VX] - quat.mQ[VX]) > epsilon
		||    fabs(mQ[VY] - quat.mQ[VY]) > epsilon
		||	  fabs(mQ[VZ] - quat.mQ[VZ]) > epsilon
		||    fabs(mQ[VS] - quat.mQ[VS]) > epsilon );
}
inline const LLQuaternion&	LLQuaternion::set(F32 x, F32 y, F32 z, F32 w)
{
	mQ[VX] = x;
	mQ[VY] = y;
	mQ[VZ] = z;
	mQ[VS] = w;
	normalize();
	return (*this);
}
inline const LLQuaternion&	LLQuaternion::set(const LLQuaternion &quat)
{
	mQ[VX] = quat.mQ[VX];
	mQ[VY] = quat.mQ[VY];
	mQ[VZ] = quat.mQ[VZ];
	mQ[VW] = quat.mQ[VW];
	normalize();
	return (*this);
}
inline const LLQuaternion&	LLQuaternion::set(const F32 *q)
{
	mQ[VX] = q[VX];
	mQ[VY] = q[VY];
	mQ[VZ] = q[VZ];
	mQ[VS] = q[VW];
	normalize();
	return (*this);
}
inline const LLQuaternion&	LLQuaternion::setQuatInit(F32 x, F32 y, F32 z, F32 w)
{
	mQ[VX] = x;
	mQ[VY] = y;
	mQ[VZ] = z;
	mQ[VS] = w;
	normalize();
	return (*this);
}
inline const LLQuaternion&	LLQuaternion::setQuat(const LLQuaternion &quat)
{
	mQ[VX] = quat.mQ[VX];
	mQ[VY] = quat.mQ[VY];
	mQ[VZ] = quat.mQ[VZ];
	mQ[VW] = quat.mQ[VW];
	normalize();
	return (*this);
}
inline const LLQuaternion&	LLQuaternion::setQuat(const F32 *q)
{
	mQ[VX] = q[VX];
	mQ[VY] = q[VY];
	mQ[VZ] = q[VZ];
	mQ[VS] = q[VW];
	normalize();
	return (*this);
}
inline void LLQuaternion::getAngleAxis(F32* angle, F32* x, F32* y, F32* z) const
{
	F32 v = sqrtf(mQ[VX] * mQ[VX] + mQ[VY] * mQ[VY] + mQ[VZ] * mQ[VZ]);
	if (v > FP_MAG_THRESHOLD)
	{
		F32 oomag = 1.0f / v;
		F32 w = mQ[VW];
		if (w < 0.0f)
		{
			w = -w;
			oomag = -oomag;
		}
		*x = mQ[VX] * oomag;
		*y = mQ[VY] * oomag;
		*z = mQ[VZ] * oomag;
		*angle = 2.0f * atan2f(v, w);
	}
	else
	{
		*angle = 0.0f;
		*x = 0.0f;
		*y = 0.0f;
		*z = 1.0f;
	}
}
inline const LLQuaternion& LLQuaternion::conjugate()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}
inline const LLQuaternion& LLQuaternion::conjQuat()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}
inline const LLQuaternion& LLQuaternion::transpose()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}
inline const LLQuaternion& LLQuaternion::transQuat()
{
	mQ[VX] *= -1.f;
	mQ[VY] *= -1.f;
	mQ[VZ] *= -1.f;
	return (*this);
}
inline LLQuaternion 	operator+(const LLQuaternion &a, const LLQuaternion &b)
{
	return LLQuaternion(
		a.mQ[VX] + b.mQ[VX],
		a.mQ[VY] + b.mQ[VY],
		a.mQ[VZ] + b.mQ[VZ],
		a.mQ[VW] + b.mQ[VW] );
}
inline LLQuaternion 	operator-(const LLQuaternion &a, const LLQuaternion &b)
{
	return LLQuaternion(
		a.mQ[VX] - b.mQ[VX],
		a.mQ[VY] - b.mQ[VY],
		a.mQ[VZ] - b.mQ[VZ],
		a.mQ[VW] - b.mQ[VW] );
}
inline LLQuaternion 	operator-(const LLQuaternion &a)
{
	return LLQuaternion(
		-a.mQ[VX],
		-a.mQ[VY],
		-a.mQ[VZ],
		-a.mQ[VW] );
}
inline LLQuaternion 	operator*(F32 a, const LLQuaternion &q)
{
	return LLQuaternion(
		a * q.mQ[VX],
		a * q.mQ[VY],
		a * q.mQ[VZ],
		a * q.mQ[VW] );
}
inline LLQuaternion 	operator*(const LLQuaternion &q, F32 a)
{
	return LLQuaternion(
		a * q.mQ[VX],
		a * q.mQ[VY],
		a * q.mQ[VZ],
		a * q.mQ[VW] );
}
inline LLQuaternion	operator~(const LLQuaternion &a)
{
	LLQuaternion q(a);
	q.conjQuat();
	return q;
}
inline bool	LLQuaternion::operator==(const LLQuaternion &b) const
{
	return (  (mQ[VX] == b.mQ[VX])
			&&(mQ[VY] == b.mQ[VY])
			&&(mQ[VZ] == b.mQ[VZ])
			&&(mQ[VS] == b.mQ[VS]));
}
inline bool	LLQuaternion::operator!=(const LLQuaternion &b) const
{
	return (  (mQ[VX] != b.mQ[VX])
			||(mQ[VY] != b.mQ[VY])
			||(mQ[VZ] != b.mQ[VZ])
			||(mQ[VS] != b.mQ[VS]));
}
inline const LLQuaternion&	operator*=(LLQuaternion &a, const LLQuaternion &b)
{
#if 1
	LLQuaternion q(
		b.mQ[3] * a.mQ[0] + b.mQ[0] * a.mQ[3] + b.mQ[1] * a.mQ[2] - b.mQ[2] * a.mQ[1],
		b.mQ[3] * a.mQ[1] + b.mQ[1] * a.mQ[3] + b.mQ[2] * a.mQ[0] - b.mQ[0] * a.mQ[2],
		b.mQ[3] * a.mQ[2] + b.mQ[2] * a.mQ[3] + b.mQ[0] * a.mQ[1] - b.mQ[1] * a.mQ[0],
		b.mQ[3] * a.mQ[3] - b.mQ[0] * a.mQ[0] - b.mQ[1] * a.mQ[1] - b.mQ[2] * a.mQ[2]
	);
	a = q;
#else
	a = a * b;
#endif
	return a;
}
const F32 ONE_PART_IN_A_MILLION = 0.000001f;
inline F32	LLQuaternion::normalize()
{
	F32 mag = sqrtf(mQ[VX]*mQ[VX] + mQ[VY]*mQ[VY] + mQ[VZ]*mQ[VZ] + mQ[VS]*mQ[VS]);
	if (mag > FP_MAG_THRESHOLD)
	{
		if (fabs(1.f - mag) > ONE_PART_IN_A_MILLION)
		{
			F32 oomag = 1.f/mag;
			mQ[VX] *= oomag;
			mQ[VY] *= oomag;
			mQ[VZ] *= oomag;
			mQ[VS] *= oomag;
		}
	}
	else
	{
		mQ[VX] = 0.f;
		mQ[VY] = 0.f;
		mQ[VZ] = 0.f;
		mQ[VS] = 1.f;
	}
	return mag;
}
inline F32	LLQuaternion::normQuat()
{
	F32 mag = sqrtf(mQ[VX]*mQ[VX] + mQ[VY]*mQ[VY] + mQ[VZ]*mQ[VZ] + mQ[VS]*mQ[VS]);
	if (mag > FP_MAG_THRESHOLD)
	{
		if (fabs(1.f - mag) > ONE_PART_IN_A_MILLION)
		{
			F32 oomag = 1.f/mag;
			mQ[VX] *= oomag;
			mQ[VY] *= oomag;
			mQ[VZ] *= oomag;
			mQ[VS] *= oomag;
		}
	}
	else
	{
		mQ[VX] = 0.f;
		mQ[VY] = 0.f;
		mQ[VZ] = 0.f;
		mQ[VS] = 1.f;
	}
	return mag;
}
LLQuaternion::Order StringToOrder( const char *str );
#endif
