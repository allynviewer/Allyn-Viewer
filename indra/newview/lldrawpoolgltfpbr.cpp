/**
 * @file lldrawpoolgltfpbr.cpp
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
#include "lldrawpoolgltfpbr.h"
#include "llviewershadermgr.h"
#include "llreflectionprobe.h"
#include "pipeline.h"
#include "llglslshader.h"
#include "llgltfmaterial.h"
#include "llface.h"
#include "lldrawable.h"
#include "llviewerobject.h"
#include "llviewercamera.h"
#include "llrender.h"
#include "llglstates.h"
#include "llvertexbuffer.h"
#include <algorithm>
#include <vector>

LLGLSLShader gDeferredPBRProgram(LLViewerShaderMgr::SHADER_DEFERRED);

extern LLVector3 getSunDir();

LLDrawPoolGLTFPBR::LLDrawPoolGLTFPBR(U32 type)
:	LLRenderPass(type)
{
}

LLDrawPool* LLDrawPoolGLTFPBR::instancePool()
{
	return new LLDrawPoolGLTFPBR(mType);
}

U32 LLDrawPoolGLTFPBR::getVertexDataMask()
{
	return LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TANGENT;
}

void LLDrawPoolGLTFPBR::prerender()
{
}

S32 LLDrawPoolGLTFPBR::getNumPasses()
{
	return gDeferredPBRProgram.mProgramObject ? 1 : 0;
}

void LLDrawPoolGLTFPBR::render(S32)
{
	if (LLPipeline::sRenderDeferred || !gDeferredPBRProgram.mProgramObject)
	{
		return;
	}
	if (mType == LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK)
	{
		drawGeometry(LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK, 1.f);
		return;
	}
	drawGeometry(LLRenderPass::PASS_GLTF_PBR, 1.f);
	drawGeometry(LLRenderPass::PASS_GLTF_PBR_BLEND, 1.f);
}

S32 LLDrawPoolGLTFPBR::getNumDeferredPasses()
{
	return gDeferredPBRProgram.mProgramObject ? 1 : 0;
}

void LLDrawPoolGLTFPBR::renderDeferred(S32)
{
	if (!gDeferredPBRProgram.mProgramObject)
	{
		return;
	}
	drawGeometry(mType == LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK ? LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK : LLRenderPass::PASS_GLTF_PBR, 0.f);
}

S32 LLDrawPoolGLTFPBR::getNumPostDeferredPasses()
{
	return 0;
}

void LLDrawPoolGLTFPBR::renderPostDeferred(S32)
{
}

S32 LLDrawPoolGLTFPBR::getNumShadowPasses()
{
	return gDeferredPBRProgram.mProgramObject ? 1 : 0;
}

void LLDrawPoolGLTFPBR::renderShadow(S32)
{
	if (mType == LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK)
	{
		pushMaskBatches(LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK, getVertexDataMask(), TRUE, FALSE);
	}
	else
	{
		pushBatches(LLRenderPass::PASS_GLTF_PBR, getVertexDataMask(), TRUE, FALSE);
	}
}

void LLDrawPoolGLTFPBR::drawGeometry(U32 pass, F32 output_mode)
{
	if (!gDeferredPBRProgram.mProgramObject)
	{
		return;
	}
	gDeferredPBRProgram.bind();
	gDeferredPBRProgram.uniform1f(LLStaticHashedString("pbr_output_mode"), output_mode);
	allynBindReflectionState(&gDeferredPBRProgram, 4);
	LLVector3 sun = getSunDir();
	LLVector4a sun_v;
	sun_v.load3(sun.mV);
	LLVector4a sun_view;
	glh_get_current_modelview().rotate(sun_v, sun_view);
	gDeferredPBRProgram.uniform3fv(LLStaticHashedString("pbr_sun_dir"), 1, sun_view.getF32ptr());
	F32 elev = llclamp(sun.mV[2], 0.f, 1.f);
	gDeferredPBRProgram.uniform3f(LLStaticHashedString("pbr_sun_color"), elev, elev * 0.96f, elev * 0.9f);
	gDeferredPBRProgram.uniform1f(LLStaticHashedString("pbr_is_hud"), LLPipeline::sRenderingHUDs ? 1.f : 0.f);
	U32 mask = getVertexDataMask();
	if (pass == LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK)
	{
		pushMaskBatches(pass, mask, TRUE, FALSE);
	}
	else
	{
		for (LLCullResult::drawinfo_iterator i = gPipeline.beginRenderMap(pass); i != gPipeline.endRenderMap(pass); ++i)
		{
			LLDrawInfo* params = *i;
			if (!params)
			{
				continue;
			}
			bool two = false;
			if (params->mFace && params->mFace->getTextureEntry())
			{
				LLGLTFMaterial* mat = params->mFace->getTextureEntry()->getGLTFMaterial();
				two = mat && mat->mReady && mat->mDoubleSided;
			}
			if (two)
			{
				LLGLDisable<GL_CULL_FACE> nocull;
				pushBatch(*params, mask, TRUE, FALSE);
			}
			else
			{
				pushBatch(*params, mask, TRUE, FALSE);
			}
		}
	}
	gDeferredPBRProgram.unbind();
}
