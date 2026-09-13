/** 
 * @file m4math.h
 * @brief LLMatrix4 class header file.
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
#ifndef LL_M4MATH_H
#define LL_M4MATH_H
#include "v3math.h"
class LLVector4;
class LLMatrix3;
class LLQuaternion;
static const U32 NUM_VALUES_IN_MAT4 = 4;
class LLMatrix4
{
public:
	F32	mMatrix[NUM_VALUES_IN_MAT4][NUM_VALUES_IN_MAT4];
	LLMatrix4()
	{
		setIdentity();
	}
	explicit LLMatrix4(const F32 *mat);
	explicit LLMatrix4(const LLMatrix3 &mat);
	explicit LLMatrix4(const LLQuaternion &q);
	LLMatrix4(const LLMatrix3 &mat, const LLVector4 &pos);
	LLMatrix4(const LLQuaternion &q, const LLVector4 &pos);
	LLMatrix4(F32 angle,
			  const LLVector4 &vec,
			  const LLVector4 &pos);
	LLMatrix4(F32 angle, const LLVector4 &vec);
	LLMatrix4(const F32 roll, const F32 pitch, const F32 yaw,
			  const LLVector4 &pos);
	LLMatrix4(const F32 roll, const F32 pitch, const F32 yaw);
	~LLMatrix4() = default;
	LLSD getValue() const;
	void setValue(const LLSD&);
	void initRows(const LLVector4 &row0,
				  const LLVector4 &row1,
				  const LLVector4 &row2,
				  const LLVector4 &row3);
	const LLMatrix4& setIdentity();
	bool isIdentity() const;
	const LLMatrix4& setZero();
	const LLMatrix4& initRotation(const F32 angle, const F32 x, const F32 y, const F32 z);
	const LLMatrix4& initRotation(const F32 angle, const LLVector4 &axis);
	const LLMatrix4& initRotation(const F32 roll, const F32 pitch, const F32 yaw);
	const LLMatrix4& initRotation(const LLQuaternion &q);
	const LLMatrix4& initMatrix(const LLMatrix3 &mat);
	const LLMatrix4& initMatrix(const LLMatrix3 &mat, const LLVector4 &translation);
	const LLMatrix4& initRotTrans(const F32 angle,
								  const F32 rx, const F32 ry, const F32 rz,
								  const F32 px, const F32 py, const F32 pz);
	const LLMatrix4& initRotTrans(const F32 angle, const LLVector3 &axis, const LLVector3 &translation);
	const LLMatrix4& initRotTrans(const F32 roll, const F32 pitch, const F32 yaw, const LLVector4 &pos);
	const LLMatrix4& initRotTrans(const LLQuaternion &q, const LLVector4 &pos);
	const LLMatrix4& initScale(const LLVector3 &scale);
	const LLMatrix4& initAll(const LLVector3 &scale, const LLQuaternion &q, const LLVector3 &pos);
	const LLMatrix4& setTranslation(const F32 x, const F32 y, const F32 z);
	void setFwdRow(const LLVector3 &row);
	void setLeftRow(const LLVector3 &row);
	void setUpRow(const LLVector3 &row);
	void setFwdCol(const LLVector3 &col);
	void setLeftCol(const LLVector3 &col);
	void setUpCol(const LLVector3 &col);
	const LLMatrix4& setTranslation(const LLVector4 &translation);
	const LLMatrix4& setTranslation(const LLVector3 &translation);
	void condition(void);
	F32			 determinant(void) const;
	LLQuaternion quaternion(void) const;
	LLVector4 getFwdRow4() const;
	LLVector4 getLeftRow4() const;
	LLVector4 getUpRow4() const;
	LLMatrix3 getMat3() const;
	const LLVector3& getTranslation() const { return *(LLVector3*)&mMatrix[3][0]; }
	const LLMatrix4& transpose();
	const LLMatrix4& invert();
	const LLMatrix4& rotate(const F32 angle, const F32 x, const F32 y, const F32 z);
	const LLMatrix4& rotate(const F32 angle, const LLVector4 &vec);
	const LLMatrix4& rotate(const F32 roll, const F32 pitch, const F32 yaw);
	const LLMatrix4& rotate(const LLQuaternion &q);
	const LLMatrix4& translate(const LLVector3 &vec);
	friend LLVector4 operator*(const LLVector4 &a, const LLMatrix4 &b);
	friend const LLVector3 operator*(const LLVector3 &a, const LLMatrix4 &b);
	friend LLVector4 rotate_vector(const LLVector4 &a, const LLMatrix4 &b);
	friend LLVector3 rotate_vector(const LLVector3 &a, const LLMatrix4 &b);
	friend bool operator==(const LLMatrix4 &a, const LLMatrix4 &b);
	friend bool operator!=(const LLMatrix4 &a, const LLMatrix4 &b);
	friend bool operator<(const LLMatrix4 &a, const LLMatrix4& b);
	friend const LLMatrix4& operator+=(LLMatrix4 &a, const LLMatrix4 &b);
	friend const LLMatrix4& operator-=(LLMatrix4 &a, const LLMatrix4 &b);
	friend const LLMatrix4& operator*=(LLMatrix4 &a, const LLMatrix4 &b);
	friend const LLMatrix4& operator*=(LLMatrix4 &a, const F32 &b);
	friend std::ostream&	 operator<<(std::ostream& s, const LLMatrix4 &a);
};
static_assert(std::is_trivially_copyable<LLMatrix4>::value, "LLMatrix4 must be a trivially copyable type");
inline const LLMatrix4&	LLMatrix4::setIdentity()
{
	mMatrix[0][0] = 1.f;
	mMatrix[0][1] = 0.f;
	mMatrix[0][2] = 0.f;
	mMatrix[0][3] = 0.f;
	mMatrix[1][0] = 0.f;
	mMatrix[1][1] = 1.f;
	mMatrix[1][2] = 0.f;
	mMatrix[1][3] = 0.f;
	mMatrix[2][0] = 0.f;
	mMatrix[2][1] = 0.f;
	mMatrix[2][2] = 1.f;
	mMatrix[2][3] = 0.f;
	mMatrix[3][0] = 0.f;
	mMatrix[3][1] = 0.f;
	mMatrix[3][2] = 0.f;
	mMatrix[3][3] = 1.f;
	return (*this);
}
inline bool LLMatrix4::isIdentity() const
{
	return
		mMatrix[0][0] == 1.f &&
		mMatrix[0][1] == 0.f &&
		mMatrix[0][2] == 0.f &&
		mMatrix[0][3] == 0.f &&
		mMatrix[1][0] == 0.f &&
		mMatrix[1][1] == 1.f &&
		mMatrix[1][2] == 0.f &&
		mMatrix[1][3] == 0.f &&
		mMatrix[2][0] == 0.f &&
		mMatrix[2][1] == 0.f &&
		mMatrix[2][2] == 1.f &&
		mMatrix[2][3] == 0.f &&
		mMatrix[3][0] == 0.f &&
		mMatrix[3][1] == 0.f &&
		mMatrix[3][2] == 0.f &&
		mMatrix[3][3] == 1.f;
}
inline const LLMatrix4& operator*=(LLMatrix4 &a, const LLMatrix4 &b)
{
	U32		i, j;
	LLMatrix4	mat;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][0] * b.mMatrix[0][i] +
							    a.mMatrix[j][1] * b.mMatrix[1][i] +
							    a.mMatrix[j][2] * b.mMatrix[2][i] +
								a.mMatrix[j][3] * b.mMatrix[3][i];
		}
	}
	a = mat;
	return a;
}
inline const LLMatrix4& operator*=(LLMatrix4 &a, const F32 &b)
{
	U32		i, j;
	LLMatrix4	mat;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][i] * b;
		}
	}
	a = mat;
	return a;
}
inline const LLMatrix4& operator+=(LLMatrix4 &a, const LLMatrix4 &b)
{
	LLMatrix4 mat;
	U32		i, j;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][i] + b.mMatrix[j][i];
		}
	}
	a = mat;
	return a;
}
inline const LLMatrix4& operator-=(LLMatrix4 &a, const LLMatrix4 &b)
{
	LLMatrix4 mat;
	U32		i, j;
	for (i = 0; i < NUM_VALUES_IN_MAT4; i++)
	{
		for (j = 0; j < NUM_VALUES_IN_MAT4; j++)
		{
			mat.mMatrix[j][i] = a.mMatrix[j][i] - b.mMatrix[j][i];
		}
	}
	a = mat;
	return a;
}
inline const LLVector3 operator*(const LLVector3 &a, const LLMatrix4 &b)
{
	return LLVector3(a.mV[VX] * b.mMatrix[VX][VX] +
					 a.mV[VY] * b.mMatrix[VY][VX] +
					 a.mV[VZ] * b.mMatrix[VZ][VX] +
					 b.mMatrix[VW][VX],
					 a.mV[VX] * b.mMatrix[VX][VY] +
					 a.mV[VY] * b.mMatrix[VY][VY] +
					 a.mV[VZ] * b.mMatrix[VZ][VY] +
					 b.mMatrix[VW][VY],
					 a.mV[VX] * b.mMatrix[VX][VZ] +
					 a.mV[VY] * b.mMatrix[VY][VZ] +
					 a.mV[VZ] * b.mMatrix[VZ][VZ] +
					 b.mMatrix[VW][VZ]);
}
#endif
