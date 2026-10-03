/** 
 * @file lldrawpool.cpp
 * @brief LLDrawPoolMaterials class implementation
 * @author Jonathan "Geenz" Goodman
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "lldrawpoolmaterials.h"
#include "llviewershadermgr.h"
#include "pipeline.h"
#include "llgltfmateriallist.h"
#include "llface.h"
S32 diffuse_channel = -1;
LLDrawPoolMaterials::LLDrawPoolMaterials()
:  LLRenderPass(LLDrawPool::POOL_MATERIALS)
{
}
void LLDrawPoolMaterials::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}
S32 LLDrawPoolMaterials::getNumDeferredPasses()
{
	return 12;
}
void LLDrawPoolMaterials::beginDeferredPass(S32 pass)
{
	U32 shader_idx[] =
	{
		0,
		2,
		3,
		4,
		6,
		7,
		8,
		10,
		11,
		12,
		14,
		15,
	};
	mShader = &(gDeferredMaterialProgram[shader_idx[pass]]);
	if (LLPipeline::sUnderWaterRender)
	{
		mShader = &(gDeferredMaterialWaterProgram[shader_idx[pass]]);
	}
	mShader->bind();
	diffuse_channel = mShader->enableTexture(LLShaderMgr::DIFFUSE_MAP);
	LL_RECORD_BLOCK_TIME(FTM_RENDER_MATERIALS);
}
void LLDrawPoolMaterials::endDeferredPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_MATERIALS);
	mShader->unbind();
	LLRenderPass::endRenderPass(pass);
}
void LLDrawPoolMaterials::renderDeferred(S32 pass)
{
	U32 type_list[] =
	{
		LLRenderPass::PASS_MATERIAL,
		LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
		LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
		LLRenderPass::PASS_SPECMAP,
		LLRenderPass::PASS_SPECMAP_MASK,
		LLRenderPass::PASS_SPECMAP_EMISSIVE,
		LLRenderPass::PASS_NORMMAP,
		LLRenderPass::PASS_NORMMAP_MASK,
		LLRenderPass::PASS_NORMMAP_EMISSIVE,
		LLRenderPass::PASS_NORMSPEC,
		LLRenderPass::PASS_NORMSPEC_MASK,
		LLRenderPass::PASS_NORMSPEC_EMISSIVE,
	};
	llassert(pass < sizeof(type_list)/sizeof(U32));
	U32 type = type_list[pass];
	U32 mask = mShader->mAttributeMask;
	LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type);
	LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type);
	for (LLCullResult::drawinfo_iterator i = begin; i != end; ++i)
	{
		LLDrawInfo& params = **i;
		LLColor4 spec(params.mSpecColor.mV[0], params.mSpecColor.mV[1], params.mSpecColor.mV[2], params.mSpecColor.mV[3]);
		F32 env = params.mEnvIntensity;
		F32 emissive = params.mFullbright ? 1.f : 0.f;
		const LLTextureEntry* te = (params.mFace && params.mFace->getViewerObject()) ? params.mFace->getTextureEntry() : NULL;
		LLPBRGraphics::apply(te, spec, env, emissive);
		LLPBRGraphics::bindDraw(mShader, params.mFace);
		mShader->uniform4f(LLShaderMgr::SPECULAR_COLOR, spec.mV[0], spec.mV[1], spec.mV[2], spec.mV[3]);
		mShader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, env);
		if (params.mNormalMap)
		{
			params.mNormalMap->addTextureStats(params.mVSize);
			bindNormalMap(params.mNormalMap);
		}
		if (params.mSpecularMap)
		{
			params.mSpecularMap->addTextureStats(params.mVSize);
			bindSpecularMap(params.mSpecularMap);
		}
		mShader->setMinimumAlpha(params.mAlphaMaskCutoff);
		mShader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, emissive);
		pushBatch(params, mask, TRUE);
	}
}
void LLDrawPoolMaterials::bindSpecularMap(LLViewerTexture* tex)
{
	mShader->bindTexture(LLShaderMgr::SPECULAR_MAP, tex);
}
void LLDrawPoolMaterials::bindNormalMap(LLViewerTexture* tex)
{
	mShader->bindTexture(LLShaderMgr::BUMP_MAP, tex);
}
void LLDrawPoolMaterials::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture, BOOL batch_textures)
{
	if (mShader)
	{
		LLPBRGraphics::bindDraw(mShader, params.mFace);
	}
	applyModelMatrix(params);
	bool tex_setup = false;
	if (batch_textures && params.mTextureList.size() > 1)
	{
		for (U32 i = 0; i < params.mTextureList.size(); ++i)
		{
			if (params.mTextureList[i].notNull())
			{
				gGL.getTexUnit(i)->bind(params.mTextureList[i], TRUE);
			}
		}
	}
	else
	{
		if (params.mTextureMatrix)
		{
			{
				gGL.getTexUnit(0)->activate();
				gGL.matrixMode(LLRender::MM_TEXTURE);
			}
			gGL.loadMatrix(*params.mTextureMatrix);
			gPipeline.mTextureMatrixOps++;
			tex_setup = true;
		}
		if (mVertexShaderLevel > 1 && texture)
		{
			if (params.mTexture.notNull())
			{
				gGL.getTexUnit(diffuse_channel)->bind(params.mTexture);
				params.mTexture->addTextureStats(params.mVSize);
			}
			else
			{
				gGL.getTexUnit(diffuse_channel)->unbind(LLTexUnit::TT_TEXTURE);
			}
		}
	}
	if (params.mGroup)
	{
		params.mGroup->rebuildMesh();
	}
	params.mVertexBuffer->setBuffer(mask);
	params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
	gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
	if (tex_setup)
	{
		gGL.getTexUnit(0)->activate();
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}
