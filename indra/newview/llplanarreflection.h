/**
 * @file llplanarreflection.h
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * $/LicenseInfo$
 */
#ifndef LL_LLPLANARREFLECTION_H
#define LL_LLPLANARREFLECTION_H

#include "llgl.h"
#include "llrendertarget.h"
#include "v3math.h"
#include "llmatrix4a.h"

class LLGLSLShader;

class LLPlanarReflection
{
public:
	LLPlanarReflection();
	void releaseGL();
	void submit(const LLVector3& point, const LLVector3& normal, const LLMatrix4a& world_to_box);
	void clearSubmitted();
	void capture();
	void bindState(LLGLSLShader* shader, S32 unit);
	bool hasCapture() const { return mReady; }
private:
	void allocate(S32 width, S32 height);
	LLRenderTarget mTarget;
	LLRenderTarget mGBuffer;
	LLVector3 mPoint;
	LLVector3 mNormal;
	LLMatrix4a mWorldToBox;
	LLMatrix4a mViewProj;
	bool mSubmitted;
	bool mReady;
	bool mSuppressSample;
	bool mAllocated;
	S32 mWidth;
	S32 mHeight;
};

extern LLPlanarReflection gPlanarReflection;

#endif
