/** 
 * @file llvector4a.h
 * @brief LLVector4a class header file - memory aligned and vectorized 4 component vector
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#ifndef	LL_LLVECTOR4A_H
#define	LL_LLVECTOR4A_H
class LLRotation;
#include <assert.h>
#include "llpreprocessor.h"
#include "llmemory.h"
class LLVector4a;
LL_ALIGN_PREFIX(16)
class LLVector4a
{
public:
	static void initClass()
	{
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
	}
	static inline const LLVector4a& getZero()
	{
		extern const LLVector4a LL_V4A_ZERO;
		return LL_V4A_ZERO;
	}
	static inline const LLVector4a& getEpsilon()
	{
		extern const LLVector4a LL_V4A_EPSILON;
		return LL_V4A_EPSILON;
	}
	static inline void copy4a(F32* dst, const F32* src)
	{
		_mm_store_ps(dst, _mm_load_ps(src));
	}
	static void memcpyNonAliased16(F32* __restrict dst, const F32* __restrict src, size_t bytes);
	LLVector4a()
#if !defined(LL_DEBUG)
		= default;
#else
	{
		ll_assert_aligned(this,16);
	}
#endif
	LLVector4a(F32 x, F32 y, F32 z, F32 w = 0.f)
	{
		set(x, y, z, w);
	}
	LLVector4a(F32 x)
	{
		splat(x);
	}
	LLVector4a(const LLSimdScalar& x)
	{
		splat(x);
	}
	LLVector4a(LLQuad q)
	{
		mQ = q;
	}
	inline void load4a(const F32* src);
	inline void loadua(const F32* src);
	inline void load3(const F32* src, const F32 w=0.f);
	inline void store4a(F32* dst) const;
	inline F32* getF32ptr();
	inline const F32* const getF32ptr() const;
	inline F32 operator[](const S32 idx) const;
	inline LLSimdScalar getScalarAt(const S32 idx) const;
	template <int N> LL_FORCE_INLINE LLSimdScalar getScalarAt() const;
	inline void set(F32 x, F32 y, F32 z, F32 w = 0.f);
	inline void clear();
	inline void splat(const F32 x);
	inline void splat(const LLSimdScalar& x);
	template <int N> void splat(const LLVector4a& src);
	inline void splat(const LLVector4a& v, U32 i);
	template <int N> inline void copyComponent(const LLVector4a& src);
	inline void setSelectWithMask( const LLVector4Logical& mask, const LLVector4a& sourceIfTrue, const LLVector4a& sourceIfFalse );
	inline void setAdd(const LLVector4a& a, const LLVector4a& b);
	inline void setSub(const LLVector4a& a, const LLVector4a& b);
	inline void setMul(const LLVector4a& a, const LLVector4a& b);
	inline void setDiv(const LLVector4a& a, const LLVector4a& b);
	inline void setAbs(const LLVector4a& src);
	inline void add(const LLVector4a& rhs);
	inline void sub(const LLVector4a& rhs);
	inline void mul(const LLVector4a& rhs);
	inline void div(const LLVector4a& rhs);
	inline void mul(const F32 x);
	inline void setCross3(const LLVector4a& a, const LLVector4a& b);
	inline void setAllDot3(const LLVector4a& a, const LLVector4a& b);
	inline void setAllDot4(const LLVector4a& a, const LLVector4a& b);
	inline LLSimdScalar dot3(const LLVector4a& b) const;
	inline LLSimdScalar dot4(const LLVector4a& b) const;
	inline void normalize3();
	inline void normalize4();
	inline LLSimdScalar normalize3withLength();
	inline void normalize3fast();
	inline void normalize3fast_checked(LLVector4a* d = 0);
	inline LLBool32 isNormalized3( F32 tolerance = 1e-3 ) const;
	inline LLBool32 isNormalized4( F32 tolerance = 1e-3 ) const;
	inline void setAllLength3( const LLVector4a& v );
	inline LLSimdScalar getLength3() const;
	inline void setMin(const LLVector4a& lhs, const LLVector4a& rhs);
	inline void setMax(const LLVector4a& lhs, const LLVector4a& rhs);
	inline void clamp( const LLVector4a& low, const LLVector4a& high );
	inline void setLerp(const LLVector4a& lhs, const LLVector4a& rhs, F32 c);
	inline LLBool32 isFinite3() const;
	inline LLBool32 isFinite4() const;
	void setRotated( const LLRotation& rot, const LLVector4a& vec );
	void setRotated( const class LLQuaternion2& quat, const LLVector4a& vec );
	inline void setRotatedInv( const LLRotation& rot, const LLVector4a& vec );
	inline void setRotatedInv( const class LLQuaternion2& quat, const LLVector4a& vec );
	void quantize8( const LLVector4a& low, const LLVector4a& high );
	void quantize16( const LLVector4a& low, const LLVector4a& high );
	void negate();
	inline LLVector4Logical greaterThan(const LLVector4a& rhs) const;
	inline LLVector4Logical lessThan(const LLVector4a& rhs) const;
	inline LLVector4Logical greaterEqual(const LLVector4a& rhs) const;
	inline LLVector4Logical lessEqual(const LLVector4a& rhs) const;
	inline LLVector4Logical equal(const LLVector4a& rhs) const;
	inline bool equals4(const LLVector4a& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;
	inline bool equals3(const LLVector4a& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;
	inline const LLVector4a& operator= (const LLQuad& rhs);
	inline operator LLQuad() const;
private:
	LLQuad mQ;
} LL_ALIGN_POSTFIX(16);
inline void update_min_max(LLVector4a& min, LLVector4a& max, const LLVector4a& p)
{
	min.setMin(min, p);
	max.setMax(max, p);
}
inline std::ostream& operator<<(std::ostream& s, const LLVector4a& v)
{
	s << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
	return s;
}
#if !defined(LL_DEBUG)
static_assert(std::is_trivial<LLVector4a>::value, "LLVector4a must be a be a trivial type");
static_assert(std::is_standard_layout<LLVector4a>::value, "LLVector4a must be a standard layout type");
#endif
#endif
