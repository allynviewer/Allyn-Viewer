/** 
 * @file v4math.h
 * @brief LLVector4 class header file.
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
#ifndef LL_V4MATH_H
#define LL_V4MATH_H
#include "llerror.h"
#include "llmath.h"
#include "v3math.h"
#include "v2math.h"
class LLMatrix3;
class LLMatrix4;
class LLQuaternion;
static const U32 LENGTHOFVECTOR4 = 4;
class LLVector4
{
	public:
		F32 mV[LENGTHOFVECTOR4];
		LLVector4();
		explicit LLVector4(const F32 *vec);
		explicit LLVector4(const F64 *vec);
        explicit LLVector4(const LLVector2 &vec);
        explicit LLVector4(const LLVector2 &vec, F32 z, F32 w);
		explicit LLVector4(const LLVector3 &vec);
		explicit LLVector4(const LLVector3 &vec, F32 w);
        explicit LLVector4(const LLSD &sd);
		LLVector4(F32 x, F32 y, F32 z);
		LLVector4(F32 x, F32 y, F32 z, F32 w);
		LLSD getValue() const
		{
			LLSD ret;
			ret[0] = mV[0];
			ret[1] = mV[1];
			ret[2] = mV[2];
			ret[3] = mV[3];
			return ret;
		}
        void setValue(const LLSD& sd)
        {
            mV[0] = sd[0].asReal();
            mV[1] = sd[1].asReal();
            mV[2] = sd[2].asReal();
            mV[3] = sd[3].asReal();
        }
		inline BOOL isFinite() const;
		inline void	clear();
		inline void	clearVec();
		inline void	zeroVec();
		inline void	set(F32 x, F32 y, F32 z);
		inline void	set(F32 x, F32 y, F32 z, F32 w);
		inline void	set(const LLVector4 &vec);
		inline void	set(const LLVector3 &vec, F32 w = 1.f);
		inline void	set(const F32 *vec);
		inline void	setVec(F32 x, F32 y, F32 z);
		inline void	setVec(F32 x, F32 y, F32 z, F32 w);
		inline void	setVec(const LLVector4 &vec);
		inline void	setVec(const LLVector3 &vec, F32 w = 1.f);
		inline void	setVec(const F32 *vec);
		F32	length() const;
		F32	lengthSquared() const;
		F32	normalize();
		F32			magVec() const;
		F32			magVecSquared() const;
		F32			normVec();
		BOOL abs();
		BOOL isExactlyClear() const		{ return (mV[VW] == 1.0f) && !mV[VX] && !mV[VY] && !mV[VZ]; }
		BOOL isExactlyZero() const		{ return !mV[VW] && !mV[VX] && !mV[VY] && !mV[VZ]; }
		const LLVector4&	rotVec(F32 angle, const LLVector4 &vec);
		const LLVector4&	rotVec(F32 angle, F32 x, F32 y, F32 z);
		const LLVector4&	rotVec(const LLMatrix4 &mat);
		const LLVector4&	rotVec(const LLQuaternion &q);
		const LLVector4&	scaleVec(const LLVector4& vec);
		F32 operator[](int idx) const { return mV[idx]; }
		F32 &operator[](int idx) { return mV[idx]; }
		friend std::ostream&	 operator<<(std::ostream& s, const LLVector4 &a);
		friend LLVector4 operator+(const LLVector4 &a, const LLVector4 &b);
		friend LLVector4 operator-(const LLVector4 &a, const LLVector4 &b);
		friend F32  operator*(const LLVector4 &a, const LLVector4 &b);
		friend LLVector4 operator%(const LLVector4 &a, const LLVector4 &b);
		friend LLVector4 operator/(const LLVector4 &a, F32 k);
		friend LLVector4 operator*(const LLVector4 &a, F32 k);
		friend LLVector4 operator*(F32 k, const LLVector4 &a);
		friend bool operator==(const LLVector4 &a, const LLVector4 &b);
		friend bool operator!=(const LLVector4 &a, const LLVector4 &b);
		friend const LLVector4& operator+=(LLVector4 &a, const LLVector4 &b);
		friend const LLVector4& operator-=(LLVector4 &a, const LLVector4 &b);
		friend const LLVector4& operator%=(LLVector4 &a, const LLVector4 &b);
		friend const LLVector4& operator*=(LLVector4 &a, F32 k);
		friend const LLVector4& operator/=(LLVector4 &a, F32 k);
		friend LLVector4 operator-(const LLVector4 &a);
};
static_assert(std::is_trivially_copyable<LLVector4>::value, "LLVector4 must be a trivially copyable type");
F32 angle_between(const LLVector4 &a, const LLVector4 &b);
BOOL are_parallel(const LLVector4 &a, const LLVector4 &b, F32 epsilon=F_APPROXIMATELY_ZERO);
F32	dist_vec(const LLVector4 &a, const LLVector4 &b);
F32	dist_vec_squared(const LLVector4 &a, const LLVector4 &b);
LLVector3	vec4to3(const LLVector4 &vec);
LLVector4	vec3to4(const LLVector3 &vec);
LLVector4 lerp(const LLVector4 &a, const LLVector4 &b, F32 u);
inline LLVector4::LLVector4(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
}
inline LLVector4::LLVector4(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = 1.f;
}
inline LLVector4::LLVector4(F32 x, F32 y, F32 z, F32 w)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = w;
}
inline LLVector4::LLVector4(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}
inline LLVector4::LLVector4(const F64 *vec)
{
	mV[VX] = (F32) vec[VX];
	mV[VY] = (F32) vec[VY];
	mV[VZ] = (F32) vec[VZ];
	mV[VW] = (F32) vec[VW];
}
inline LLVector4::LLVector4(const LLVector2 &vec)
{
    mV[VX] = vec[VX];
    mV[VY] = vec[VY];
    mV[VZ] = 0.f;
    mV[VW] = 0.f;
}
inline LLVector4::LLVector4(const LLVector2 &vec, F32 z, F32 w)
{
    mV[VX] = vec[VX];
    mV[VY] = vec[VY];
    mV[VZ] = z;
    mV[VW] = w;
}
inline LLVector4::LLVector4(const LLVector3 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = 1.f;
}
inline LLVector4::LLVector4(const LLVector3 &vec, F32 w)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = w;
}
inline LLVector4::LLVector4(const LLSD &sd)
{
    setValue(sd);
}
inline BOOL LLVector4::isFinite() const
{
	return (std::isfinite(mV[VX]) && std::isfinite(mV[VY]) && std::isfinite(mV[VZ]) && std::isfinite(mV[VW]));
}
inline void	LLVector4::clear(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
}
inline void	LLVector4::clearVec(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 1.f;
}
inline void	LLVector4::zeroVec(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
	mV[VZ] = 0.f;
	mV[VW] = 0.f;
}
inline void	LLVector4::set(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = 1.f;
}
inline void	LLVector4::set(F32 x, F32 y, F32 z, F32 w)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = w;
}
inline void	LLVector4::set(const LLVector4 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
}
inline void	LLVector4::set(const LLVector3 &vec, F32 w)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = w;
}
inline void	LLVector4::set(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}
inline void	LLVector4::setVec(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = 1.f;
}
inline void	LLVector4::setVec(F32 x, F32 y, F32 z, F32 w)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
	mV[VW] = w;
}
inline void	LLVector4::setVec(const LLVector4 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = vec.mV[VW];
}
inline void	LLVector4::setVec(const LLVector3 &vec, F32 w)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = w;
}
inline void	LLVector4::setVec(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
	mV[VW] = vec[VW];
}
inline F32		LLVector4::length(void) const
{
	return (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
}
inline F32		LLVector4::lengthSquared(void) const
{
	return mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ];
}
inline F32		LLVector4::magVec(void) const
{
	return (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
}
inline F32		LLVector4::magVecSquared(void) const
{
	return mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ];
}
inline LLVector4 operator+(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 c(a);
	return c += b;
}
inline LLVector4 operator-(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 c(a);
	return c -= b;
}
inline F32  operator*(const LLVector4 &a, const LLVector4 &b)
{
	return (a.mV[VX]*b.mV[VX] + a.mV[VY]*b.mV[VY] + a.mV[VZ]*b.mV[VZ]);
}
inline LLVector4 operator%(const LLVector4 &a, const LLVector4 &b)
{
	return LLVector4(a.mV[VY]*b.mV[VZ] - b.mV[VY]*a.mV[VZ], a.mV[VZ]*b.mV[VX] - b.mV[VZ]*a.mV[VX], a.mV[VX]*b.mV[VY] - b.mV[VX]*a.mV[VY]);
}
inline LLVector4 operator/(const LLVector4 &a, F32 k)
{
	F32 t = 1.f / k;
	return LLVector4( a.mV[VX] * t, a.mV[VY] * t, a.mV[VZ] * t );
}
inline LLVector4 operator*(const LLVector4 &a, F32 k)
{
	return LLVector4( a.mV[VX] * k, a.mV[VY] * k, a.mV[VZ] * k );
}
inline LLVector4 operator*(F32 k, const LLVector4 &a)
{
	return LLVector4( a.mV[VX] * k, a.mV[VY] * k, a.mV[VZ] * k );
}
inline bool operator==(const LLVector4 &a, const LLVector4 &b)
{
	return (  (a.mV[VX] == b.mV[VX])
			&&(a.mV[VY] == b.mV[VY])
			&&(a.mV[VZ] == b.mV[VZ]));
}
inline bool operator!=(const LLVector4 &a, const LLVector4 &b)
{
	return (  (a.mV[VX] != b.mV[VX])
			||(a.mV[VY] != b.mV[VY])
			||(a.mV[VZ] != b.mV[VZ])
			||(a.mV[VW] != b.mV[VW]) );
}
inline const LLVector4& operator+=(LLVector4 &a, const LLVector4 &b)
{
	a.mV[VX] += b.mV[VX];
	a.mV[VY] += b.mV[VY];
	a.mV[VZ] += b.mV[VZ];
	return a;
}
inline const LLVector4& operator-=(LLVector4 &a, const LLVector4 &b)
{
	a.mV[VX] -= b.mV[VX];
	a.mV[VY] -= b.mV[VY];
	a.mV[VZ] -= b.mV[VZ];
	return a;
}
inline const LLVector4& operator%=(LLVector4 &a, const LLVector4 &b)
{
	LLVector4 ret(a.mV[VY]*b.mV[VZ] - b.mV[VY]*a.mV[VZ], a.mV[VZ]*b.mV[VX] - b.mV[VZ]*a.mV[VX], a.mV[VX]*b.mV[VY] - b.mV[VX]*a.mV[VY]);
	a = ret;
	return a;
}
inline const LLVector4& operator*=(LLVector4 &a, F32 k)
{
	a.mV[VX] *= k;
	a.mV[VY] *= k;
	a.mV[VZ] *= k;
	return a;
}
inline const LLVector4& operator/=(LLVector4 &a, F32 k)
{
	F32 t = 1.f / k;
	a.mV[VX] *= t;
	a.mV[VY] *= t;
	a.mV[VZ] *= t;
	return a;
}
inline LLVector4 operator-(const LLVector4 &a)
{
	return LLVector4( -a.mV[VX], -a.mV[VY], -a.mV[VZ] );
}
inline F32	dist_vec(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 vec = a - b;
	return (vec.length());
}
inline F32	dist_vec_squared(const LLVector4 &a, const LLVector4 &b)
{
	LLVector4 vec = a - b;
	return (vec.lengthSquared());
}
inline LLVector4 lerp(const LLVector4 &a, const LLVector4 &b, F32 u)
{
	return LLVector4(
		a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u,
		a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u,
		a.mV[VZ] + (b.mV[VZ] - a.mV[VZ]) * u,
		a.mV[VW] + (b.mV[VW] - a.mV[VW]) * u);
}
inline F32		LLVector4::normalize(void)
{
	F32 mag = (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
	F32 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[VX] *= oomag;
		mV[VY] *= oomag;
		mV[VZ] *= oomag;
	}
	else
	{
		mV[0] = 0.f;
		mV[1] = 0.f;
		mV[2] = 0.f;
		mag = 0;
	}
	return (mag);
}
inline F32		LLVector4::normVec(void)
{
	F32 mag = (F32) sqrt(mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ]);
	F32 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[VX] *= oomag;
		mV[VY] *= oomag;
		mV[VZ] *= oomag;
	}
	else
	{
		mV[0] = 0.f;
		mV[1] = 0.f;
		mV[2] = 0.f;
		mag = 0;
	}
	return (mag);
}
#endif
