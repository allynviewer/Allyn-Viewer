/**
 * @file llreflectionprobe.cpp
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
#include "llreflectionprobe.h"
#include "llplanarreflection.h"
#include "llviewershadermgr.h"
#include "pipeline.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llglslshader.h"
#include "llrender.h"
#include "llgl.h"
#include "llframetimer.h"
#include <vector>
#include <algorithm>

#ifndef GL_TEXTURE_CUBE_MAP_ARRAY
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x900A
#endif
#ifndef GL_MAX_ARRAY_TEXTURE_LAYERS
#define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif
#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif

typedef void (APIENTRY *AllynTexImage3DFn)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void (APIENTRY *AllynCopyTexSubImage3DFn)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
static AllynTexImage3DFn sTexImage3D = NULL;
static AllynCopyTexSubImage3DFn sCopyTexSubImage3D = NULL;

static bool allynPBRTex3D()
{
	if (!sTexImage3D)
	{
		sTexImage3D = (AllynTexImage3DFn)wglGetProcAddress("glTexImage3D");
	}
	if (!sCopyTexSubImage3D)
	{
		sCopyTexSubImage3D = (AllynCopyTexSubImage3DFn)wglGetProcAddress("glCopyTexSubImage3D");
	}
	return sTexImage3D != NULL && sCopyTexSubImage3D != NULL;
}

LLGLSLShader gRadianceGenProgram(LLViewerShaderMgr::SHADER_DEFERRED);
LLGLSLShader gReflectionMipProgram(LLViewerShaderMgr::SHADER_DEFERRED);
LLGLSLShader gIrradianceGenProgram(LLViewerShaderMgr::SHADER_DEFERRED);

namespace
{
	const S32 PROBE_RES = 128;
	const S32 CAPTURE_RES = 256;
	const S32 PROBE_MIPS = 7;
	const S32 IRRADIANCE_RES = 16;
	const S32 MAX_SLOTS = 256;
	const S32 SHADER_PROBES = 8;
	const F32 CELL_NEAR = 16.f;
	const F32 CELL_MAX = 256.f;
	const F32 PROBE_RANGE = 512.f;
	const F32 SKY_HEIGHT = 64.f;
	const F64 SCAN_PERIOD = 0.5;
	const F64 RETIRE_SECONDS = 5.0;
	const S32 MAX_AUTO_NEAR = 8;

	const U32 kCubeFace[6] =
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
	};

	const LLVector3 kFaceAt[6] =
	{
		LLVector3(1.f, 0.f, 0.f),
		LLVector3(-1.f, 0.f, 0.f),
		LLVector3(0.f, 1.f, 0.f),
		LLVector3(0.f, -1.f, 0.f),
		LLVector3(0.f, 0.f, 1.f),
		LLVector3(0.f, 0.f, -1.f)
	};

	const LLVector3 kFaceUp[6] =
	{
		LLVector3(0.f, -1.f, 0.f),
		LLVector3(0.f, -1.f, 0.f),
		LLVector3(0.f, 0.f, 1.f),
		LLVector3(0.f, 0.f, -1.f),
		LLVector3(0.f, -1.f, 0.f),
		LLVector3(0.f, -1.f, 0.f)
	};

	bool sHDRFloat = false;

	struct Probe
	{
		Probe()
		:	radius(0.f),
			faces(0),
			build_faces(0),
			rebuilding(false),
			filtered(false),
			sky(false),
			live(false),
			manual(false),
			touched(0.0),
			captured(0.0),
			near_clip(0.1f),
			stamp(0)
		{
		}
		LLVector3 origin;
		F32 radius;
		U32 faces;
		U32 build_faces;
		bool rebuilding;
		bool filtered;
		bool sky;
		bool live;
		bool manual;
		LLUUID manual_id;
		F64 touched;
		F64 captured;
		F32 near_clip;
		U32 stamp;
	};

	class ReflectionMaps
	{
	public:
		ReflectionMaps()
		:	mTexture(0),
			mIrradiance(0),
			mStaging(0),
			mDummyCube(0),
			mDummyIrradiance(0),
			mCopyFBO(0),
			mSlots(0),
			mArray(false),
			mFloat(false),
			mReady(false),
			mLastScan(0.0),
			mGatherStamp(0)
		{
		}

		void releaseGL()
		{
			if (mTexture)
			{
				glDeleteTextures(1, &mTexture);
				mTexture = 0;
			}
			if (mIrradiance)
			{
				glDeleteTextures(1, &mIrradiance);
				mIrradiance = 0;
			}
			if (mStaging)
			{
				glDeleteTextures(1, &mStaging);
				mStaging = 0;
			}
			if (mDummyCube)
			{
				glDeleteTextures(1, &mDummyCube);
				mDummyCube = 0;
			}
			if (mDummyIrradiance)
			{
				glDeleteTextures(1, &mDummyIrradiance);
				mDummyIrradiance = 0;
			}
			if (mCopyFBO)
			{
				glDeleteFramebuffers(1, &mCopyFBO);
				mCopyFBO = 0;
			}
			mCapture.release();
			mLit.release();
			mDown.release();
			mFilter.release();
			mIrradianceTarget.release();
			mReady = false;
			mSlots = 0;
			mProbes.clear();
		}

		void capture()
		{
			if (!LLPipeline::sRenderDeferred || gPipeline.mDeferredScreen.getWidth() < 1)
			{
				return;
			}
			S32 detail = gSavedSettings.getS32("RenderReflectionProbeDetail");
			bool probes = detail > 0 && ensure();
			if (probes)
			{
				rebuild();
			}
			gatherNetwork(probes);
			if (!probes)
			{
				return;
			}
			S32 slot = -1;
			S32 face = -1;
			if (!selectFace(slot, face))
			{
				return;
			}
			captureFace(slot, face);
		}

		void bindState(LLGLSLShader* shader, S32 unit)
		{
			if (!shader)
			{
				return;
			}
			static const LLStaticHashedString array_name("pbrProbeArray");
			static const LLStaticHashedString cube_name("pbrProbeCube");
			static const LLStaticHashedString irr_array_name("pbrIrradianceArray");
			static const LLStaticHashedString irr_cube_name("pbrIrradianceCube");
			static const LLStaticHashedString count_name("pbr_probe_count");
			static const LLStaticHashedString origin_name("pbr_probe_origin");
			static const LLStaticHashedString param_name("pbr_probe_param");
			static const LLStaticHashedString lod_name("pbr_probe_lod_max");
			static const LLStaticHashedString cam_name("pbr_cam_pos");
			static const LLStaticHashedString view_name("pbr_view_to_world");
			static const LLStaticHashedString use_name("pbr_use_array");
			shader->uniform1f(lod_name, (F32)(PROBE_MIPS - 1));
			shader->uniform1f(use_name, mArray ? 1.f : 0.f);
			LLVector3 cam = LLViewerCamera::getInstance()->getOrigin();
			shader->uniform3f(cam_name, cam.mV[0], cam.mV[1], cam.mV[2]);
			const F32* mv = glh_get_current_modelview().getF32ptr();
			F32 view_to_world[9] = { mv[0], mv[1], mv[2], mv[4], mv[5], mv[6], mv[8], mv[9], mv[10] };
			GLint view_loc = shader->getUniformLocation(view_name);
			if (view_loc >= 0)
			{
				glUniformMatrix3fvARB(view_loc, 1, GL_TRUE, view_to_world);
			}
			gPlanarReflection.bindState(shader, unit + 2);
			if (!mReady || !mTexture)
			{
				shader->uniform1f(count_name, 0.f);
				gGL.getTexUnit(0)->activate();
				return;
			}
			GLenum target = mArray ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
			gGL.getTexUnit(unit)->activate();
			glBindTexture(target, mTexture);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			shader->uniform1i(mArray ? array_name : cube_name, unit);
			if (mIrradiance)
			{
				S32 irr_unit = unit + 1;
				gGL.getTexUnit(irr_unit)->activate();
				glBindTexture(mArray ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP, mIrradiance);
				shader->uniform1i(mArray ? irr_array_name : irr_cube_name, irr_unit);
			}
			if (mArray && mDummyCube && mDummyIrradiance && unit + 4 < gGLManager.mNumTextureImageUnits)
			{
				gGL.getTexUnit(unit + 3)->activate();
				glBindTexture(GL_TEXTURE_CUBE_MAP, mDummyCube);
				shader->uniform1i(cube_name, unit + 3);
				gGL.getTexUnit(unit + 4)->activate();
				glBindTexture(GL_TEXTURE_CUBE_MAP, mDummyIrradiance);
				shader->uniform1i(irr_cube_name, unit + 4);
			}
			else if (!mArray && unit + 4 < gGLManager.mNumTextureImageUnits)
			{
				shader->uniform1i(array_name, unit + 3);
				shader->uniform1i(irr_array_name, unit + 4);
			}
			F32 origin_data[SHADER_PROBES * 4];
			F32 param_data[SHADER_PROBES * 4];
			for (S32 n = 0; n < SHADER_PROBES * 4; ++n)
			{
				origin_data[n] = 0.f;
				param_data[n] = 0.f;
			}
			S32 count = 0;
			std::vector<S32> nearest;
			nearest.reserve(mProbes.size());
			for (S32 i = 0; i < (S32)mProbes.size(); ++i)
			{
				if (mProbes[i].live && mProbes[i].filtered)
				{
					nearest.push_back(i);
				}
			}
			std::sort(nearest.begin(), nearest.end(), [&](S32 a, S32 b)
			{
				if (mProbes[a].sky != mProbes[b].sky)
				{
					return mProbes[a].sky;
				}
				return (mProbes[a].origin - cam).magVecSquared() < (mProbes[b].origin - cam).magVecSquared();
			});
			for (S32 i = 0; i < (S32)nearest.size() && count < SHADER_PROBES; ++i)
			{
				const Probe& probe = mProbes[nearest[i]];
				origin_data[count * 4 + 0] = probe.origin.mV[0];
				origin_data[count * 4 + 1] = probe.origin.mV[1];
				origin_data[count * 4 + 2] = probe.origin.mV[2];
				origin_data[count * 4 + 3] = probe.radius;
				param_data[count * 4 + 0] = (F32)nearest[i];
				param_data[count * 4 + 1] = 1.f;
				param_data[count * 4 + 2] = 0.f;
				param_data[count * 4 + 3] = 1.f;
				++count;
			}
			shader->uniform1f(count_name, (F32)count);
			GLint origin_loc = shader->getUniformLocation(origin_name);
			GLint param_loc = shader->getUniformLocation(param_name);
			if (origin_loc >= 0)
			{
				glUniform4fvARB(origin_loc, SHADER_PROBES, origin_data);
			}
			if (param_loc >= 0)
			{
				glUniform4fvARB(param_loc, SHADER_PROBES, param_data);
			}
			gGL.getTexUnit(0)->activate();
		}

	private:
		bool ensure()
		{
			bool use_array = allynPBRCubeArraySupported() && allynPBRTex3D();
			bool use_float = allynHDRColorTarget();
			if (mReady && mArray == use_array && mFloat == use_float && mTexture)
			{
				return true;
			}
			releaseGL();
			mArray = use_array;
			mFloat = use_float;
			S32 requested = llclamp(gSavedSettings.getS32("RenderReflectionProbeCount"), 1, MAX_SLOTS);
			S32 pot = 1;
			while (pot * 2 <= requested)
			{
				pot *= 2;
			}
			S32 slots = mArray ? pot : 1;
			if (mArray)
			{
				GLint layers = 0;
				glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &layers);
				if (layers >= 6)
				{
					slots = llmin(slots, layers / 6);
				}
			}
			if (slots < 1)
			{
				slots = 1;
			}
			while (slots >= 1 && !allocateTextures(slots))
			{
				if (!mArray || slots == 1)
				{
					releaseGL();
					return false;
				}
				slots /= 2;
			}
			GLenum light_format = mFloat ? GL_RGBA16F_ARB : GL_RGBA;
			if (!mCapture.allocate(CAPTURE_RES, CAPTURE_RES, GL_SRGB8_ALPHA8, TRUE, FALSE, LLTexUnit::TT_TEXTURE, FALSE)
				|| !mCapture.addColorAttachment(GL_RGBA)
				|| !mCapture.addColorAttachment(GL_RGB10_A2)
				|| !mCapture.addColorAttachment(light_format)
				|| !mLit.allocate(CAPTURE_RES, CAPTURE_RES, light_format, FALSE, FALSE, LLTexUnit::TT_TEXTURE, FALSE)
				|| !mDown.allocate(PROBE_RES, PROBE_RES, light_format, FALSE, FALSE, LLTexUnit::TT_TEXTURE, FALSE)
				|| !mFilter.allocate(PROBE_RES, PROBE_RES, light_format, FALSE, FALSE, LLTexUnit::TT_TEXTURE, FALSE)
				|| !mIrradianceTarget.allocate(IRRADIANCE_RES, IRRADIANCE_RES, light_format, FALSE, FALSE, LLTexUnit::TT_TEXTURE, FALSE))
			{
				releaseGL();
				return false;
			}
			glGenTextures(1, &mStaging);
			GLenum internal = mFloat ? GL_RGBA16F_ARB : GL_RGBA;
			GLenum type = mFloat ? GL_FLOAT : GL_UNSIGNED_BYTE;
			if (!mStaging || !allocateCube(mStaging, internal, type, PROBE_RES, 1))
			{
				releaseGL();
				return false;
			}
			if (mArray)
			{
				glGenTextures(1, &mDummyCube);
				glGenTextures(1, &mDummyIrradiance);
				if (!mDummyCube || !mDummyIrradiance
					|| !allocateCube(mDummyCube, internal, type, 1, 1)
					|| !allocateCube(mDummyIrradiance, internal, type, 1, 1))
				{
					releaseGL();
					return false;
				}
			}
			mSlots = slots;
			mProbes.assign(slots, Probe());
			mProbes[0].sky = true;
			mProbes[0].radius = PROBE_RANGE;
			mProbes[0].live = true;
			mReady = true;
			return true;
		}

		bool allocateTextures(S32 slots)
		{
			GLenum internal = mFloat ? GL_RGBA16F_ARB : GL_RGBA;
			GLenum type = mFloat ? GL_FLOAT : GL_UNSIGNED_BYTE;
			while (glGetError() != GL_NO_ERROR)
			{
			}
			glGenTextures(1, &mTexture);
			glGenTextures(1, &mIrradiance);
			bool ok = false;
			if (mArray)
			{
				ok = allocateArray(mTexture, internal, type, PROBE_RES, slots, PROBE_MIPS)
					&& allocateArray(mIrradiance, internal, type, IRRADIANCE_RES, slots, 1);
			}
			else
			{
				ok = allocateCube(mTexture, internal, type, PROBE_RES, PROBE_MIPS)
					&& allocateCube(mIrradiance, internal, type, IRRADIANCE_RES, 1);
			}
			if (!ok)
			{
				if (mTexture)
				{
					glDeleteTextures(1, &mTexture);
					mTexture = 0;
				}
				if (mIrradiance)
				{
					glDeleteTextures(1, &mIrradiance);
					mIrradiance = 0;
				}
			}
			return ok;
		}

		bool allocateCube(GLuint name, GLenum internal, GLenum type, S32 res, S32 mips)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, name);
			for (S32 mip = 0; mip < mips; ++mip)
			{
				S32 size = llmax(res >> mip, 1);
				for (S32 face = 0; face < 6; ++face)
				{
					glTexImage2D(kCubeFace[face], mip, internal, size, size, 0, GL_RGBA, type, NULL);
				}
			}
			if (glGetError() != GL_NO_ERROR)
			{
				return false;
			}
			applySampler(GL_TEXTURE_CUBE_MAP, mips);
			return true;
		}

		bool allocateArray(GLuint name, GLenum internal, GLenum type, S32 res, S32 slots, S32 mips)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, name);
			for (S32 mip = 0; mip < mips; ++mip)
			{
				S32 size = llmax(res >> mip, 1);
				if (!sTexImage3D)
				{
					return false;
				}
				sTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip, internal, size, size, slots * 6, 0, GL_RGBA, type, NULL);
			}
			if (glGetError() != GL_NO_ERROR)
			{
				return false;
			}
			applySampler(GL_TEXTURE_CUBE_MAP_ARRAY, mips);
			return true;
		}

		void applySampler(GLenum target, S32 mips)
		{
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mips > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
			glBindTexture(target, 0);
		}

		void discardImages(Probe& probe)
		{
			probe.faces = 0;
			probe.build_faces = 0;
			probe.rebuilding = false;
			probe.filtered = false;
		}

		void clearManualSlot(S32 index)
		{
			if (index < 0 || index >= mSlots)
			{
				return;
			}
			Probe& probe = mProbes[index];
			if (!probe.manual)
			{
				return;
			}
			probe.manual = false;
			probe.manual_id.setNull();
			discardImages(probe);
			probe.live = false;
			probe.radius = 0.f;
			probe.stamp = 0;
			probe.sky = false;
		}

		void restoreSkySlot()
		{
			Probe& sky = mProbes[0];
			sky.manual = false;
			sky.manual_id.setNull();
			sky.origin = LLViewerCamera::getInstance()->getOrigin() + LLVector3(0.f, 0.f, SKY_HEIGHT);
			sky.radius = PROBE_RANGE;
			discardImages(sky);
			sky.sky = true;
			sky.live = true;
			sky.touched = LLFrameTimer::getTotalSeconds();
			sky.stamp = 0;
			sky.near_clip = 0.1f;
		}

		void assignManual(S32 slot, const LLUUID& id, const LLVector3& origin, F32 radius, F32 near_clip)
		{
			Probe& probe = mProbes[slot];
			bool same = probe.manual && probe.manual_id == id;
			if (!same || (probe.origin - origin).magVec() > 1.f || fabsf(probe.radius - radius) > 0.5f)
			{
				discardImages(probe);
			}
			probe.origin = origin;
			probe.radius = radius;
			probe.near_clip = near_clip > 0.f ? near_clip : 0.1f;
			probe.manual = true;
			probe.manual_id = id;
			probe.sky = false;
			probe.live = true;
			probe.touched = LLFrameTimer::getTotalSeconds();
			probe.stamp = mGatherStamp;
		}

		S32 findManual(const LLUUID& id) const
		{
			for (S32 i = 1; i < mSlots; ++i)
			{
				if (mProbes[i].manual && mProbes[i].manual_id == id)
				{
					return i;
				}
			}
			return -1;
		}

		S32 stealSlot() const
		{
			LLVector3 cam = LLViewerCamera::getInstance()->getOrigin();
			S32 best = -1;
			F32 best_dist = -1.f;
			for (S32 i = 1; i < mSlots; ++i)
			{
				if (mProbes[i].manual)
				{
					continue;
				}
				F32 dist = mProbes[i].radius <= 0.f ? 1.0e12f : (mProbes[i].origin - cam).magVecSquared();
				if (dist > best_dist)
				{
					best_dist = dist;
					best = i;
				}
			}
			return best;
		}

		void gatherNetwork(bool place_probes)
		{
			S32 level = gSavedSettings.getS32("RenderReflectionProbeLevel");
			S32 mirror_detail = gSavedSettings.getS32("RenderMirrorDetail");
			LLVector3 cam = LLViewerCamera::getInstance()->getOrigin();
			bool have_mirror = false;
			F32 mirror_dist = 0.f;
			LLVector3 mirror_point;
			LLVector3 mirror_normal;
			LLMatrix4a mirror_box;
			struct ManualItem
			{
				LLUUID id;
				LLVector3 origin;
				F32 radius;
				F32 near_clip;
				F32 dist;
				bool placed;
			};
			std::vector<ManualItem> manuals;
			if (mirror_detail > 0 || place_probes)
			{
				for (LLViewerObjectList::vobj_list_t::const_iterator it = gObjectList.mObjects.begin(); it != gObjectList.mObjects.end(); ++it)
				{
					LLViewerObject* object = it->get();
					if (!object || object->isDead() || object->isHUDAttachment())
					{
						continue;
					}
					const LLReflectionProbeParams* params = object->getReflectionProbeParams();
					if (!params)
					{
						continue;
					}
					LLVector3 origin = object->getRenderPosition();
					LLVector3 scale = object->getScale();
					if (params->getIsMirror())
					{
						if (mirror_detail <= 0)
						{
							continue;
						}
						S32 axis = 0;
						if (scale.mV[1] < scale.mV[axis])
						{
							axis = 1;
						}
						if (scale.mV[2] < scale.mV[axis])
						{
							axis = 2;
						}
						LLVector3 normal(0.f, 0.f, 0.f);
						normal.mV[axis] = 1.f;
						normal *= object->getRenderRotation();
						if ((normal * (cam - origin)) < 0.f)
						{
							normal = -normal;
						}
						F32 half = llmax(scale.mV[axis], 0.01f) * 0.5f;
						LLVector3 point = origin + normal * half;
						if (((cam - point) * normal) <= 0.f)
						{
							continue;
						}
						F32 dist = (point - cam).magVecSquared();
						if (!have_mirror || dist < mirror_dist)
						{
							LLVector3 half_scale(llmax(scale.mV[0], 0.01f) * 0.5f, llmax(scale.mV[1], 0.01f) * 0.5f, llmax(scale.mV[2], 0.01f) * 0.5f);
							LLMatrix4 world_to_box;
							world_to_box.initAll(half_scale, object->getRenderRotation(), origin);
							world_to_box.invert();
							mirror_box.loadu(world_to_box);
							mirror_point = point;
							mirror_normal = normal;
							mirror_dist = dist;
							have_mirror = true;
						}
					}
					else if (place_probes && level >= 1)
					{
						F32 hx = llmax(scale.mV[0], 0.01f) * 0.5f;
						F32 hy = llmax(scale.mV[1], 0.01f) * 0.5f;
						F32 hz = llmax(scale.mV[2], 0.01f) * 0.5f;
						F32 radius = params->getIsBox() ? sqrtf(hx * hx + hy * hy + hz * hz) : llmax(hx, llmax(hy, hz));
						if (radius < 0.5f)
						{
							radius = 0.5f;
						}
						ManualItem item;
						item.id = object->getID();
						item.origin = origin;
						item.radius = radius;
						item.near_clip = params->getClipDistance();
						item.dist = (origin - cam).magVecSquared();
						item.placed = false;
						manuals.push_back(item);
					}
				}
			}
			if (have_mirror)
			{
				gPlanarReflection.submit(mirror_point, mirror_normal, mirror_box);
			}
			else
			{
				gPlanarReflection.clearSubmitted();
			}
			if (!place_probes || mSlots < 1)
			{
				return;
			}
			++mGatherStamp;
			if (mGatherStamp == 0)
			{
				mGatherStamp = 1;
			}
			if (level < 1 || manuals.empty())
			{
				for (S32 i = 0; i < mSlots; ++i)
				{
					clearManualSlot(i);
				}
				if (level == 1 && mArray)
				{
					mProbes[0].live = false;
				}
				else if (level != 1 && !mProbes[0].sky)
				{
					restoreSkySlot();
				}
				if (mArray && level < 3)
				{
					for (S32 i = 1; i < mSlots; ++i)
					{
						if (!mProbes[i].manual)
						{
							mProbes[i].live = false;
						}
					}
				}
				return;
			}
			if (!mArray)
			{
				if (level == 1)
				{
					S32 best = 0;
					for (S32 n = 1; n < (S32)manuals.size(); ++n)
					{
						if (manuals[n].dist < manuals[best].dist)
						{
							best = n;
						}
					}
					assignManual(0, manuals[best].id, manuals[best].origin, manuals[best].radius, manuals[best].near_clip);
				}
				else if (mProbes[0].manual)
				{
					restoreSkySlot();
				}
				return;
			}
			if (level == 1)
			{
				if (mProbes[0].manual)
				{
					clearManualSlot(0);
				}
				mProbes[0].live = false;
			}
			else if (mProbes[0].manual || !mProbes[0].sky)
			{
				restoreSkySlot();
			}
			if (level < 3)
			{
				for (S32 i = 1; i < mSlots; ++i)
				{
					if (!mProbes[i].manual)
					{
						mProbes[i].live = false;
						mProbes[i].radius = 0.f;
						discardImages(mProbes[i]);
					}
				}
			}
			for (S32 n = 0; n < (S32)manuals.size(); ++n)
			{
				S32 slot = findManual(manuals[n].id);
				if (slot > 0)
				{
					assignManual(slot, manuals[n].id, manuals[n].origin, manuals[n].radius, manuals[n].near_clip);
					manuals[n].placed = true;
				}
			}
			for (S32 i = 1; i < mSlots; ++i)
			{
				if (mProbes[i].manual && mProbes[i].stamp != mGatherStamp)
				{
					clearManualSlot(i);
				}
			}
			for (S32 n = 0; n < (S32)manuals.size(); ++n)
			{
				if (manuals[n].placed)
				{
					continue;
				}
				S32 slot = freeSlot();
				if (slot < 0)
				{
					slot = stealSlot();
				}
				if (slot > 0)
				{
					assignManual(slot, manuals[n].id, manuals[n].origin, manuals[n].radius, manuals[n].near_clip);
				}
			}
		}

		void rebuild()
		{
			F64 now = LLFrameTimer::getTotalSeconds();
			if (now - mLastScan < SCAN_PERIOD)
			{
				return;
			}
			mLastScan = now;
			LLVector3 cam = LLViewerCamera::getInstance()->getOrigin();
			S32 level = gSavedSettings.getS32("RenderReflectionProbeLevel");
			Probe& sky = mProbes[0];
			LLVector3 sky_origin = cam + LLVector3(0.f, 0.f, SKY_HEIGHT);
			if (level == 1)
			{
				if (mArray)
				{
					sky.live = false;
				}
			}
			else if (sky.manual || !sky.live || (sky.origin - sky_origin).magVec() > 32.f)
			{
				sky.manual = false;
				sky.manual_id.setNull();
				sky.origin = sky_origin;
				sky.radius = PROBE_RANGE;
				discardImages(sky);
				sky.sky = true;
				sky.live = true;
				sky.touched = now;
			}
			for (S32 i = 1; i < mSlots; ++i)
			{
				if (!mProbes[i].manual)
				{
					mProbes[i].live = false;
				}
			}
			if (level >= 3 && mArray)
			{
				S32 near_count = 0;
				for (S32 ring = 0; ring < 5; ++ring)
				{
					F32 cell = CELL_NEAR * (F32)(1 << ring);
					if (cell > CELL_MAX)
					{
						break;
					}
					LLVector3 base(floorf(cam.mV[0] / cell) * cell, floorf(cam.mV[1] / cell) * cell, floorf(cam.mV[2] / cell) * cell);
					for (S32 z = -1; z <= 1; ++z)
					{
						for (S32 y = -1; y <= 1; ++y)
						{
							for (S32 x = -1; x <= 1; ++x)
							{
								if (ring > 0 && x == 0 && y == 0 && z == 0)
								{
									continue;
								}
								LLVector3 pos = base + LLVector3((F32)x, (F32)y, (F32)z) * cell + LLVector3(cell * 0.5f, cell * 0.5f, cell * 0.5f);
								if ((pos - cam).magVec() > PROBE_RANGE)
								{
									continue;
								}
								if (ring == 0)
								{
									if (near_count >= MAX_AUTO_NEAR)
									{
										continue;
									}
									++near_count;
								}
								S32 slot = findSlot(pos, cell * 0.25f);
								if (slot < 0)
								{
									slot = freeSlot();
								}
								if (slot <= 0)
								{
									continue;
								}
								Probe& probe = mProbes[slot];
								if ((probe.origin - pos).magVec() > 1.f)
								{
									probe.origin = pos;
									discardImages(probe);
								}
								probe.radius = cell * 0.75f;
								probe.sky = false;
								probe.manual = false;
								probe.live = true;
								probe.touched = now;
							}
						}
					}
				}
			}
			for (S32 i = 1; i < mSlots; ++i)
			{
				if (!mProbes[i].manual && !mProbes[i].live && mProbes[i].radius > 0.f && now - mProbes[i].touched > RETIRE_SECONDS)
				{
					mProbes[i].radius = 0.f;
					discardImages(mProbes[i]);
				}
			}
		}

		S32 findSlot(const LLVector3& pos, F32 limit)
		{
			for (S32 i = 1; i < mSlots; ++i)
			{
				if (!mProbes[i].manual && mProbes[i].radius > 0.f && (mProbes[i].origin - pos).magVec() <= limit)
				{
					return i;
				}
			}
			return -1;
		}

		S32 freeSlot()
		{
			for (S32 i = 1; i < mSlots; ++i)
			{
				if (mProbes[i].manual)
				{
					continue;
				}
				if (mProbes[i].radius <= 0.f)
				{
					return i;
				}
			}
			return -1;
		}

		bool copyStagingBase(S32 slot)
		{
			if (!mStaging || !mTexture)
			{
				return false;
			}
			if (!mCopyFBO)
			{
				glGenFramebuffers(1, &mCopyFBO);
			}
			if (!mCopyFBO)
			{
				return false;
			}
			while (glGetError() != GL_NO_ERROR)
			{
			}
			GLint previous = 0;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
			glBindFramebuffer(GL_FRAMEBUFFER, mCopyFBO);
			GLenum store = mArray ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
			glBindTexture(store, mTexture);
			bool ok = true;
			for (S32 face = 0; face < 6; ++face)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, kCubeFace[face], mStaging, 0);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					ok = false;
					break;
				}
				if (mArray)
				{
					if (!sCopyTexSubImage3D)
					{
						ok = false;
						break;
					}
					sCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, slot * 6 + face, 0, 0, PROBE_RES, PROBE_RES);
				}
				else
				{
					glCopyTexSubImage2D(kCubeFace[face], 0, 0, 0, 0, 0, PROBE_RES, PROBE_RES);
				}
				if (glGetError() != GL_NO_ERROR)
				{
					ok = false;
					break;
				}
			}
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, kCubeFace[0], 0, 0);
			glBindTexture(store, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previous);
			return ok;
		}

		void publishSlot(S32 slot)
		{
			Probe& probe = mProbes[slot];
			if (!copyStagingBase(slot))
			{
				return;
			}
			filterSlot(slot);
			probe.faces = 0x3f;
			probe.build_faces = 0;
			probe.rebuilding = false;
			probe.filtered = true;
			probe.captured = LLFrameTimer::getTotalSeconds();
		}

		void storeFace(S32 slot, S32 face)
		{
			Probe& probe = mProbes[slot];
			if (probe.rebuilding)
			{
				probe.build_faces |= (1u << face);
				if (probe.build_faces == 0x3f)
				{
					publishSlot(slot);
				}
			}
			else
			{
				probe.faces |= (1u << face);
				if (probe.faces == 0x3f)
				{
					publishSlot(slot);
				}
			}
		}

		bool selectFace(S32& slot, S32& face)
		{
			S32 level = gSavedSettings.getS32("RenderReflectionProbeLevel");
			S32 detail = gSavedSettings.getS32("RenderReflectionProbeDetail");
			F64 now = LLFrameTimer::getTotalSeconds();
			F64 period = detail >= 2 ? 0.5 : 2.0;
			S32 limit = (mArray && level >= 1) ? mSlots : 1;
			S32 best = -1;
			S32 best_score = -1;
			for (S32 i = 0; i < limit; ++i)
			{
				Probe& probe = mProbes[i];
				if (i == 0)
				{
					if (level == 1 && !probe.manual)
					{
						probe.live = false;
						continue;
					}
					probe.live = true;
				}
				else if (!probe.live)
				{
					continue;
				}
				if ((i == 0 || probe.manual) && !probe.rebuilding && probe.faces == 0x3f && probe.filtered && now - probe.captured > period)
				{
					probe.rebuilding = true;
					probe.build_faces = 0;
				}
				U32 progress = probe.rebuilding ? probe.build_faces : probe.faces;
				S32 score = -1;
				if (progress != 0 && progress != 0x3f)
				{
					score = 2000 + (S32)progress;
				}
				else if (progress == 0)
				{
					score = 1000;
				}
				else if (probe.rebuilding || !probe.filtered)
				{
					score = 500;
				}
				if (score > best_score)
				{
					best_score = score;
					best = i;
				}
			}
			if (best < 0)
			{
				return false;
			}
			Probe& chosen = mProbes[best];
			U32 chosen_progress = chosen.rebuilding ? chosen.build_faces : chosen.faces;
			if (chosen_progress == 0x3f)
			{
				if (chosen.rebuilding || !chosen.filtered)
				{
					publishSlot(best);
				}
				return false;
			}
			slot = best;
			face = 0;
			for (S32 i = 0; i < 6; ++i)
			{
				if ((chosen_progress & (1u << i)) == 0)
				{
					face = i;
					break;
				}
			}
			return true;
		}

		void captureFace(S32 slot, S32 face)
		{
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
			BOOL hud = gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD);
			const Probe& probe = mProbes[slot];
			F32 far_clip = probe.sky ? saved_far : llmin(saved_far, llmax(probe.radius * 4.f, 32.f));
			if (!(far_clip > 0.2f))
			{
				far_clip = saved_far;
			}
			view_cam->setOriginAndLookAt(probe.origin, kFaceUp[face], probe.origin + kFaceAt[face]);
			view_cam->setView(F_PI * 0.5f);
			view_cam->setAspect(1.f);
			F32 near_clip = 0.1f;
			if (probe.near_clip > 0.05f)
			{
				near_clip = probe.near_clip;
			}
			if (near_clip > far_clip * 0.5f)
			{
				near_clip = llmax(0.1f, far_clip * 0.25f);
			}
			view_cam->setNear(near_clip);
			view_cam->setFar(far_clip);
			LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_GI_SOURCE;
			view_cam->setPerspective(FALSE, 0, 0, CAPTURE_RES, CAPTURE_RES, FALSE, near_clip, far_clip);
			glh_set_current_projection(gGLProjection);
			mCapture.bindTarget();
			glClearColor(0.f, 0.f, 0.f, 0.f);
			mCapture.clear();
			gPipeline.setCaptureGBuffer(&mCapture);
			static LLCullResult result;
			LLCamera camera(*view_cam);
			gPipeline.updateCull(camera, result);
			gPipeline.stateSort(camera, result);
			gPipeline.renderGeomDeferred(camera);
			mCapture.flush();
			gPipeline.renderDeferredLightingToRT(&mLit);
			gPipeline.setCaptureGBuffer(NULL);
			mDown.copyContents(mLit, 0, 0, CAPTURE_RES, CAPTURE_RES, 0, 0, PROBE_RES, PROBE_RES, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			mDown.bindTarget();
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, mStaging);
			glCopyTexSubImage2D(kCubeFace[face], 0, 0, 0, 0, 0, PROBE_RES, PROBE_RES);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			mDown.flush();
			storeFace(slot, face);
			if (hud != gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
			{
				gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
			}
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
		}

		void filterSlot(S32 slot)
		{
			LLGLSLShader* radiance = gRadianceGenProgram.mProgramObject ? &gRadianceGenProgram : NULL;
			if (!radiance && gReflectionMipProgram.mProgramObject)
			{
				radiance = &gReflectionMipProgram;
			}
			if (radiance)
			{
				filterProgram(*radiance, mTexture, slot, PROBE_RES, PROBE_MIPS, true);
			}
			if (gIrradianceGenProgram.mProgramObject)
			{
				filterProgram(gIrradianceGenProgram, mIrradiance, slot, IRRADIANCE_RES, 1, false);
			}
			GLenum store = mArray ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
			glBindTexture(store, mTexture);
			glTexParameteri(store, GL_TEXTURE_MAX_LEVEL, PROBE_MIPS - 1);
			glBindTexture(store, 0);
		}

		void filterProgram(LLGLSLShader& shader, GLuint destination, S32 slot, S32 res, S32 mips, bool radiance)
		{
			GLenum store = mArray ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
			for (S32 mip = radiance ? 1 : 0; mip < mips; ++mip)
			{
				S32 size = llmax(res >> mip, 1);
				F32 rough = radiance ? (F32)mip / (F32)(PROBE_MIPS - 1) : 1.f;
				for (S32 face = 0; face < 6; ++face)
				{
					mFilter.bindTarget();
					glViewport(0, 0, size, size);
					glClearColor(0.f, 0.f, 0.f, 0.f);
					glClear(GL_COLOR_BUFFER_BIT);
					gGL.matrixMode(LLRender::MM_PROJECTION);
					gGL.pushMatrix();
					gGL.loadIdentity();
					gGL.matrixMode(LLRender::MM_MODELVIEW);
					gGL.pushMatrix();
					gGL.loadIdentity();
					shader.bind();
					shader.uniform1f(LLStaticHashedString("pbr_face"), (F32)face);
					shader.uniform1f(LLStaticHashedString("pbr_roughness"), rough);
					shader.uniform1f(LLStaticHashedString("pbr_layer"), (F32)slot);
					shader.uniform1f(LLStaticHashedString("pbr_use_array"), mArray ? 1.f : 0.f);
					shader.uniform1i(LLStaticHashedString(mArray ? "pbrSourceArray" : "pbrSourceCube"), 0);
					if (mArray)
					{
						glActiveTextureARB(GL_TEXTURE0_ARB);
						glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, mTexture);
						if (mDummyCube)
						{
							shader.uniform1i(LLStaticHashedString("pbrSourceCube"), 1);
							glActiveTextureARB(GL_TEXTURE1_ARB);
							glBindTexture(GL_TEXTURE_CUBE_MAP, mDummyCube);
							glActiveTextureARB(GL_TEXTURE0_ARB);
						}
					}
					else
					{
						gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, mTexture);
						shader.uniform1i(LLStaticHashedString("pbrSourceArray"), 1);
					}
					gPipeline.drawFullScreenRect();
					shader.unbind();
					glReadBuffer(GL_COLOR_ATTACHMENT0);
					gGL.getTexUnit(0)->activate();
					glBindTexture(store, destination);
					if (mArray)
					{
						if (sCopyTexSubImage3D)
						{
							sCopyTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, radiance ? mip : 0, 0, 0, slot * 6 + face, 0, 0, size, size);
						}
					}
					else
					{
						glCopyTexSubImage2D(kCubeFace[face], radiance ? mip : 0, 0, 0, 0, 0, size, size);
					}
					glBindTexture(store, 0);
					gGL.matrixMode(LLRender::MM_PROJECTION);
					gGL.popMatrix();
					gGL.matrixMode(LLRender::MM_MODELVIEW);
					gGL.popMatrix();
					mFilter.flush();
				}
			}
			if (mArray && mDummyCube)
			{
				gGL.getTexUnit(1)->unbind(LLTexUnit::TT_CUBE_MAP);
			}
			gGL.getTexUnit(0)->activate();
		}

		GLuint mTexture;
		GLuint mIrradiance;
		GLuint mStaging;
		GLuint mDummyCube;
		GLuint mDummyIrradiance;
		GLuint mCopyFBO;
		S32 mSlots;
		bool mArray;
		bool mFloat;
		bool mReady;
		F64 mLastScan;
		U32 mGatherStamp;
		std::vector<Probe> mProbes;
		LLRenderTarget mCapture;
		LLRenderTarget mLit;
		LLRenderTarget mDown;
		LLRenderTarget mFilter;
		LLRenderTarget mIrradianceTarget;
	};

	ReflectionMaps gMaps;
}

bool allynPBRCubeArraySupported()
{
	return gGLManager.mGLSLVersionMajor >= 4;
}

bool allynHDRColorTarget()
{
	return sHDRFloat;
}

void allynSetHDRColorTarget(bool enabled)
{
	sHDRFloat = enabled;
}

void allynCaptureReflections()
{
	gMaps.capture();
	gPlanarReflection.capture();
}

void allynBindReflectionState(LLGLSLShader* shader, S32 probe_unit)
{
	gMaps.bindState(shader, probe_unit);
}

void allynReleaseReflections()
{
	gMaps.releaseGL();
	gPlanarReflection.releaseGL();
}
