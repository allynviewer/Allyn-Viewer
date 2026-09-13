/** 
 * @file v3dmath.h
 * @brief High precision 3 dimensional vector.
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
#ifndef LL_V3DMATH_H
#define LL_V3DMATH_H
#include "llerror.h"
#include "v3math.h"
class LLVector3d
{
	public:
		F64 mdV[3];
		const static LLVector3d zero;
		const static LLVector3d x_axis;
		const static LLVector3d y_axis;
		const static LLVector3d z_axis;
		const static LLVector3d x_axis_neg;
		const static LLVector3d y_axis_neg;
		const static LLVector3d z_axis_neg;
		inline LLVector3d();
		inline LLVector3d(const F64 x, const F64 y, const F64 z);
		inline explicit LLVector3d(const F64 *vec);
		inline explicit LLVector3d(const LLVector3 &vec);
		LLVector3d(const LLSD& sd)
		{
			setValue(sd);
		}
		void setValue(const LLSD& sd)
		{
			mdV[0] = sd[0].asReal();
			mdV[1] = sd[1].asReal();
			mdV[2] = sd[2].asReal();
		}
		LLSD getValue() const
		{
			LLSD ret;
			ret[0] = mdV[0];
			ret[1] = mdV[1];
			ret[2] = mdV[2];
			return ret;
		}
		inline BOOL isFinite() const;
		BOOL		clamp(const F64 min, const F64 max);
		BOOL		abs();
		inline const LLVector3d&	clear();
		inline const LLVector3d&	clearVec();
		inline const LLVector3d&	setZero();
		inline const LLVector3d&	zeroVec();
		inline const LLVector3d&	set(const F64 x, const F64 y, const F64 z);
		inline const LLVector3d&	set(const LLVector3d &vec);
		inline const LLVector3d&	set(const F64 *vec);
		inline const LLVector3d&	set(const LLVector3 &vec);
		inline const LLVector3d&	setVec(const F64 x, const F64 y, const F64 z);
		inline const LLVector3d&	setVec(const LLVector3d &vec);
		inline const LLVector3d&	setVec(const F64 *vec);
		inline const LLVector3d&	setVec(const LLVector3 &vec);
		F64		magVec() const;
		F64		magVecSquared() const;
		inline F64		normVec();
		F64 length() const;
		F64 lengthSquared() const;
		inline F64 normalize();
		const LLVector3d&	rotVec(const F64 angle, const LLVector3d &vec);
		const LLVector3d&	rotVec(const F64 angle, const F64 x, const F64 y, const F64 z);
		const LLVector3d&	rotVec(const LLMatrix3 &mat);
		const LLVector3d&	rotVec(const LLQuaternion &q);
		BOOL isNull() const;
		BOOL isExactlyZero() const		{ return !mdV[VX] && !mdV[VY] && !mdV[VZ]; }
		const LLVector3d&	operator=(const LLVector4 &a);
		F64 operator[](int idx) const { return mdV[idx]; }
		F64 &operator[](int idx) { return mdV[idx]; }
		friend LLVector3d operator+(const LLVector3d &a, const LLVector3d &b);
		friend LLVector3d operator-(const LLVector3d &a, const LLVector3d &b);
		friend F64 operator*(const LLVector3d &a, const LLVector3d &b);
		friend LLVector3d operator%(const LLVector3d &a, const LLVector3d &b);
		friend LLVector3d operator*(const LLVector3d &a, const F64 k);
		friend LLVector3d operator/(const LLVector3d &a, const F64 k);
		friend LLVector3d operator*(const F64 k, const LLVector3d &a);
		friend bool operator==(const LLVector3d &a, const LLVector3d &b);
		friend bool operator!=(const LLVector3d &a, const LLVector3d &b);
		friend const LLVector3d& operator+=(LLVector3d &a, const LLVector3d &b);
		friend const LLVector3d& operator-=(LLVector3d &a, const LLVector3d &b);
		friend const LLVector3d& operator%=(LLVector3d &a, const LLVector3d &b);
		friend const LLVector3d& operator*=(LLVector3d &a, const F64 k);
		friend const LLVector3d& operator/=(LLVector3d &a, const F64 k);
		friend LLVector3d operator-(const LLVector3d &a);
		friend std::ostream&	 operator<<(std::ostream& s, const LLVector3d &a);
		static BOOL parseVector3d(const std::string& buf, LLVector3d* value);
};
static_assert(std::is_trivially_copyable<LLVector3d>::value, "LLVector3d must be a trivially copyable type");
typedef LLVector3d LLGlobalVec;
inline const LLVector3d &LLVector3d::set(const LLVector3 &vec)
{
	mdV[0] = vec.mV[0];
	mdV[1] = vec.mV[1];
	mdV[2] = vec.mV[2];
	return *this;
}
inline const LLVector3d &LLVector3d::setVec(const LLVector3 &vec)
{
	mdV[0] = vec.mV[0];
	mdV[1] = vec.mV[1];
	mdV[2] = vec.mV[2];
	return *this;
}
inline LLVector3d::LLVector3d(void)
{
	mdV[0] = 0.f;
	mdV[1] = 0.f;
	mdV[2] = 0.f;
}
inline LLVector3d::LLVector3d(const F64 x, const F64 y, const F64 z)
{
	mdV[VX] = x;
	mdV[VY] = y;
	mdV[VZ] = z;
}
inline LLVector3d::LLVector3d(const F64 *vec)
{
	mdV[VX] = vec[VX];
	mdV[VY] = vec[VY];
	mdV[VZ] = vec[VZ];
}
inline LLVector3d::LLVector3d(const LLVector3 &vec)
{
	mdV[VX] = vec.mV[VX];
	mdV[VY] = vec.mV[VY];
	mdV[VZ] = vec.mV[VZ];
}
inline BOOL LLVector3d::isFinite() const
{
	return (std::isfinite(mdV[VX]) && std::isfinite(mdV[VY]) && std::isfinite(mdV[VZ]));
}
inline const LLVector3d&	LLVector3d::clear(void)
{
	mdV[0] = 0.f;
	mdV[1] = 0.f;
	mdV[2]= 0.f;
	return (*this);
}
inline const LLVector3d&	LLVector3d::clearVec(void)
{
	mdV[0] = 0.f;
	mdV[1] = 0.f;
	mdV[2]= 0.f;
	return (*this);
}
inline const LLVector3d&	LLVector3d::setZero(void)
{
	mdV[0] = 0.f;
	mdV[1] = 0.f;
	mdV[2] = 0.f;
	return (*this);
}
inline const LLVector3d&	LLVector3d::zeroVec(void)
{
	mdV[0] = 0.f;
	mdV[1] = 0.f;
	mdV[2] = 0.f;
	return (*this);
}
inline const LLVector3d&	LLVector3d::set(const F64 x, const F64 y, const F64 z)
{
	mdV[VX] = x;
	mdV[VY] = y;
	mdV[VZ] = z;
	return (*this);
}
inline const LLVector3d&	LLVector3d::set(const LLVector3d &vec)
{
	mdV[0] = vec.mdV[0];
	mdV[1] = vec.mdV[1];
	mdV[2] = vec.mdV[2];
	return (*this);
}
inline const LLVector3d&	LLVector3d::set(const F64 *vec)
{
	mdV[0] = vec[0];
	mdV[1] = vec[1];
	mdV[2] = vec[2];
	return (*this);
}
inline const LLVector3d&	LLVector3d::setVec(const F64 x, const F64 y, const F64 z)
{
	mdV[VX] = x;
	mdV[VY] = y;
	mdV[VZ] = z;
	return (*this);
}
inline const LLVector3d&	LLVector3d::setVec(const LLVector3d &vec)
{
	mdV[0] = vec.mdV[0];
	mdV[1] = vec.mdV[1];
	mdV[2] = vec.mdV[2];
	return (*this);
}
inline const LLVector3d&	LLVector3d::setVec(const F64 *vec)
{
	mdV[0] = vec[0];
	mdV[1] = vec[1];
	mdV[2] = vec[2];
	return (*this);
}
inline F64 LLVector3d::normVec(void)
{
	F64 mag = (F32) sqrt(mdV[0]*mdV[0] + mdV[1]*mdV[1] + mdV[2]*mdV[2]);
	F64 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mdV[0] *= oomag;
		mdV[1] *= oomag;
		mdV[2] *= oomag;
	}
	else
	{
		mdV[0] = 0.f;
		mdV[1] = 0.f;
		mdV[2] = 0.f;
		mag = 0;
	}
	return (mag);
}
inline F64 LLVector3d::normalize(void)
{
	F64 mag = (F32) sqrt(mdV[0]*mdV[0] + mdV[1]*mdV[1] + mdV[2]*mdV[2]);
	F64 oomag;
	if (mag > FP_MAG_THRESHOLD)
	{
		oomag = 1.f/mag;
		mdV[0] *= oomag;
		mdV[1] *= oomag;
		mdV[2] *= oomag;
	}
	else
	{
		mdV[0] = 0.f;
		mdV[1] = 0.f;
		mdV[2] = 0.f;
		mag = 0;
	}
	return (mag);
}
inline F64	LLVector3d::magVec(void) const
{
	return (F32) sqrt(mdV[0]*mdV[0] + mdV[1]*mdV[1] + mdV[2]*mdV[2]);
}
inline F64	LLVector3d::magVecSquared(void) const
{
	return mdV[0]*mdV[0] + mdV[1]*mdV[1] + mdV[2]*mdV[2];
}
inline F64	LLVector3d::length(void) const
{
	return (F32) sqrt(mdV[0]*mdV[0] + mdV[1]*mdV[1] + mdV[2]*mdV[2]);
}
inline F64	LLVector3d::lengthSquared(void) const
{
	return mdV[0]*mdV[0] + mdV[1]*mdV[1] + mdV[2]*mdV[2];
}
inline LLVector3d operator+(const LLVector3d &a, const LLVector3d &b)
{
	LLVector3d c(a);
	return c += b;
}
inline LLVector3d operator-(const LLVector3d &a, const LLVector3d &b)
{
	LLVector3d c(a);
	return c -= b;
}
inline F64  operator*(const LLVector3d &a, const LLVector3d &b)
{
	return (a.mdV[0]*b.mdV[0] + a.mdV[1]*b.mdV[1] + a.mdV[2]*b.mdV[2]);
}
inline LLVector3d operator%(const LLVector3d &a, const LLVector3d &b)
{
	return LLVector3d( a.mdV[1]*b.mdV[2] - b.mdV[1]*a.mdV[2], a.mdV[2]*b.mdV[0] - b.mdV[2]*a.mdV[0], a.mdV[0]*b.mdV[1] - b.mdV[0]*a.mdV[1] );
}
inline LLVector3d operator/(const LLVector3d &a, const F64 k)
{
	F64 t = 1.f / k;
	return LLVector3d( a.mdV[0] * t, a.mdV[1] * t, a.mdV[2] * t );
}
inline LLVector3d operator*(const LLVector3d &a, const F64 k)
{
	return LLVector3d( a.mdV[0] * k, a.mdV[1] * k, a.mdV[2] * k );
}
inline LLVector3d operator*(F64 k, const LLVector3d &a)
{
	return LLVector3d( a.mdV[0] * k, a.mdV[1] * k, a.mdV[2] * k );
}
inline bool operator==(const LLVector3d &a, const LLVector3d &b)
{
	return (  (a.mdV[0] == b.mdV[0])
			&&(a.mdV[1] == b.mdV[1])
			&&(a.mdV[2] == b.mdV[2]));
}
inline bool operator!=(const LLVector3d &a, const LLVector3d &b)
{
	return (  (a.mdV[0] != b.mdV[0])
			||(a.mdV[1] != b.mdV[1])
			||(a.mdV[2] != b.mdV[2]));
}
inline const LLVector3d& operator+=(LLVector3d &a, const LLVector3d &b)
{
	a.mdV[0] += b.mdV[0];
	a.mdV[1] += b.mdV[1];
	a.mdV[2] += b.mdV[2];
	return a;
}
inline const LLVector3d& operator-=(LLVector3d &a, const LLVector3d &b)
{
	a.mdV[0] -= b.mdV[0];
	a.mdV[1] -= b.mdV[1];
	a.mdV[2] -= b.mdV[2];
	return a;
}
inline const LLVector3d& operator%=(LLVector3d &a, const LLVector3d &b)
{
	LLVector3d ret( a.mdV[1]*b.mdV[2] - b.mdV[1]*a.mdV[2], a.mdV[2]*b.mdV[0] - b.mdV[2]*a.mdV[0], a.mdV[0]*b.mdV[1] - b.mdV[0]*a.mdV[1]);
	a = ret;
	return a;
}
inline const LLVector3d& operator*=(LLVector3d &a, const F64 k)
{
	a.mdV[0] *= k;
	a.mdV[1] *= k;
	a.mdV[2] *= k;
	return a;
}
inline const LLVector3d& operator/=(LLVector3d &a, const F64 k)
{
	F64 t = 1.f / k;
	a.mdV[0] *= t;
	a.mdV[1] *= t;
	a.mdV[2] *= t;
	return a;
}
inline LLVector3d operator-(const LLVector3d &a)
{
	return LLVector3d( -a.mdV[0], -a.mdV[1], -a.mdV[2] );
}
inline F64	dist_vec(const LLVector3d &a, const LLVector3d &b)
{
	F64 x = a.mdV[0] - b.mdV[0];
	F64 y = a.mdV[1] - b.mdV[1];
	F64 z = a.mdV[2] - b.mdV[2];
	return (F32) sqrt( x*x + y*y + z*z );
}
inline F64	dist_vec_squared(const LLVector3d &a, const LLVector3d &b)
{
	F64 x = a.mdV[0] - b.mdV[0];
	F64 y = a.mdV[1] - b.mdV[1];
	F64 z = a.mdV[2] - b.mdV[2];
	return x*x + y*y + z*z;
}
inline F64	dist_vec_squared2D(const LLVector3d &a, const LLVector3d &b)
{
	F64 x = a.mdV[0] - b.mdV[0];
	F64 y = a.mdV[1] - b.mdV[1];
	return x*x + y*y;
}
inline LLVector3d lerp(const LLVector3d &a, const LLVector3d &b, const F64 u)
{
	return LLVector3d(
		a.mdV[VX] + (b.mdV[VX] - a.mdV[VX]) * u,
		a.mdV[VY] + (b.mdV[VY] - a.mdV[VY]) * u,
		a.mdV[VZ] + (b.mdV[VZ] - a.mdV[VZ]) * u);
}
inline BOOL	LLVector3d::isNull() const
{
	if ( F_APPROXIMATELY_ZERO > mdV[VX]*mdV[VX] + mdV[VY]*mdV[VY] + mdV[VZ]*mdV[VZ] )
	{
		return TRUE;
	}
	return FALSE;
}
inline F64 angle_between(const LLVector3d& a, const LLVector3d& b)
{
	LLVector3d an = a;
	LLVector3d bn = b;
	an.normalize();
	bn.normalize();
	F64 cosine = an * bn;
	F64 angle = (cosine >= 1.0f) ? 0.0f :
				(cosine <= -1.0f) ? F_PI :
				acos(cosine);
	return angle;
}
inline BOOL are_parallel(const LLVector3d &a, const LLVector3d &b, const F64 epsilon)
{
	LLVector3d an = a;
	LLVector3d bn = b;
	an.normalize();
	bn.normalize();
	F64 dot = an * bn;
	if ( (1.0f - fabs(dot)) < epsilon)
	{
		return TRUE;
	}
	return FALSE;
}
inline LLVector3d projected_vec(const LLVector3d &a, const LLVector3d &b)
{
	LLVector3d project_axis = b;
	project_axis.normalize();
	return project_axis * (a * project_axis);
}
inline LLVector3d inverse_projected_vec(const LLVector3d& a, const LLVector3d& b)
{
	LLVector3d normalized_a = a;
	normalized_a.normalize();
	LLVector3d normalized_b = b;
	F64 b_length = normalized_b.normalize();
	F64 dot_product = normalized_a * normalized_b;
	return normalized_a * (b_length / dot_product);
}
#endif
