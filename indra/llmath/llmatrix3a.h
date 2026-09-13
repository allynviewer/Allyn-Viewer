/** 
 * @file llmatrix3a.h
 * @brief LLMatrix3a class header file - memory aligned and vectorized 3x3 matrix
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
#ifndef	LL_LLMATRIX3A_H
#define	LL_LLMATRIX3A_H
LL_ALIGN_PREFIX(16)
class LLMatrix3a
{
public:
	static void batchTransform( const LLMatrix3a& xform, const LLVector4a* src, int numVectors, LLVector4a* dst );
	static inline const LLMatrix3a& getIdentity();
	LLMatrix3a() = default;
	inline LLMatrix3a( const LLVector4a& c0, const LLVector4a& c1, const LLVector4a& c2 );
	inline void loadu(const LLMatrix3& src);
	inline void setRows(const LLVector4a& r0, const LLVector4a& r1, const LLVector4a& r2);
	inline void setColumns(const LLVector4a& c0, const LLVector4a& c1, const LLVector4a& c2);
	inline const LLVector4a& getColumn(const U32 column) const;
	void setMul( const LLMatrix3a& lhs, const LLMatrix3a& rhs );
	inline void setTranspose(const LLMatrix3a& src);
	inline void setLerp(const LLMatrix3a& a, const LLMatrix3a& b, F32 w);
	inline void getDeterminant( LLVector4a& dest ) const;
	inline LLSimdScalar getDeterminant() const;
	inline LLBool32 isFinite() const;
	inline bool isApproximatelyEqual( const LLMatrix3a& rhs, F32 tolerance = F_APPROXIMATELY_ZERO ) const;
protected:
	LL_ALIGN_16(LLVector4a mColumns[3]);
} LL_ALIGN_POSTFIX(16);
LL_ALIGN_PREFIX(16)
class LLRotation : public LLMatrix3a
{
public:
	LLRotation() = default;
	inline bool isOkRotation() const;
} LL_ALIGN_POSTFIX(16);
#if !defined(LL_DEBUG)
static_assert(std::is_trivial<LLMatrix3a>::value, "LLMatrix3a must be a trivial type");
static_assert(std::is_standard_layout<LLMatrix3a>::value, "LLMatrix3a must be a standard layout type");
static_assert(std::is_trivial<LLRotation>::value, "LLRotation must be a trivial type");
static_assert(std::is_standard_layout<LLRotation>::value, "LLRotation must be a standard layout type");
#endif
#endif
