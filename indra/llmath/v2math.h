/** 
 * @file v2math.h
 * @brief LLVector2 class header file.
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
#ifndef LL_V2MATH_H
#define LL_V2MATH_H
#include "llmath.h"
#include "v3math.h"
class LLVector4;
class LLMatrix3;
class LLQuaternion;
static const U32 LENGTHOFVECTOR2 = 2;
class LLVector2
{
	public:
		F32 mV[LENGTHOFVECTOR2];
		static LLVector2 zero;
		LLVector2();
		LLVector2(F32 x, F32 y);
		LLVector2(const F32 *vec);
        explicit LLVector2(const LLVector3 &vec);
        explicit LLVector2(const LLSD &sd);
		void	clear();
		void	setZero();
		void	clearVec();
		void	zeroVec();
		void	set(F32 x, F32 y);
		void	set(const LLVector2 &vec);
		void	set(const F32 *vec);
		LLSD	getValue() const;
		void	setValue(const LLSD& sd);
		void	setVec(F32 x, F32 y);
		void	setVec(const LLVector2 &vec);
		void	setVec(const F32 *vec);
		inline bool isFinite() const;
		F32		length() const;
		F32		lengthSquared() const;
		F32		normalize();
		F32		magVec() const;
		F32		magVecSquared() const;
		F32		normVec();
		BOOL		abs();
		const LLVector2&	scaleVec(const LLVector2& vec);
		BOOL isNull();
		BOOL isExactlyZero() const		{ return !mV[VX] && !mV[VY]; }
		F32 operator[](int idx) const { return mV[idx]; }
		F32 &operator[](int idx) { return mV[idx]; }
		friend bool operator<(const LLVector2 &a, const LLVector2 &b);
		friend LLVector2 operator+(const LLVector2 &a, const LLVector2 &b);
		friend LLVector2 operator-(const LLVector2 &a, const LLVector2 &b);
		friend F32 operator*(const LLVector2 &a, const LLVector2 &b);
		friend LLVector2 operator%(const LLVector2 &a, const LLVector2 &b);
		friend LLVector2 operator/(const LLVector2 &a, F32 k);
		friend LLVector2 operator*(const LLVector2 &a, F32 k);
		friend LLVector2 operator*(F32 k, const LLVector2 &a);
		friend bool operator==(const LLVector2 &a, const LLVector2 &b);
		friend bool operator!=(const LLVector2 &a, const LLVector2 &b);
		friend const LLVector2& operator+=(LLVector2 &a, const LLVector2 &b);
		friend const LLVector2& operator-=(LLVector2 &a, const LLVector2 &b);
		friend const LLVector2& operator%=(LLVector2 &a, const LLVector2 &b);
		friend const LLVector2& operator*=(LLVector2 &a, F32 k);
		friend const LLVector2& operator/=(LLVector2 &a, F32 k);
		friend LLVector2 operator-(const LLVector2 &a);
		friend std::ostream&	 operator<<(std::ostream& s, const LLVector2 &a);
};
static_assert(std::is_trivially_copyable<LLVector2>::value, "LLVector2 must be a trivially copyable type");
F32	angle_between(const LLVector2 &a, const LLVector2 &b);
BOOL are_parallel(const LLVector2 &a, const LLVector2 &b, F32 epsilon=F_APPROXIMATELY_ZERO);
F32	dist_vec(const LLVector2 &a, const LLVector2 &b);
F32	dist_vec_squared(const LLVector2 &a, const LLVector2 &b);
F32	dist_vec_squared2D(const LLVector2 &a, const LLVector2 &b);
LLVector2 lerp(const LLVector2 &a, const LLVector2 &b, F32 u);
inline LLVector2::LLVector2(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
}
inline LLVector2::LLVector2(F32 x, F32 y)
{
	mV[VX] = x;
	mV[VY] = y;
}
inline LLVector2::LLVector2(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
}
inline LLVector2::LLVector2(const LLVector3 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
}
inline LLVector2::LLVector2(const LLSD &sd)
{
    setValue(sd);
}
inline void	LLVector2::clear(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
}
inline void	LLVector2::setZero(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
}
inline void	LLVector2::clearVec(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
}
inline void	LLVector2::zeroVec(void)
{
	mV[VX] = 0.f;
	mV[VY] = 0.f;
}
inline void	LLVector2::set(F32 x, F32 y)
{
	mV[VX] = x;
	mV[VY] = y;
}
inline void	LLVector2::set(const LLVector2 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
}
inline void	LLVector2::set(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
}
inline void	LLVector2::setVec(F32 x, F32 y)
{
	mV[VX] = x;
	mV[VY] = y;
}
inline void	LLVector2::setVec(const LLVector2 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
}
inline void	LLVector2::setVec(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
}
inline F32 LLVector2::length(void) const
{
	return (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1]);
}
inline F32 LLVector2::lengthSquared(void) const
{
	return mV[0]*mV[0] + mV[1]*mV[1];
}
inline F32		LLVector2::normalize(void)
{
	F32 mag = (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1]);
	F32 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[0] *= oomag;
		mV[1] *= oomag;
	}
	else
	{
		mV[0] = 0.f;
		mV[1] = 0.f;
		mag = 0;
	}
	return (mag);
}
inline bool LLVector2::isFinite() const
{
	return (std::isfinite(mV[VX]) && std::isfinite(mV[VY]));
}
inline F32		LLVector2::magVec(void) const
{
	return (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1]);
}
inline F32		LLVector2::magVecSquared(void) const
{
	return mV[0]*mV[0] + mV[1]*mV[1];
}
inline F32		LLVector2::normVec(void)
{
	F32 mag = (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1]);
	F32 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[0] *= oomag;
		mV[1] *= oomag;
	}
	else
	{
		mV[0] = 0.f;
		mV[1] = 0.f;
		mag = 0;
	}
	return (mag);
}
inline const LLVector2&	LLVector2::scaleVec(const LLVector2& vec)
{
	mV[VX] *= vec.mV[VX];
	mV[VY] *= vec.mV[VY];
	return *this;
}
inline BOOL	LLVector2::isNull()
{
	if ( F_APPROXIMATELY_ZERO > mV[VX]*mV[VX] + mV[VY]*mV[VY] )
	{
		return TRUE;
	}
	return FALSE;
}
inline bool operator<(const LLVector2 &a, const LLVector2 &b)
{
	if( a.mV[VX] == b.mV[VX] )
	{
		return a.mV[VY] < b.mV[VY];
	}
	else
	{
		return a.mV[VX] < b.mV[VX];
	}
}
inline LLVector2 operator+(const LLVector2 &a, const LLVector2 &b)
{
	LLVector2 c(a);
	return c += b;
}
inline LLVector2 operator-(const LLVector2 &a, const LLVector2 &b)
{
	LLVector2 c(a);
	return c -= b;
}
inline F32  operator*(const LLVector2 &a, const LLVector2 &b)
{
	return (a.mV[0]*b.mV[0] + a.mV[1]*b.mV[1]);
}
inline LLVector2 operator%(const LLVector2 &a, const LLVector2 &b)
{
	return LLVector2(a.mV[0]*b.mV[1] - b.mV[0]*a.mV[1], a.mV[1]*b.mV[0] - b.mV[1]*a.mV[0]);
}
inline LLVector2 operator/(const LLVector2 &a, F32 k)
{
	F32 t = 1.f / k;
	return LLVector2( a.mV[0] * t, a.mV[1] * t );
}
inline LLVector2 operator*(const LLVector2 &a, F32 k)
{
	return LLVector2( a.mV[0] * k, a.mV[1] * k );
}
inline LLVector2 operator*(F32 k, const LLVector2 &a)
{
	return LLVector2( a.mV[0] * k, a.mV[1] * k );
}
inline bool operator==(const LLVector2 &a, const LLVector2 &b)
{
	return (  (a.mV[0] == b.mV[0])
			&&(a.mV[1] == b.mV[1]));
}
inline bool operator!=(const LLVector2 &a, const LLVector2 &b)
{
	return (  (a.mV[0] != b.mV[0])
			||(a.mV[1] != b.mV[1]));
}
inline const LLVector2& operator+=(LLVector2 &a, const LLVector2 &b)
{
	a.mV[0] += b.mV[0];
	a.mV[1] += b.mV[1];
	return a;
}
inline const LLVector2& operator-=(LLVector2 &a, const LLVector2 &b)
{
	a.mV[0] -= b.mV[0];
	a.mV[1] -= b.mV[1];
	return a;
}
inline const LLVector2& operator%=(LLVector2 &a, const LLVector2 &b)
{
	LLVector2 ret(a.mV[0]*b.mV[1] - b.mV[0]*a.mV[1], a.mV[1]*b.mV[0] - b.mV[1]*a.mV[0]);
	a = ret;
	return a;
}
inline const LLVector2& operator*=(LLVector2 &a, F32 k)
{
	a.mV[0] *= k;
	a.mV[1] *= k;
	return a;
}
inline const LLVector2& operator/=(LLVector2 &a, F32 k)
{
	F32 t = 1.f / k;
	a.mV[0] *= t;
	a.mV[1] *= t;
	return a;
}
inline LLVector2 operator-(const LLVector2 &a)
{
	return LLVector2( -a.mV[0], -a.mV[1] );
}
inline void update_min_max(LLVector2& min, LLVector2& max, const LLVector2& pos)
{
	for (U32 i = 0; i < 2; i++)
	{
		if (min.mV[i] > pos.mV[i])
		{
			min.mV[i] = pos.mV[i];
		}
		if (max.mV[i] < pos.mV[i])
		{
			max.mV[i] = pos.mV[i];
		}
	}
}
inline std::ostream& operator<<(std::ostream& s, const LLVector2 &a)
{
	s << "{ " << a.mV[VX] << ", " << a.mV[VY] << " }";
	return s;
}
#endif
