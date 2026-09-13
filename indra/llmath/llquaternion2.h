/** 
 * @file llquaternion2.h
 * @brief LLQuaternion2 class header file - SIMD-enabled quaternion class
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
#ifndef	LL_QUATERNION2_H
#define	LL_QUATERNION2_H
#include "llquaternion.h"
LL_ALIGN_PREFIX(16)
class LLQuaternion2
{
public:
	LLQuaternion2() = default;
	explicit LLQuaternion2( const class LLQuaternion& quat );
	inline void operator=( const LLQuaternion& quat )
	{
		mQ.loadua( quat.mQ );
	}
	inline const LLVector4a& getVector4a() const;
	inline LLVector4a& getVector4aRw();
	inline void setConjugate(const LLQuaternion2& src);
	inline void normalize();
	inline void quantize8();
	inline void quantize16();
	inline void mul(const LLQuaternion2& b);
	inline bool equals(const LLQuaternion2& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;
	inline bool isOkRotation() const;
protected:
	LL_ALIGN_16(LLVector4a mQ);
} LL_ALIGN_POSTFIX(16);
#if !defined(LL_DEBUG)
static_assert(std::is_trivial<LLQuaternion2>::value, "LLQuaternion2 must be a trivial type");
static_assert(std::is_standard_layout<LLQuaternion2>::value, "LLQuaternion2 must be a standard layout type");
#endif
#endif
