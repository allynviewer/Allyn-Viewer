/**
 * @file llplanarreflection.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llplanarreflection.h"
#include "llreflectionprobe.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llglslshader.h"
#include "llrender.h"
#include "llgl.h"

LLPlanarReflection gPlanarReflection;

namespace
{
	void reflectionMatrix(LLMatrix4a& out, const LLVector3& n, F32 d)
	{
		F32 nx = n.mV[0];
		F32 ny = n.mV[1];
		F32 nz = n.mV[2];
		LLVector4a c0, c1, c2, c3;
		c0.set(1.f - 2.f * nx * nx, -2.f * nx * ny, -2.f * nx * nz, 0.f);
		c1.set(-2.f * ny * nx, 1.f - 2.f * ny * ny, -2.f * ny * nz, 0.f);
		c2.set(-2.f * nz * nx, -2.f * nz * ny, 1.f - 2.f * nz * nz, 0.f);
		c3.set(-2.f * d * nx, -2.f * d * ny, -2.f * d * nz, 1.f);
		out.setRow<0>(c0);
		out.setRow<1>(c1);
		out.setRow<2>(c2);
		out.setRow<3>(c3);
	}
}

LLPlanarReflection::LLPlanarReflection()
:	mSubmitted(false),
	mReady(false),
	mSuppressSample(false),
	mAllocated(false),
	mWidth(0),
	mHeight(0)
{
	mPoint.setVec(0.f, 0.f, 0.f);
	mNormal.setVec(0.f, 0.f, 1.f);
	mWorldToBox.setIdentity();
	mViewProj.setIdentity();
}

void LLPlanarReflection::releaseGL()
{
	mTarget.release();
	mGBuffer.release();
	mAllocated = false;
	mReady = false;
	mSuppressSample = false;
	mWidth = 0;
	mHeight = 0;
}

void LLPlanarReflection::submit(const LLVector3& point, const LLVector3& normal, const LLMatrix4a& world_to_box)
{
	mPoint = point;
	mNormal = normal;
	mNormal.normVec();
	mWorldToBox = world_to_box;
	mSubmitted = true;
}

void LLPlanarReflection::clearSubmitted()
{
	mSubmitted = false;
}

void LLPlanarReflection::allocate(S32 width, S32 height)
{
	if (mAllocated && mWidth == width && mHeight == height)
	{
		return;
	}
	releaseGL();
	GLenum light_format = allynHDRColorTarget() ? GL_RGBA16F_ARB : GL_RGBA;
	if (!mGBuffer.allocate((U32)width, (U32)height, GL_SRGB8_ALPHA8, TRUE, FALSE, LLTexUnit::TT_TEXTURE, FALSE))
	{
		return;
	}
	if (!mGBuffer.addColorAttachment(GL_RGBA) || !mGBuffer.addColorAttachment(GL_RGB10_A2) || !mGBuffer.addColorAttachment(light_format))
	{
		mGBuffer.release();
		return;
	}
	if (!mTarget.allocate((U32)width, (U32)height, light_format, FALSE, FALSE, LLTexUnit::TT_TEXTURE, FALSE))
	{
		mGBuffer.release();
		return;
	}
	mAllocated = true;
	mWidth = width;
	mHeight = height;
}

void LLPlanarReflection::capture()
{
	S32 detail = gSavedSettings.getS32("RenderMirrorDetail");
	if (!mSubmitted || detail <= 0 || !LLPipeline::sRenderDeferred || gPipeline.mDeferredScreen.getWidth() < 1)
	{
		mReady = false;
		mSubmitted = false;
		return;
	}
	F32 scale = 0.25f;
	if (detail == 2)
	{
		scale = 0.5f;
	}
	else if (detail == 3)
	{
		scale = 0.75f;
	}
	else if (detail >= 4)
	{
		scale = 1.f;
	}
	S32 width = llmax((S32)((F32)gViewerWindow->getWindowWidthRaw() * scale), 1);
	S32 height = llmax((S32)((F32)gViewerWindow->getWindowHeightRaw() * scale), 1);
	allocate(width, height);
	if (!mAllocated)
	{
		mReady = false;
		mSubmitted = false;
		return;
	}
	LLViewerCamera* view_cam = LLViewerCamera::getInstance();
	LLVector3 saved_origin = view_cam->getOrigin();
	LLVector3 saved_at = view_cam->getAtAxis();
	LLVector3 saved_up = view_cam->getUpAxis();
	F32 saved_view = view_cam->getView();
	F32 saved_aspect = view_cam->getAspect();
	F32 saved_near = view_cam->getNear();
	F32 saved_far = view_cam->getFar();
	LLRect saved_viewport = gGLViewport;
	LLMatrix4a saved_modelview = glh_get_current_modelview();
	LLMatrix4a saved_projection = glh_get_current_projection();
	LLViewerCamera::eCameraID saved_camera = LLViewerCamera::sCurCameraID;
	BOOL saved_reflection = LLPipeline::sReflectionRender;
	GLint saved_cull = GL_BACK;
	glGetIntegerv(GL_CULL_FACE_MODE, &saved_cull);
	LLPipeline::sReflectionRender = TRUE;
	LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_GI_SOURCE;
	F32 plane_d = -mNormal * mPoint;
	LLMatrix4a reflection;
	reflectionMatrix(reflection, mNormal, plane_d);
	LLMatrix4a mirror_view;
	mirror_view.setMul(saved_modelview, reflection);
	glh_set_current_modelview(mirror_view);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadMatrix(mirror_view);
	mViewProj.setMul(saved_projection, mirror_view);
	LLCamera camera(*view_cam);
	LLMatrix4a inv = mirror_view;
	inv.invert();
	LLVector4a origin;
	origin.clear();
	inv.affineTransform(origin, origin);
	LLVector4a eye_forward;
	eye_forward.set(0.f, 0.f, -1.f);
	LLVector4a eye_up;
	eye_up.set(0.f, 1.f, 0.f);
	LLVector4a world_forward;
	LLVector4a world_up;
	inv.rotate(eye_forward, world_forward);
	inv.rotate(eye_up, world_up);
	LLVector3 origin_v;
	LLVector3 forward_v;
	LLVector3 up_v;
	origin_v.setVec(origin.getF32ptr());
	forward_v.setVec(world_forward.getF32ptr());
	up_v.setVec(world_up.getF32ptr());
	if (forward_v.normVec() > 0.f && up_v.normVec() > 0.f)
	{
		camera.setOriginAndLookAt(origin_v, up_v, origin_v + forward_v);
	}
	else
	{
		camera.setOrigin(origin_v);
	}
	LLViewerCamera::updateFrustumPlanes(camera, FALSE, TRUE);
	LLPlane plane(mNormal, plane_d);
	{
		LLGLUserClipPlane clip(plane, mirror_view, saved_projection, true);
		glCullFace(GL_FRONT);
		mGBuffer.bindTarget();
		glClearColor(0.f, 0.f, 0.f, 0.f);
		mGBuffer.clear();
		gGL.setViewport(0, 0, mWidth, mHeight);
		gPipeline.setCaptureGBuffer(&mGBuffer);
		static LLCullResult result;
		gPipeline.updateCull(camera, result);
		gPipeline.stateSort(camera, result);
		gPipeline.renderGeomDeferred(camera);
		mGBuffer.flush();
		mSuppressSample = true;
		gPipeline.renderDeferredLightingToRT(&mTarget);
		mSuppressSample = false;
		gPipeline.setCaptureGBuffer(NULL);
		mTarget.flush();
	}
	glCullFace(saved_cull);
	LLPipeline::sReflectionRender = saved_reflection;
	LLViewerCamera::sCurCameraID = saved_camera;
	view_cam->setOriginAndLookAt(saved_origin, saved_up, saved_origin + saved_at);
	view_cam->setView(saved_view);
	view_cam->setAspect(saved_aspect);
	view_cam->setNear(saved_near);
	view_cam->setFar(saved_far);
	glh_set_current_modelview(saved_modelview);
	glh_set_current_projection(saved_projection);
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.loadMatrix(saved_projection);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.loadMatrix(saved_modelview);
	gGL.setViewport(saved_viewport);
	mReady = true;
	mSubmitted = false;
}

void LLPlanarReflection::bindState(LLGLSLShader* shader, S32 unit)
{
	if (!shader)
	{
		return;
	}
	static const LLStaticHashedString present_name("pbr_mirror_present");
	static const LLStaticHashedString map_name("pbrMirrorMap");
	static const LLStaticHashedString point_name("pbr_mirror_point");
	static const LLStaticHashedString normal_name("pbr_mirror_normal");
	static const LLStaticHashedString vp_name("pbr_mirror_vp");
	static const LLStaticHashedString box_name("pbr_mirror_box");
	bool sample = mReady && mAllocated && !mSuppressSample;
	shader->uniform1f(present_name, sample ? 1.f : 0.f);
	shader->uniform1i(map_name, unit);
	if (sample)
	{
		gGL.getTexUnit(unit)->bind(&mTarget);
		gGL.getTexUnit(unit)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
		gGL.getTexUnit(unit)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
	}
	shader->uniform3f(point_name, mPoint.mV[0], mPoint.mV[1], mPoint.mV[2]);
	shader->uniform3f(normal_name, mNormal.mV[0], mNormal.mV[1], mNormal.mV[2]);
	shader->uniformMatrix4fv(vp_name, 1, FALSE, mViewProj.getF32ptr());
	shader->uniformMatrix4fv(box_name, 1, FALSE, mWorldToBox.getF32ptr());
}
