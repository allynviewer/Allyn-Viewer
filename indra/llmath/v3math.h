/** 
 * @file v3math.h
 * @brief LLVector3 class header file.
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
#ifndef LL_V3MATH_H
#define LL_V3MATH_H
#include "llerror.h"
#include "llmath.h"
#include "llsd.h"
class LLVector2;
class LLVector4;
class LLMatrix3;
class LLMatrix4;
class LLVector3d;
class LLQuaternion;
static const U32 LENGTHOFVECTOR3 = 3;
class LLVector3
{
	public:
		F32 mV[LENGTHOFVECTOR3];
		static const LLVector3 zero;
		static const LLVector3 x_axis;
		static const LLVector3 y_axis;
		static const LLVector3 z_axis;
		static const LLVector3 x_axis_neg;
		static const LLVector3 y_axis_neg;
		static const LLVector3 z_axis_neg;
		static const LLVector3 all_one;
		inline LLVector3();
		inline LLVector3(const F32 x, const F32 y, const F32 z);
		inline explicit LLVector3(const F32 *vec);
		explicit LLVector3(const LLVector2 &vec);
		explicit LLVector3(const LLVector3d &vec);
		explicit LLVector3(const LLVector4 &vec);
		LLVector3(const LLSD& sd);
		LLSD getValue() const;
		void setValue(const LLSD& sd);
		inline BOOL isFinite() const;
		BOOL		clamp(F32 min, F32 max);
		BOOL		clamp(const LLVector3 &min_vec, const LLVector3 &max_vec);
		BOOL		clampLength( F32 length_limit );
		void		quantize16(F32 lowerxy, F32 upperxy, F32 lowerz, F32 upperz);
		void		quantize8(F32 lowerxy, F32 upperxy, F32 lowerz, F32 upperz);
		void 		snap(S32 sig_digits);
		BOOL		abs();
		inline void	clear();
		inline void	setZero();
		inline void	clearVec();
		inline void	zeroVec();
		inline void	set(F32 x, F32 y, F32 z);
		inline void	set(const LLVector3 &vec);
		inline void	set(const F32 *vec);
		const LLVector3& set(const LLVector4 &vec);
		const LLVector3& set(const LLVector3d &vec);
		inline void	setVec(F32 x, F32 y, F32 z);
		inline void	setVec(const LLVector3 &vec);
		inline void	setVec(const F32 *vec);
		const LLVector3& setVec(const LLVector4 &vec);
		const LLVector3& setVec(const LLVector3d &vec);
		F32		length() const;
		F32		lengthSquared() const;
		F32		magVec() const;
		F32		magVecSquared() const;
		inline F32		normalize();
		inline F32		normVec();
		inline BOOL inRange( F32 min, F32 max ) const;
		const LLVector3&	rotVec(F32 angle, const LLVector3 &vec);
		const LLVector3&	rotVec(F32 angle, F32 x, F32 y, F32 z);
		const LLVector3&	rotVec(const LLMatrix3 &mat);
		const LLVector3&	rotVec(const LLQuaternion &q);
		const LLVector3&	transVec(const LLMatrix4& mat);
		const LLVector3&	scaleVec(const LLVector3& vec);
		LLVector3			scaledVec(const LLVector3& vec) const;
		BOOL isNull() const;
		BOOL isExactlyZero() const		{ return !mV[VX] && !mV[VY] && !mV[VZ]; }
		F32 operator[](int idx) const { return mV[idx]; }
		F32 &operator[](int idx) { return mV[idx]; }
		friend LLVector3 operator+(const LLVector3 &a, const LLVector3 &b);
		friend LLVector3 operator-(const LLVector3 &a, const LLVector3 &b);
		friend F32 operator*(const LLVector3 &a, const LLVector3 &b);
		friend LLVector3 operator%(const LLVector3 &a, const LLVector3 &b);
		friend LLVector3 operator*(const LLVector3 &a, F32 k);
		friend LLVector3 operator/(const LLVector3 &a, F32 k);
		friend LLVector3 operator*(F32 k, const LLVector3 &a);
		friend bool operator==(const LLVector3 &a, const LLVector3 &b);
		friend bool operator!=(const LLVector3 &a, const LLVector3 &b);
		friend bool operator<(const LLVector3 &a, const LLVector3 &b);
		friend const LLVector3& operator+=(LLVector3 &a, const LLVector3 &b);
		friend const LLVector3& operator-=(LLVector3 &a, const LLVector3 &b);
		friend const LLVector3& operator%=(LLVector3 &a, const LLVector3 &b);
		friend const LLVector3& operator*=(LLVector3 &a, const LLVector3 &b);
		friend const LLVector3& operator*=(LLVector3 &a, F32 k);
		friend const LLVector3& operator/=(LLVector3 &a, F32 k);
		friend const LLVector3& operator*=(LLVector3 &a, const LLQuaternion &b);
		friend LLVector3 operator-(const LLVector3 &a);
		friend std::ostream&	 operator<<(std::ostream& s, const LLVector3 &a);
		static BOOL parseVector3(const std::string& buf, LLVector3* value);
};
static_assert(std::is_trivially_copyable<LLVector3>::value, "LLVector3 must be a trivially copyable type");
typedef LLVector3 LLSimLocalVec;
F32	angle_between(const LLVector3 &a, const LLVector3 &b);
BOOL are_parallel(const LLVector3 &a, const LLVector3 &b, F32 epsilon=F_APPROXIMATELY_ZERO);
F32	dist_vec(const LLVector3 &a, const LLVector3 &b);
F32	dist_vec_squared(const LLVector3 &a, const LLVector3 &b);
F32	dist_vec_squared2D(const LLVector3 &a, const LLVector3 &b);
LLVector3 projected_vec(const LLVector3 &a, const LLVector3 &b);
LLVector3 inverse_projected_vec(const LLVector3 &a, const LLVector3 &b);
LLVector3 parallel_component(const LLVector3 &a, const LLVector3 &b);
LLVector3 orthogonal_component(const LLVector3 &a, const LLVector3 &b);
LLVector3 lerp(const LLVector3 &a, const LLVector3 &b, F32 u);
LLVector3 point_to_box_offset(LLVector3& pos, const LLVector3* box);
bool box_valid_and_non_zero(const LLVector3* box);
inline LLVector3::LLVector3(void)
{
	mV[0] = 0.f;
	mV[1] = 0.f;
	mV[2] = 0.f;
}
inline LLVector3::LLVector3(const F32 x, const F32 y, const F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
}
inline LLVector3::LLVector3(const F32 *vec)
{
	mV[VX] = vec[VX];
	mV[VY] = vec[VY];
	mV[VZ] = vec[VZ];
}
inline BOOL LLVector3::isFinite() const
{
	return (std::isfinite(mV[VX]) && std::isfinite(mV[VY]) && std::isfinite(mV[VZ]));
}
inline void	LLVector3::clear(void)
{
	mV[0] = 0.f;
	mV[1] = 0.f;
	mV[2] = 0.f;
}
inline void	LLVector3::setZero(void)
{
	mV[0] = 0.f;
	mV[1] = 0.f;
	mV[2] = 0.f;
}
inline void	LLVector3::clearVec(void)
{
	mV[0] = 0.f;
	mV[1] = 0.f;
	mV[2] = 0.f;
}
inline void	LLVector3::zeroVec(void)
{
	mV[0] = 0.f;
	mV[1] = 0.f;
	mV[2] = 0.f;
}
inline void	LLVector3::set(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
}
inline void	LLVector3::set(const LLVector3 &vec)
{
	mV[0] = vec.mV[0];
	mV[1] = vec.mV[1];
	mV[2] = vec.mV[2];
}
inline void	LLVector3::set(const F32 *vec)
{
	mV[0] = vec[0];
	mV[1] = vec[1];
	mV[2] = vec[2];
}
inline void	LLVector3::setVec(F32 x, F32 y, F32 z)
{
	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
}
inline void	LLVector3::setVec(const LLVector3 &vec)
{
	mV[0] = vec.mV[0];
	mV[1] = vec.mV[1];
	mV[2] = vec.mV[2];
}
inline void	LLVector3::setVec(const F32 *vec)
{
	mV[0] = vec[0];
	mV[1] = vec[1];
	mV[2] = vec[2];
}
inline F32 LLVector3::normalize(void)
{
	F32 mag = (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
	F32 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[0] *= oomag;
		mV[1] *= oomag;
		mV[2] *= oomag;
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
inline F32 LLVector3::normVec(void)
{
	F32 mag = (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
	F32 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mV[0] *= oomag;
		mV[1] *= oomag;
		mV[2] *= oomag;
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
inline F32	LLVector3::length(void) const
{
	return (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
}
inline F32	LLVector3::lengthSquared(void) const
{
	return mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2];
}
inline F32	LLVector3::magVec(void) const
{
	return (F32) sqrt(mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2]);
}
inline F32	LLVector3::magVecSquared(void) const
{
	return mV[0]*mV[0] + mV[1]*mV[1] + mV[2]*mV[2];
}
inline BOOL LLVector3::inRange( F32 min, F32 max ) const
{
	return mV[0] >= min && mV[0] <= max &&
		   mV[1] >= min && mV[1] <= max &&
		   mV[2] >= min && mV[2] <= max;
}
inline LLVector3 operator+(const LLVector3 &a, const LLVector3 &b)
{
	LLVector3 c(a);
	return c += b;
}
inline LLVector3 operator-(const LLVector3 &a, const LLVector3 &b)
{
	LLVector3 c(a);
	return c -= b;
}
inline F32  operator*(const LLVector3 &a, const LLVector3 &b)
{
	return (a.mV[0]*b.mV[0] + a.mV[1]*b.mV[1] + a.mV[2]*b.mV[2]);
}
inline LLVector3 operator%(const LLVector3 &a, const LLVector3 &b)
{
	return LLVector3( a.mV[1]*b.mV[2] - b.mV[1]*a.mV[2], a.mV[2]*b.mV[0] - b.mV[2]*a.mV[0], a.mV[0]*b.mV[1] - b.mV[0]*a.mV[1] );
}
inline LLVector3 operator/(const LLVector3 &a, F32 k)
{
	F32 t = 1.f / k;
	return LLVector3( a.mV[0] * t, a.mV[1] * t, a.mV[2] * t );
}
inline LLVector3 operator*(const LLVector3 &a, F32 k)
{
	return LLVector3( a.mV[0] * k, a.mV[1] * k, a.mV[2] * k );
}
inline LLVector3 operator*(F32 k, const LLVector3 &a)
{
	return LLVector3( a.mV[0] * k, a.mV[1] * k, a.mV[2] * k );
}
inline bool operator==(const LLVector3 &a, const LLVector3 &b)
{
	return (  (a.mV[0] == b.mV[0])
			&&(a.mV[1] == b.mV[1])
			&&(a.mV[2] == b.mV[2]));
}
inline bool operator!=(const LLVector3 &a, const LLVector3 &b)
{
	return (  (a.mV[0] != b.mV[0])
			||(a.mV[1] != b.mV[1])
			||(a.mV[2] != b.mV[2]));
}
inline bool operator<(const LLVector3 &a, const LLVector3 &b)
{
	return (a.mV[0] < b.mV[0]
			|| (a.mV[0] == b.mV[0]
				&& (a.mV[1] < b.mV[1]
					|| ((a.mV[1] == b.mV[1])
						&& a.mV[2] < b.mV[2]))));
}
inline const LLVector3& operator+=(LLVector3 &a, const LLVector3 &b)
{
	a.mV[0] += b.mV[0];
	a.mV[1] += b.mV[1];
	a.mV[2] += b.mV[2];
	return a;
}
inline const LLVector3& operator-=(LLVector3 &a, const LLVector3 &b)
{
	a.mV[0] -= b.mV[0];
	a.mV[1] -= b.mV[1];
	a.mV[2] -= b.mV[2];
	return a;
}
inline const LLVector3& operator%=(LLVector3 &a, const LLVector3 &b)
{
	LLVector3 ret( a.mV[1]*b.mV[2] - b.mV[1]*a.mV[2], a.mV[2]*b.mV[0] - b.mV[2]*a.mV[0], a.mV[0]*b.mV[1] - b.mV[0]*a.mV[1]);
	a = ret;
	return a;
}
inline const LLVector3& operator*=(LLVector3 &a, F32 k)
{
	a.mV[0] *= k;
	a.mV[1] *= k;
	a.mV[2] *= k;
	return a;
}
inline const LLVector3& operator*=(LLVector3 &a, const LLVector3 &b)
{
	a.mV[0] *= b.mV[0];
	a.mV[1] *= b.mV[1];
	a.mV[2] *= b.mV[2];
	return a;
}
inline const LLVector3& operator/=(LLVector3 &a, F32 k)
{
	F32 t = 1.f / k;
	a.mV[0] *= t;
	a.mV[1] *= t;
	a.mV[2] *= t;
	return a;
}
inline LLVector3 operator-(const LLVector3 &a)
{
	return LLVector3( -a.mV[0], -a.mV[1], -a.mV[2] );
}
inline F32	dist_vec(const LLVector3 &a, const LLVector3 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	F32 z = a.mV[2] - b.mV[2];
	return (F32) sqrt( x*x + y*y + z*z );
}
inline F32	dist_vec_squared(const LLVector3 &a, const LLVector3 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	F32 z = a.mV[2] - b.mV[2];
	return x*x + y*y + z*z;
}
inline F32	dist_vec_squared2D(const LLVector3 &a, const LLVector3 &b)
{
	F32 x = a.mV[0] - b.mV[0];
	F32 y = a.mV[1] - b.mV[1];
	return x*x + y*y;
}
inline LLVector3 projected_vec(const LLVector3 &a, const LLVector3 &b)
{
	F32 bb = b * b;
	if (bb > FP_MAG_THRESHOLD * FP_MAG_THRESHOLD)
	{
		return ((a * b) / bb) * b;
	}
	else
	{
		return b.zero;
	}
}
inline LLVector3 inverse_projected_vec(const LLVector3& a, const LLVector3& b)
{
	LLVector3 normalized_a = a;
	normalized_a.normalize();
	LLVector3 normalized_b = b;
	F32 b_length = normalized_b.normalize();
	F32 dot_product = normalized_a * normalized_b;
	return normalized_a * (b_length / dot_product);
}
inline LLVector3 parallel_component(const LLVector3 &a, const LLVector3 &b)
{
	return projected_vec(a, b);
}
inline LLVector3 orthogonal_component(const LLVector3 &a, const LLVector3 &b)
{
	return a - projected_vec(a, b);
}
inline LLVector3 lerp(const LLVector3 &a, const LLVector3 &b, F32 u)
{
	return LLVector3(
		a.mV[VX] + (b.mV[VX] - a.mV[VX]) * u,
		a.mV[VY] + (b.mV[VY] - a.mV[VY]) * u,
		a.mV[VZ] + (b.mV[VZ] - a.mV[VZ]) * u);
}
inline BOOL	LLVector3::isNull() const
{
	if ( F_APPROXIMATELY_ZERO > mV[VX]*mV[VX] + mV[VY]*mV[VY] + mV[VZ]*mV[VZ] )
	{
		return TRUE;
	}
	return FALSE;
}
inline void update_min_max(LLVector3& min, LLVector3& max, const LLVector3& pos)
{
	for (U32 i = 0; i < 3; i++)
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
inline void update_min_max(LLVector3& min, LLVector3& max, const F32* pos)
{
	for (U32 i = 0; i < 3; i++)
	{
		if (min.mV[i] > pos[i])
		{
			min.mV[i] = pos[i];
		}
		if (max.mV[i] < pos[i])
		{
			max.mV[i] = pos[i];
		}
	}
}
inline F32 angle_between(const LLVector3& a, const LLVector3& b)
{
	F32 ab = a * b;
	if (ab == -0.0f)
	{
		ab = 0.0f;
	}
	LLVector3 c = a % b;
	return atan2f(sqrtf(c * c), ab);
}
inline BOOL are_parallel(const LLVector3 &a, const LLVector3 &b, F32 epsilon)
{
	LLVector3 an = a;
	LLVector3 bn = b;
	an.normalize();
	bn.normalize();
	F32 dot = an * bn;
	if ( (1.0f - fabs(dot)) < epsilon)
	{
		return TRUE;
	}
	return FALSE;
}
inline std::ostream& operator<<(std::ostream& s, const LLVector3 &a)
{
	s << "{ " << a.mV[VX] << ", " << a.mV[VY] << ", " << a.mV[VZ] << " }";
	return s;
}
#endif
