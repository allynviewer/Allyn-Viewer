/** 
 * @file llmath.h
 * @brief Useful math constants and macros.
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
#ifndef LLMATH_H
#define LLMATH_H
#include "llpreprocessor.h"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <limits>
#include "lldefs.h"
#include "is_approx_equal_fraction.h"
constexpr F32	GRAVITY			= -9.8f;
constexpr F32	F_PI		= 3.1415926535897932384626433832795f;
constexpr F32	F_TWO_PI	= 6.283185307179586476925286766559f;
constexpr F32	F_PI_BY_TWO	= 1.5707963267948966192313216916398f;
constexpr F32	F_SQRT_TWO_PI = 2.506628274631000502415765284811f;
constexpr F32	F_E			= 2.71828182845904523536f;
constexpr F32	F_SQRT2		= 1.4142135623730950488016887242097f;
constexpr F32	F_SQRT3		= 1.73205080756888288657986402541f;
constexpr F32	OO_SQRT2	= 0.7071067811865475244008443621049f;
constexpr F32	OO_SQRT3	= 0.577350269189625764509f;
constexpr F32	DEG_TO_RAD	= 0.017453292519943295769236907684886f;
constexpr F32	RAD_TO_DEG	= 57.295779513082320876798154814105f;
constexpr F32	F_APPROXIMATELY_ZERO = 0.00001f;
constexpr F32	F_LN10		= 2.3025850929940456840179914546844f;
constexpr F32	OO_LN10		= 0.43429448190325182765112891891661f;
constexpr F32	F_LN2		= 0.69314718056f;
constexpr F32	OO_LN2		= 1.4426950408889634073599246810019f;
constexpr F32	F_ALMOST_ZERO	= 0.0001f;
constexpr F32	F_ALMOST_ONE	= 1.0f - F_ALMOST_ZERO;
constexpr F32	GIMBAL_THRESHOLD = 0.000436f;
constexpr F32 FP_MAG_THRESHOLD = 0.0000001f;
inline bool is_approx_zero( F32 f ) { return (-F_APPROXIMATELY_ZERO < f) && (f < F_APPROXIMATELY_ZERO); }
inline bool is_zero(F32 x)
{
	return (*(U32*)(&x) & 0x7fffffff) == 0;
}
inline bool is_approx_equal(F32 x, F32 y)
{
	constexpr S32 COMPARE_MANTISSA_UP_TO_BIT = 0x02;
	return (std::abs((S32) ((U32&)x - (U32&)y) ) < COMPARE_MANTISSA_UP_TO_BIT);
}
inline bool is_approx_equal(F64 x, F64 y)
{
	constexpr S64 COMPARE_MANTISSA_UP_TO_BIT = 0x02;
	return (std::abs((S32) ((U64&)x - (U64&)y) ) < COMPARE_MANTISSA_UP_TO_BIT);
}
inline S32 llabs(const S32 a)
{
	return S32(std::labs(a));
}
inline F32 llabs(const F32 a)
{
	return F32(std::fabs(a));
}
inline F64 llabs(const F64 a)
{
	return F64(std::fabs(a));
}
inline S32 lltrunc( F32 f )
{
	return (S32)trunc(f);
}
inline S32 lltrunc( F64 f )
{
	return (S32)trunc(f);
}
inline S32 llfloor( F32 f )
{
#if LL_WINDOWS && !defined( __INTEL_COMPILER ) && !defined(_WIN64)
		const U32 zpfp = 0xBEFFFFFF;
		S32 result;
		__asm {
			fld		f
			fadd	dword ptr [zpfp]
			fistp	result
		}
		return result;
#else
		return (S32)floor(f);
#endif
}
inline S32 llceil( F32 f )
{
	return (S32)ceil(f);
}
inline S32 ll_round(const F32 val)
{
	return (S32)round(val);
}
inline S32 ll_pos_round(const F32 val)
{
	return val + .5f;
}
inline F32 ll_round(F32 val, F32 nearest)
{
	return F32(round(val * (1.0f / nearest))) * nearest;
}
inline F64 ll_round(F64 val, F64 nearest)
{
	return F64(round(val * (1.0 / nearest))) * nearest;
}
constexpr F32 FAST_MAG_ALPHA = 0.960433870103f;
constexpr F32 FAST_MAG_BETA = 0.397824734759f;
inline F32 fastMagnitude(F32 a, F32 b)
{
	a = (a > 0) ? a : -a;
	b = (b > 0) ? b : -b;
	return(FAST_MAG_ALPHA * llmax(a,b) + FAST_MAG_BETA * llmin(a,b));
}
constexpr F64 LL_DOUBLE_TO_FIX_MAGIC	= 68719476736.0*1.5;
constexpr S32 LL_SHIFT_AMOUNT			= 16;
#ifdef LL_LITTLE_ENDIAN
	#define LL_EXP_INDEX				1
	#define LL_MAN_INDEX				0
#else
	#define LL_EXP_INDEX				0
	#define LL_MAN_INDEX				1
#endif
static union
{
	double d;
	struct
	{
#ifdef LL_LITTLE_ENDIAN
		S32 j, i;
#else
		S32 i, j;
#endif
	} n;
} LLECO;
#define LL_EXP_A (1048576 * OO_LN2)
#define LL_EXP_C (60801)
#define LL_FAST_EXP(y) (LLECO.n.i = ll_round(F32(LL_EXP_A*(y))) + (1072693248 - LL_EXP_C), LLECO.d)
inline F32 llfastpow(const F32 x, const F32 y)
{
	return (F32)(LL_FAST_EXP(y * log(x)));
}
inline F32 snap_to_sig_figs(F32 foo, S32 sig_figs)
{
	F32 bar = 1.f;
	for (S32 i = 0; i < sig_figs; i++)
	{
		bar *= 10.f;
	}
	F32 sign = (foo > 0.f) ? 1.f : -1.f;
	F32 new_foo = F32( S64(foo * bar + sign * 0.5f));
	new_foo /= bar;
	return new_foo;
}
inline F32 lerp(F32 a, F32 b, F32 u)
{
	return a + ((b - a) * u);
}
inline F32 lerp2d(F32 x00, F32 x01, F32 x10, F32 x11, F32 u, F32 v)
{
	F32 a = x00 + (x01-x00)*u;
	F32 b = x10 + (x11-x10)*u;
	F32 r = a + (b-a)*v;
	return r;
}
inline F32 ramp(F32 x, F32 a, F32 b)
{
	return (a == b) ? 0.0f : ((a - x) / (a - b));
}
inline F32 rescale(F32 x, F32 x1, F32 x2, F32 y1, F32 y2)
{
	return lerp(y1, y2, ramp(x, x1, x2));
}
inline F32 clamp_rescale(F32 x, F32 x1, F32 x2, F32 y1, F32 y2)
{
	if (y1 < y2)
	{
		return llclamp(rescale(x,x1,x2,y1,y2),y1,y2);
	}
	else
	{
		return llclamp(rescale(x,x1,x2,y1,y2),y2,y1);
	}
}
inline F32 cubic_step( F32 x, F32 x0, F32 x1, F32 s0, F32 s1 )
{
	if (x <= x0)
		return s0;
	if (x >= x1)
		return s1;
	F32 f = (x - x0) / (x1 - x0);
	return	s0 + (s1 - s0) * (f * f) * (3.0f - 2.0f * f);
}
inline F32 cubic_step( F32 x )
{
	x = llclampf(x);
	return	(x * x) * (3.0f - 2.0f * x);
}
inline F32 quadratic_step( F32 x, F32 x0, F32 x1, F32 s0, F32 s1 )
{
	if (x <= x0)
		return s0;
	if (x >= x1)
		return s1;
	F32 f = (x - x0) / (x1 - x0);
	F32 f_squared = f * f;
	return	(s0 * (1.f - f_squared)) + ((s1 - s0) * f_squared);
}
inline F32 llsimple_angle(F32 angle)
{
	while(angle <= -F_PI)
		angle += F_TWO_PI;
	while(angle >  F_PI)
		angle -= F_TWO_PI;
	return angle;
}
inline U32 get_lower_power_two(U32 val, U32 max_power_two)
{
	if(!max_power_two)
	{
		max_power_two = 1U << 31 ;
	}
	if(max_power_two & (max_power_two - 1))
	{
		return 0 ;
	}
	for(; val < max_power_two ; max_power_two >>= 1) ;
	return max_power_two ;
}
inline U32 get_next_power_two(U32 val, U32 max_power_two)
{
	if(!max_power_two)
	{
		max_power_two = 1U << 31 ;
	}
	if(val >= max_power_two)
	{
		return max_power_two;
	}
	val--;
	val = (val >> 1) | val;
	val = (val >> 2) | val;
	val = (val >> 4) | val;
	val = (val >> 8) | val;
	val = (val >> 16) | val;
	val++;
	return val;
}
inline F32 llgaussian(F32 x, F32 o)
{
	return 1.f/(F_SQRT_TWO_PI*o)*powf(F_E, -(x*x)/(2*o*o));
}
template <class VEC_TYPE>
inline void ll_remove_outliers(std::vector<VEC_TYPE>& data, F32 k)
{
	if (data.size() < 100)
	{
		return;
	}
	VEC_TYPE Q1 = data[data.size()/4];
	VEC_TYPE Q3 = data[data.size()-data.size()/4-1];
	if ((F32)(Q3-Q1) < 1.f)
	{
		return;
	}
	VEC_TYPE min = (VEC_TYPE) ((F32) Q1-k * (F32) (Q3-Q1));
	VEC_TYPE max = (VEC_TYPE) ((F32) Q3+k * (F32) (Q3-Q1));
	U32 i = 0;
	while (i < data.size() && data[i] < min)
	{
		i++;
	}
	S32 j = (S32)data.size()-1;
	while (j > 0 && data[j] > max)
	{
		j--;
	}
	if (j < (S32)data.size()-1)
	{
		data.erase(data.begin()+j, data.end());
	}
	if (i > 0)
	{
		data.erase(data.begin(), data.begin()+i);
	}
}
#include "llsimdmath.h"
#endif
