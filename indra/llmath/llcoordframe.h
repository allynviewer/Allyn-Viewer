/** 
 * @file llcoordframe.h
 * @brief LLCoordFrame class header file.
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
#ifndef LL_COORDFRAME_H
#define LL_COORDFRAME_H
#include "v3math.h"
#include "v4math.h"
#include "llerror.h"
class LLCoordFrame
{
public:
	LLCoordFrame();
	explicit LLCoordFrame(const LLVector3 &origin);
	LLCoordFrame(const LLVector3 &x_axis,
				 const LLVector3 &y_axis,
				 const LLVector3 &z_axis);
	LLCoordFrame(const LLVector3 &origin,
				 const LLVector3 &x_axis,
				 const LLVector3 &y_axis,
				 const LLVector3 &z_axis);
	LLCoordFrame(const LLVector3 &origin,
				 const LLMatrix3 &rotation);
	LLCoordFrame(const LLVector3 &origin,
				 const LLVector3 &direction);
	explicit LLCoordFrame(const LLQuaternion &q);
	LLCoordFrame(const LLVector3 &origin,
				 const LLQuaternion &q);
	explicit LLCoordFrame(const LLMatrix4 &mat);
	BOOL isFinite() { return mOrigin.isFinite() && mXAxis.isFinite() && mYAxis.isFinite() && mZAxis.isFinite(); }
	void reset();
	void resetAxes();
	void setOrigin(F32 x, F32 y, F32 z);
	void setOrigin(const LLVector3 &origin);
	void setOrigin(const F32 *origin);
	void setOrigin(const LLCoordFrame &frame);
	inline void setOriginX(F32 x) { mOrigin.mV[VX] = x; }
	inline void setOriginY(F32 y) { mOrigin.mV[VY] = y; }
	inline void setOriginZ(F32 z) { mOrigin.mV[VZ] = z; }
	void setAxes(const LLVector3 &x_axis,
				 const LLVector3 &y_axis,
				 const LLVector3 &z_axis);
	void setAxes(const LLMatrix3 &rotation_matrix);
	void setAxes(const LLQuaternion &q);
	void setAxes(const F32 *rotation_matrix);
	void setAxes(const LLCoordFrame &frame);
	void translate(F32 x, F32 y, F32 z);
	void translate(const LLVector3 &v);
	void translate(const F32 *origin);
	void rotate(F32 angle, F32 x, F32 y, F32 z);
	void rotate(F32 angle, const LLVector3 &rotation_axis);
	void rotate(const LLQuaternion &q);
	void rotate(const LLMatrix3 &m);
	void orthonormalize();
	void roll(F32 angle);
	void pitch(F32 angle);
	void yaw(F32 angle);
	inline const LLVector3 &getOrigin() const { return mOrigin; }
	inline const LLVector3 &getXAxis() const  { return mXAxis; }
	inline const LLVector3 &getYAxis() const  { return mYAxis; }
	inline const LLVector3 &getZAxis() const  { return mZAxis; }
	inline const LLVector3 &getAtAxis() const   { return mXAxis; }
	inline const LLVector3 &getLeftAxis() const { return mYAxis; }
	inline const LLVector3 &getUpAxis() const   { return mZAxis; }
	LLQuaternion getQuaternion() const;
	void getMatrixToParent(LLMatrix4 &mat) const;
	void getMatrixToLocal(LLMatrix4 &mat) const;
	void getRotMatrixToParent(LLMatrix4 &mat) const;
	size_t writeOrientation(char *buffer) const;
	size_t readOrientation(const char *buffer);
	LLVector3 rotateToLocal(const LLVector3 &v) const;
	LLVector4 rotateToLocal(const LLVector4 &v) const;
	LLVector3 rotateToAbsolute(const LLVector3 &v) const;
	LLVector4 rotateToAbsolute(const LLVector4 &v) const;
	LLVector3 transformToLocal(const LLVector3 &v) const;
	LLVector4 transformToLocal(const LLVector4 &v) const;
	LLVector3 transformToAbsolute(const LLVector3 &v) const;
	LLVector4 transformToAbsolute(const LLVector4 &v) const;
	void getOpenGLTranslation(F32 *ogl_matrix) const;
	void getOpenGLRotation(F32 *ogl_matrix) const;
	void getOpenGLTransform(F32 *ogl_matrix) const;
	void lookDir(const LLVector3 &xuv, const LLVector3 &up);
	void lookDir(const LLVector3 &xuv);
	void lookAt(const LLVector3 &origin, const LLVector3 &point_of_interest, const LLVector3 &up);
	void lookAt(const LLVector3 &origin, const LLVector3 &point_of_interest);
	void setOriginAndLookAt(const LLVector3 &origin, const LLVector3 &up, const LLVector3 &point_of_interest)
	{
		lookAt(origin, point_of_interest, up);
	}
	friend std::ostream& operator<<(std::ostream &s, const LLCoordFrame &C);
	LLVector3 mOrigin;
	LLVector3 mXAxis;
	LLVector3 mYAxis;
	LLVector3 mZAxis;
};
#endif
