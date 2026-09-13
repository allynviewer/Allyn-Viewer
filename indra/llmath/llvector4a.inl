/** 
 * @file llvector4a.inl
 * @brief LLVector4a inline function implementations
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
inline void LLVector4a::load4a(const F32* src)
{
	mQ = _mm_load_ps(src);
}
inline void LLVector4a::loadua(const F32* src)
{
	mQ = _mm_loadu_ps(src);
}
inline void LLVector4a::load3(const F32* src, const F32 w)
{
	mQ = _mm_set_ps(w, src[2], src[1], src[0]);
}
inline void LLVector4a::store4a(F32* dst) const
{
	_mm_store_ps(dst, mQ);
}
F32* LLVector4a::getF32ptr()
{
	return (F32*) &mQ;
}
const F32* const LLVector4a::getF32ptr() const
{
	return (const F32* const) &mQ;
}
inline F32 LLVector4a::operator[](const S32 idx) const
{
	return ((F32*)&mQ)[idx];
}
inline LLSimdScalar LLVector4a::getScalarAt(const S32 idx) const
{
	switch (idx)
	{
		case 0:
			return mQ;
		case 1:
			return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(1, 1, 1, 1));
		case 2:
			return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(2, 2, 2, 2));
		case 3:
		default:
			return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(3, 3, 3, 3));
	}
}
template <int N> LL_FORCE_INLINE LLSimdScalar LLVector4a::getScalarAt() const
{
	return _mm_shuffle_ps(mQ, mQ, _MM_SHUFFLE(N, N, N, N));
}
template<> LL_FORCE_INLINE LLSimdScalar LLVector4a::getScalarAt<0>() const
{
	return mQ;
}
inline void LLVector4a::set(F32 x, F32 y, F32 z, F32 w)
{
	mQ = _mm_set_ps(w, z, y, x);
}
inline void LLVector4a::clear()
{
	mQ = LLVector4a::getZero().mQ;
}
inline void LLVector4a::splat(const F32 x)
{
	mQ = _mm_set1_ps(x);
}
inline void LLVector4a::splat(const LLSimdScalar& x)
{
	mQ = _mm_shuffle_ps( x.getQuad(), x.getQuad(), _MM_SHUFFLE(0,0,0,0) );
}
template <int N> void LLVector4a::splat(const LLVector4a& src)
{
	mQ = _mm_shuffle_ps(src.mQ, src.mQ, _MM_SHUFFLE(N, N, N, N) );
}
inline void LLVector4a::splat(const LLVector4a& v, U32 i)
{
	switch (i)
	{
		case 0:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(0, 0, 0, 0));
			break;
		case 1:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(1, 1, 1, 1));
			break;
		case 2:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(2, 2, 2, 2));
			break;
		case 3:
			mQ = _mm_shuffle_ps(v.mQ, v.mQ, _MM_SHUFFLE(3, 3, 3, 3));
			break;
	}
}
template <int N> inline void LLVector4a::copyComponent(const LLVector4a& src)
{
	static const LLVector4Logical mask = _mm_load_ps((F32*)&S_V4LOGICAL_MASK_TABLE[N*4]);
	setSelectWithMask(mask,src,mQ);
}
inline void LLVector4a::setSelectWithMask( const LLVector4Logical& mask, const LLVector4a& sourceIfTrue, const LLVector4a& sourceIfFalse )
{
	mQ = _mm_xor_ps( sourceIfFalse, _mm_and_ps( mask, _mm_xor_ps( sourceIfTrue, sourceIfFalse ) ) );
}
inline void LLVector4a::setAdd(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_add_ps(a.mQ, b.mQ);
}
inline void LLVector4a::setSub(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_sub_ps(a.mQ, b.mQ);
}
inline void LLVector4a::setMul(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_mul_ps(a.mQ, b.mQ);
}
inline void LLVector4a::setDiv(const LLVector4a& a, const LLVector4a& b)
{
	mQ = _mm_div_ps( a.mQ, b.mQ );
}
inline void LLVector4a::setAbs(const LLVector4a& src)
{
	static const LL_ALIGN_16(U32 F_ABS_MASK_4A[4]) = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };
	mQ = _mm_and_ps(src.mQ, *reinterpret_cast<const LLQuad*>(F_ABS_MASK_4A));
}
inline void LLVector4a::add(const LLVector4a& rhs)
{
	mQ = _mm_add_ps(mQ, rhs.mQ);
}
inline void LLVector4a::sub(const LLVector4a& rhs)
{
	mQ = _mm_sub_ps(mQ, rhs.mQ);
}
inline void LLVector4a::mul(const LLVector4a& rhs)
{
	mQ = _mm_mul_ps(mQ, rhs.mQ);
}
inline void LLVector4a::div(const LLVector4a& rhs)
{
	mQ = _mm_div_ps(mQ, rhs.mQ);
}
inline void LLVector4a::mul(const F32 x)
{
	LLVector4a t;
	t.splat(x);
	mQ = _mm_mul_ps(mQ, t.mQ);
}
inline void LLVector4a::setCross3(const LLVector4a& a, const LLVector4a& b)
{
	const LLQuad vector1 = _mm_shuffle_ps( a.mQ, a.mQ, _MM_SHUFFLE( 3, 0, 2, 1 ));
	const LLQuad vector2 = _mm_shuffle_ps( b.mQ, b.mQ, _MM_SHUFFLE( 3, 1, 0, 2 ));
	mQ = _mm_mul_ps( vector1, vector2 );
	const LLQuad vector3 = _mm_shuffle_ps( a.mQ, a.mQ, _MM_SHUFFLE( 3, 1, 0, 2 ));
	const LLQuad vector4 = _mm_shuffle_ps( b.mQ, b.mQ, _MM_SHUFFLE( 3, 0, 2, 1 ));
	mQ = _mm_sub_ps( mQ, _mm_mul_ps( vector3, vector4 ));
}
inline void LLVector4a::setAllDot3(const LLVector4a& a, const LLVector4a& b)
{
	const LLQuad ab = _mm_mul_ps( a.mQ, b.mQ );
	const __m128i wzxy = _mm_shuffle_epi32(_mm_castps_si128(ab), _MM_SHUFFLE(3, 2, 0, 1 ));
	const LLQuad xPlusY = _mm_add_ps(ab, _mm_castsi128_ps(wzxy));
	const LLQuad xPlusYSplat = _mm_movelh_ps(xPlusY, xPlusY);
	const __m128i zSplat = _mm_shuffle_epi32(_mm_castps_si128(ab), _MM_SHUFFLE( 2, 2, 2, 2 ));
	mQ = _mm_add_ps(_mm_castsi128_ps(zSplat), xPlusYSplat);
}
inline void LLVector4a::setAllDot4(const LLVector4a& a, const LLVector4a& b)
{
	const LLQuad ab = _mm_mul_ps( a.mQ, b.mQ );
	const __m128i zwxy = _mm_shuffle_epi32(_mm_castps_si128(ab), _MM_SHUFFLE(2, 3, 0, 1 ));
	const LLQuad zPlusWandXplusY = _mm_add_ps(ab, _mm_castsi128_ps(zwxy));
	const LLQuad xPlusYSplat = _mm_movelh_ps(zPlusWandXplusY, zPlusWandXplusY);
	const LLQuad zPlusWSplat = _mm_movehl_ps(zPlusWandXplusY, zPlusWandXplusY);
	mQ = _mm_add_ps(xPlusYSplat, zPlusWSplat);
}
inline LLSimdScalar LLVector4a::dot3(const LLVector4a& b) const
{
	const LLQuad ab = _mm_mul_ps( mQ, b.mQ );
	const LLQuad splatY = _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(ab), _MM_SHUFFLE(1, 1, 1, 1) ) );
	const LLQuad splatZ = _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(ab), _MM_SHUFFLE(2, 2, 2, 2) ) );
	const LLQuad xPlusY = _mm_add_ps( ab, splatY );
	return _mm_add_ps( xPlusY, splatZ );
}
inline LLSimdScalar LLVector4a::dot4(const LLVector4a& b) const
{
 	const LLQuad ab = _mm_mul_ps( mQ, b.mQ );
	const LLQuad upperProdsInLowerElems = _mm_movehl_ps( ab, ab );
 	const LLQuad sumOfPairs = _mm_add_ps( upperProdsInLowerElems, ab );
	const LLQuad shuffled = _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128( sumOfPairs ), _MM_SHUFFLE(1, 1, 1, 1) ) );
	return _mm_add_ss( sumOfPairs, shuffled );
}
inline void LLVector4a::normalize3()
{
	LLVector4a lenSqrd; lenSqrd.setAllDot3( *this, *this );
	const LLQuad rsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	static const LLQuad half = { 0.5f, 0.5f, 0.5f, 0.5f };
	static const LLQuad three = {3.f, 3.f, 3.f, 3.f };
	const LLQuad AtimesRsqrt = _mm_mul_ps( lenSqrd.mQ, rsqrt );
	const LLQuad AtimesRsqrtTimesRsqrt = _mm_mul_ps( AtimesRsqrt, rsqrt );
	const LLQuad threeMinusAtimesRsqrtTimesRsqrt = _mm_sub_ps(three, AtimesRsqrtTimesRsqrt );
	const LLQuad nrApprox = _mm_mul_ps(half, _mm_mul_ps(rsqrt, threeMinusAtimesRsqrtTimesRsqrt));
	mQ = _mm_mul_ps( mQ, nrApprox );
}
inline void LLVector4a::normalize4()
{
	LLVector4a lenSqrd; lenSqrd.setAllDot4( *this, *this );
	const LLQuad rsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	static const LLQuad half = { 0.5f, 0.5f, 0.5f, 0.5f };
	static const LLQuad three = {3.f, 3.f, 3.f, 3.f };
	const LLQuad AtimesRsqrt = _mm_mul_ps( lenSqrd.mQ, rsqrt );
	const LLQuad AtimesRsqrtTimesRsqrt = _mm_mul_ps( AtimesRsqrt, rsqrt );
	const LLQuad threeMinusAtimesRsqrtTimesRsqrt = _mm_sub_ps(three, AtimesRsqrtTimesRsqrt );
	const LLQuad nrApprox = _mm_mul_ps(half, _mm_mul_ps(rsqrt, threeMinusAtimesRsqrtTimesRsqrt));
	mQ = _mm_mul_ps( mQ, nrApprox );
}
inline LLSimdScalar LLVector4a::normalize3withLength()
{
	LLVector4a lenSqrd; lenSqrd.setAllDot3( *this, *this );
	const LLQuad rsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	static const LLQuad half = { 0.5f, 0.5f, 0.5f, 0.5f };
	static const LLQuad three = {3.f, 3.f, 3.f, 3.f };
	const LLQuad AtimesRsqrt = _mm_mul_ps( lenSqrd.mQ, rsqrt );
	const LLQuad AtimesRsqrtTimesRsqrt = _mm_mul_ps( AtimesRsqrt, rsqrt );
	const LLQuad threeMinusAtimesRsqrtTimesRsqrt = _mm_sub_ps(three, AtimesRsqrtTimesRsqrt );
	const LLQuad nrApprox = _mm_mul_ps(half, _mm_mul_ps(rsqrt, threeMinusAtimesRsqrtTimesRsqrt));
	mQ = _mm_mul_ps( mQ, nrApprox );
	return _mm_sqrt_ss(lenSqrd);
}
inline void LLVector4a::normalize3fast()
{
	LLVector4a lenSqrd; lenSqrd.setAllDot3( *this, *this );
	const LLQuad approxRsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	mQ = _mm_mul_ps( mQ, approxRsqrt );
}
inline void LLVector4a::normalize3fast_checked(LLVector4a* d)
{
	if (!isFinite3())
	{
		*this = d ? *d : LLVector4a(0,1,0,1);
		return;
	}
	LLVector4a lenSqrd; lenSqrd.setAllDot3( *this, *this );
	if (lenSqrd.getF32ptr()[0] <= FLT_EPSILON)
	{
		*this = d ? *d : LLVector4a(0,1,0,1);
		return;
	}
	const LLQuad approxRsqrt = _mm_rsqrt_ps(lenSqrd.mQ);
	mQ = _mm_mul_ps( mQ, approxRsqrt );
}
inline LLBool32 LLVector4a::isNormalized3( F32 tolerance ) const
{
	static LL_ALIGN_16(const U32 ones[4]) = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000 };
	LLSimdScalar tol = _mm_load_ss( &tolerance );
	tol = _mm_mul_ss( tol, tol );
	LLVector4a lenSquared; lenSquared.setAllDot3( *this, *this );
	lenSquared.sub( *reinterpret_cast<const LLVector4a*>(ones) );
	lenSquared.setAbs(lenSquared);
	return _mm_comile_ss( lenSquared, tol );
}
inline LLBool32 LLVector4a::isNormalized4( F32 tolerance ) const
{
	static LL_ALIGN_16(const U32 ones[4]) = { 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000 };
	LLSimdScalar tol = _mm_load_ss( &tolerance );
	tol = _mm_mul_ss( tol, tol );
	LLVector4a lenSquared; lenSquared.setAllDot4( *this, *this );
	lenSquared.sub( *reinterpret_cast<const LLVector4a*>(ones) );
	lenSquared.setAbs(lenSquared);
	return _mm_comile_ss( lenSquared, tol );
}
inline void LLVector4a::setAllLength3( const LLVector4a& v )
{
	LLVector4a lenSqrd;
	lenSqrd.setAllDot3(v, v);
	mQ = _mm_sqrt_ps(lenSqrd.mQ);
}
inline LLSimdScalar LLVector4a::getLength3() const
{
	return _mm_sqrt_ss( dot3( (const LLVector4a)mQ ) );
}
inline void LLVector4a::setMin(const LLVector4a& lhs, const LLVector4a& rhs)
{
	mQ = _mm_min_ps(lhs.mQ, rhs.mQ);
}
inline void LLVector4a::setMax(const LLVector4a& lhs, const LLVector4a& rhs)
{
	mQ = _mm_max_ps(lhs.mQ, rhs.mQ);
}
inline void LLVector4a::setLerp(const LLVector4a& lhs, const LLVector4a& rhs, F32 c)
{
	LLVector4a t;
	t.setSub(rhs,lhs);
	t.mul(c);
	setAdd(lhs, t);
}
inline LLBool32 LLVector4a::isFinite3() const
{
	static LL_ALIGN_16(const U32 nanOrInfMask[4]) = { 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000 };
	ll_assert_aligned(nanOrInfMask,16);
	const __m128i nanOrInfMaskV = *reinterpret_cast<const __m128i*> (nanOrInfMask);
	const __m128i maskResult = _mm_and_si128( _mm_castps_si128(mQ), nanOrInfMaskV );
	const LLVector4Logical equalityCheck = _mm_castsi128_ps(_mm_cmpeq_epi32( maskResult, nanOrInfMaskV ));
	return !equalityCheck.areAnySet( LLVector4Logical::MASK_XYZ );
}
inline LLBool32 LLVector4a::isFinite4() const
{
	static LL_ALIGN_16(const U32 nanOrInfMask[4]) = { 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000 };
	const __m128i nanOrInfMaskV = *reinterpret_cast<const __m128i*> (nanOrInfMask);
	const __m128i maskResult = _mm_and_si128( _mm_castps_si128(mQ), nanOrInfMaskV );
	const LLVector4Logical equalityCheck = _mm_castsi128_ps(_mm_cmpeq_epi32( maskResult, nanOrInfMaskV ));
	return !equalityCheck.areAnySet( LLVector4Logical::MASK_XYZW );
}
inline void LLVector4a::setRotatedInv( const LLRotation& rot, const LLVector4a& vec )
{
	LLRotation inv; inv.setTranspose( rot );
	setRotated( inv, vec );
}
inline void LLVector4a::setRotatedInv( const LLQuaternion2& quat, const LLVector4a& vec )
{
	LLQuaternion2 invRot; invRot.setConjugate( quat );
	setRotated(invRot, vec);
}
inline void LLVector4a::clamp( const LLVector4a& low, const LLVector4a& high )
{
	const LLVector4Logical highMask = greaterThan( high );
	const LLVector4Logical lowMask = lessThan( low );
	setSelectWithMask( highMask, high, *this );
	setSelectWithMask( lowMask, low, *this );
}
inline void LLVector4a::negate()
{
	static LL_ALIGN_16(const U32 signMask[4]) = {0x80000000, 0x80000000, 0x80000000, 0x80000000 };
	mQ = _mm_xor_ps(*reinterpret_cast<const LLQuad*>(signMask), mQ);
}
inline LLVector4Logical LLVector4a::greaterThan(const LLVector4a& rhs) const
{
	return _mm_cmpgt_ps(mQ, rhs.mQ);
}
inline LLVector4Logical LLVector4a::lessThan(const LLVector4a& rhs) const
{
	return _mm_cmplt_ps(mQ, rhs.mQ);
}
inline LLVector4Logical LLVector4a::greaterEqual(const LLVector4a& rhs) const
{
	return _mm_cmpge_ps(mQ, rhs.mQ);
}
inline LLVector4Logical LLVector4a::lessEqual(const LLVector4a& rhs) const
{
	return _mm_cmple_ps(mQ, rhs.mQ);
}
inline LLVector4Logical LLVector4a::equal(const LLVector4a& rhs) const
{
	return _mm_cmpeq_ps(mQ, rhs.mQ);
}
inline bool LLVector4a::equals4(const LLVector4a& rhs, F32 tolerance ) const
{
	LLVector4a diff; diff.setSub( *this, rhs );
	diff.setAbs( diff );
	const LLQuad tol = _mm_set1_ps( tolerance );
	const LLQuad cmp = _mm_cmplt_ps( diff, tol );
	return (_mm_movemask_ps( cmp ) & LLVector4Logical::MASK_XYZW) == LLVector4Logical::MASK_XYZW;
}
inline bool LLVector4a::equals3(const LLVector4a& rhs, F32 tolerance ) const
{
	LLVector4a diff; diff.setSub( *this, rhs );
	diff.setAbs( diff );
	const LLQuad tol = _mm_set1_ps( tolerance );
	const LLQuad t = _mm_cmplt_ps( diff, tol );
	return (_mm_movemask_ps( t ) & LLVector4Logical::MASK_XYZ) == LLVector4Logical::MASK_XYZ;
}
inline const LLVector4a& LLVector4a::operator= (const LLQuad& rhs)
{
	mQ = rhs;
	return *this;
}
inline LLVector4a::operator LLQuad() const
{
	return mQ;
}
