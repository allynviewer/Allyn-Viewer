/** 
 * @file m3math.h
 * @brief LLMatrix3 class header file.
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
#ifndef LL_M3MATH_H
#define LL_M3MATH_H
#include "llerror.h"
#include "stdtypes.h"
class LLVector4;
class LLVector3;
class LLVector3d;
class LLQuaternion;
static const U32 NUM_VALUES_IN_MAT3	= 3;
class LLMatrix3
{
	public:
		F32	mMatrix[NUM_VALUES_IN_MAT3][NUM_VALUES_IN_MAT3];
		LLMatrix3(void);
		explicit LLMatrix3(const F32 *mat);
		explicit LLMatrix3(const LLQuaternion &q);
		LLMatrix3(const F32 angle, const F32 x, const F32 y, const F32 z);
		LLMatrix3(const F32 angle, const LLVector3 &vec);
		LLMatrix3(const F32 angle, const LLVector3d &vec);
		LLMatrix3(const F32 angle, const LLVector4 &vec);
		LLMatrix3(const F32 roll, const F32 pitch, const F32 yaw);
		const LLMatrix3& setIdentity();
		const LLMatrix3& clear();
		const LLMatrix3& setZero();
		const LLMatrix3& setRot(const F32 angle, const F32 x, const F32 y, const F32 z);
		const LLMatrix3& setRot(const F32 angle, const LLVector3 &vec);
		const LLMatrix3& setRot(const F32 roll, const F32 pitch, const F32 yaw);
		const LLMatrix3& setRot(const LLQuaternion &q);
		const LLMatrix3& setRows(const LLVector3 &x_axis, const LLVector3 &y_axis, const LLVector3 &z_axis);
		const LLMatrix3& setRow( U32 rowIndex, const LLVector3& row );
		const LLMatrix3& setCol( U32 colIndex, const LLVector3& col );
		LLQuaternion quaternion() const;
		void getEulerAngles(F32 *roll, F32 *pitch, F32 *yaw) const;
		LLVector3 getFwdRow() const;
		LLVector3 getLeftRow() const;
		LLVector3 getUpRow() const;
		F32	 determinant() const;
		const LLMatrix3& transpose();
		const LLMatrix3& orthogonalize();
		void invert();
		const LLMatrix3& adjointTranspose();
		const LLMatrix3& rotate(const F32 angle, const F32 x, const F32 y, const F32 z);
		const LLMatrix3& rotate(const F32 angle, const LLVector3 &vec);
		const LLMatrix3& rotate(const F32 roll, const F32 pitch, const F32 yaw);
		const LLMatrix3& rotate(const LLQuaternion &q);
		void add(const LLMatrix3& other_matrix);
		friend LLVector3 operator*(const LLVector3 &a, const LLMatrix3 &b);
		friend LLVector3d operator*(const LLVector3d &a, const LLMatrix3 &b);
		friend LLMatrix3 operator*(const LLMatrix3 &a, const LLMatrix3 &b);
		friend bool operator==(const LLMatrix3 &a, const LLMatrix3 &b);
		friend bool operator!=(const LLMatrix3 &a, const LLMatrix3 &b);
		friend const LLMatrix3& operator*=(LLMatrix3 &a, const LLMatrix3 &b);
		friend const LLMatrix3& operator*=(LLMatrix3 &a, F32 scalar );
		friend std::ostream&	 operator<<(std::ostream& s, const LLMatrix3 &a);
};
static_assert(std::is_trivially_copyable<LLMatrix3>::value, "LLMatrix3 must be a trivially copyable type");
inline LLMatrix3::LLMatrix3(void)
{
	mMatrix[0][0] = 1.f;
	mMatrix[0][1] = 0.f;
	mMatrix[0][2] = 0.f;
	mMatrix[1][0] = 0.f;
	mMatrix[1][1] = 1.f;
	mMatrix[1][2] = 0.f;
	mMatrix[2][0] = 0.f;
	mMatrix[2][1] = 0.f;
	mMatrix[2][2] = 1.f;
}
inline LLMatrix3::LLMatrix3(const F32 *mat)
{
	mMatrix[0][0] = mat[0];
	mMatrix[0][1] = mat[1];
	mMatrix[0][2] = mat[2];
	mMatrix[1][0] = mat[3];
	mMatrix[1][1] = mat[4];
	mMatrix[1][2] = mat[5];
	mMatrix[2][0] = mat[6];
	mMatrix[2][1] = mat[7];
	mMatrix[2][2] = mat[8];
}
#endif
