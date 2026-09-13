/** 
 * @file lldrawpoolalpha.cpp
 * @brief LLDrawPoolAlpha class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "lldrawpoolalpha.h"
#include "llglheaders.h"
#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfasttimer.h"
#include "llrender.h"
#include "llcubemap.h"
#include "llsky.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llviewerregion.h"
#include "lldrawpoolwater.h"
#include "llspatialpartition.h"
BOOL LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
static BOOL deferred_render = FALSE;
LLDrawPoolAlpha::LLDrawPoolAlpha(U32 type) :
		LLRenderPass(type), current_shader(NULL), target_shader(NULL),
		simple_shader(NULL), fullbright_shader(NULL), emissive_shader(NULL),
		mColorSFactor(LLRender::BF_UNDEF), mColorDFactor(LLRender::BF_UNDEF),
		mAlphaSFactor(LLRender::BF_UNDEF), mAlphaDFactor(LLRender::BF_UNDEF)
{
}
LLDrawPoolAlpha::~LLDrawPoolAlpha()
{
}
void LLDrawPoolAlpha::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}
S32 LLDrawPoolAlpha::getNumPostDeferredPasses()
{
	static const LLCachedControl<bool> render_depth_of_field("RenderDepthOfField");
	if (LLPipeline::sImpostorRender)
	{
		return 1;
	}
	else if (render_depth_of_field)
	{
		return 2;
	}
	else
	{
		return 1;
	}
}
void LLDrawPoolAlpha::beginPostDeferredPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA);
	if (pass == 0)
	{
		if (LLPipeline::sImpostorRender)
		{
			simple_shader = &gDeferredAlphaImpostorProgram;
			fullbright_shader = &gDeferredFullbrightProgram;
		}
		else if (LLPipeline::sUnderWaterRender)
		{
			simple_shader = &gDeferredAlphaWaterProgram;
			fullbright_shader = &gDeferredFullbrightWaterProgram;
		}
		else
		{
			simple_shader = &gDeferredAlphaProgram;
			fullbright_shader = &gDeferredFullbrightProgram;
		}
		fullbright_shader->bind();
		fullbright_shader->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 2.2f);
		fullbright_shader->unbind();
		gPipeline.bindDeferredShader(*simple_shader);
		gPipeline.unbindDeferredShader(*simple_shader);
	}
	else if (!LLPipeline::sImpostorRender)
	{
		gPipeline.mScreen.flush();
		gPipeline.mDeferredDepth.copyContents(gPipeline.mDeferredScreen, 0, 0, gPipeline.mDeferredScreen.getWidth(), gPipeline.mDeferredScreen.getHeight(),
							0, 0, gPipeline.mDeferredDepth.getWidth(), gPipeline.mDeferredDepth.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		gPipeline.mDeferredDepth.bindTarget();
		simple_shader = fullbright_shader = &gObjectFullbrightProgram[1<<SHD_ALPHA_MASK_BIT];
		fullbright_shader->bind();
		fullbright_shader->setMinimumAlpha(0.33f);
	}
	llassert_always(LLPipeline::sRenderDeferred);
	emissive_shader = &gDeferredEmissiveProgram;
	deferred_render = TRUE;
	current_shader = target_shader = NULL;
	LLGLSLShader::bindNoShader();
}
void LLDrawPoolAlpha::endPostDeferredPass(S32 pass)
{
	if (current_shader)
	{
		gPipeline.unbindDeferredShader(*current_shader);
	}
	if (pass == 1 && !LLPipeline::sImpostorRender)
	{
		gPipeline.mDeferredDepth.flush();
		gPipeline.mScreen.bindTarget();
		gObjectFullbrightProgram[1<<SHD_ALPHA_MASK_BIT].unbind();
	}
	deferred_render = FALSE;
	endRenderPass(pass);
}
void LLDrawPoolAlpha::renderPostDeferred(S32 pass)
{
	render(pass);
}
void LLDrawPoolAlpha::beginRenderPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA);
	simple_shader = &gObjectSimpleProgram[1<<SHD_ALPHA_MASK_BIT | LLPipeline::sUnderWaterRender<<SHD_WATER_BIT];
	fullbright_shader = &gObjectFullbrightProgram[1<<SHD_ALPHA_MASK_BIT | LLPipeline::sUnderWaterRender<<SHD_WATER_BIT];
	emissive_shader = &gObjectEmissiveProgram[1<<SHD_ALPHA_MASK_BIT | LLPipeline::sUnderWaterRender<<SHD_WATER_BIT];
	current_shader = target_shader = NULL;
	if (mVertexShaderLevel > 0)
	{
		LLGLSLShader::bindNoShader();
	}
}
void LLDrawPoolAlpha::endRenderPass( S32 pass )
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA);
	LLRenderPass::endRenderPass(pass);
	if(mVertexShaderLevel > 0)
	{
		LLGLSLShader::bindNoShader();
	}
}
void LLDrawPoolAlpha::render(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA);
	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLGLState<GL_LIGHTING> light_state;
	gPipeline.enableLightsDynamic(light_state);
	if (deferred_render && pass == 1)
	{
		gGL.setColorMask(false, false);
	}
	else
	{
		gGL.setColorMask(true, true);
	}
	bool write_depth = (deferred_render && pass == 1)
						 || LLPipeline::sImpostorRenderAlphaDepthPass;
	LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE);
	if (deferred_render && pass == 1)
	{
		gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);
	}
	else
	{
		mColorSFactor = LLRender::BF_SOURCE_ALPHA;
		mColorDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;
		mAlphaSFactor = LLRender::BF_ZERO;
		mAlphaDFactor = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;
		gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);
		if (mVertexShaderLevel > 0)
		{
			float min_alpha = LLPipeline::sImpostorRender ? 0.5f : 0.004f;
			fullbright_shader->bind();
			fullbright_shader->setMinimumAlpha(min_alpha);
			simple_shader->bind();
			simple_shader->setMinimumAlpha(min_alpha);
			emissive_shader->bind();
			emissive_shader->setMinimumAlpha(min_alpha);
		}
		else
		{
			if (LLPipeline::sImpostorRender)
			{
				gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
			}
			else
			{
				gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
			}
		}
	}
	renderAlpha(getVertexDataMask(), pass);
	gGL.setColorMask(true, false);
	if (deferred_render && pass == 1)
	{
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
	}
	if (sShowDebugAlpha)
	{
		LLGLState<GL_LIGHTING> light_state;
		if (LLGLSLShader::sNoFixedFunction)
		{
			gHighlightProgram.bind();
		}
		else
		{
			gPipeline.enableLightsFullbright(light_state);
		}
		gGL.diffuseColor4f(0.9f,0.f,0.f,0.4f);
		LLViewerFetchedTexture::sSmokeImagep->addTextureStats(1024.f*1024.f);
		gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sSmokeImagep, TRUE) ;
		renderAlphaHighlight(LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_TEXCOORD0);
		pushBatches(LLRenderPass::PASS_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		pushBatches(LLRenderPass::PASS_ALPHA_INVISIBLE, LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0, FALSE);
		if (LLGLSLShader::sNoFixedFunction)
		{
			gHighlightProgram.unbind();
		}
		else
		{
			gPipeline.enableLightsDynamic(light_state);
		}
	}
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
}
void LLDrawPoolAlpha::renderAlphaHighlight(U32 mask)
{
	for (LLCullResult::sg_iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (group->getSpatialPartition()->mRenderByGroup &&
			!group->isDead())
		{
			LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];
			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
			{
				LLDrawInfo& params = **k;
				if (params.mParticle)
				{
					continue;
				}
				LLRenderPass::applyModelMatrix(params);
				if (params.mGroup)
				{
					params.mGroup->rebuildMesh();
				}
				params.mVertexBuffer->setBuffer(mask);
				params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
				gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
			}
		}
	}
}
void LLDrawPoolAlpha::renderAlpha(U32 mask, S32 pass)
{
	LLGLState<GL_LIGHTING> light_state;
	bool light_enabled = TRUE;
	bool use_shaders = LLGLSLShader::sNoFixedFunction;
	bool depth_only = (pass == 1 && !LLPipeline::sImpostorRender);
	for (LLCullResult::sg_iterator i = gPipeline.beginAlphaGroups(); i != gPipeline.endAlphaGroups(); ++i)
	{
		LLSpatialGroup* group = *i;
		llassert(group);
		llassert(group->getSpatialPartition());
		if (group->getSpatialPartition()->mRenderByGroup &&
		    !group->isDead())
		{
			bool is_particle_or_hud_particle = group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_PARTICLE
													  || group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_CLOUD
													  || group->getSpatialPartition()->mPartitionType == LLViewerRegion::PARTITION_HUD_PARTICLE;
			bool draw_glow_for_this_partition = !depth_only && mVertexShaderLevel > 0;
			static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_GROUP_LOOP("Alpha Group");
			LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_GROUP_LOOP);
			bool disable_cull = is_particle_or_hud_particle;
			LLGLDisable<GL_CULL_FACE> cull(disable_cull);
			LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[LLRenderPass::PASS_ALPHA];
			for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
			{
				LLDrawInfo& params = **k;
				if(depth_only)
				{
					LLFace*	face = params.mFace;
					if(face)
					{
						const LLTextureEntry* tep = face->getTextureEntry();
						if(tep)
						{
							if(tep->getColor().mV[3] < 0.1f)
								continue;
						}
					}
				}
				LLRenderPass::applyModelMatrix(params);
				if (params.mGroup)
				{
					params.mGroup->rebuildMesh();
				}
				LLMaterial* mat = (deferred_render && !LLPipeline::sRenderingHUDs) ? params.mMaterial.get() : NULL;
				if (!use_shaders)
				{
					llassert_always(!target_shader);
					llassert_always(!current_shader);
					llassert_always(!LLGLSLShader::sNoFixedFunction);
					llassert_always(!LLGLSLShader::sCurBoundShaderPtr);
					bool fullbright = depth_only || params.mFullbright;
					if(light_enabled == fullbright)
					{
						light_enabled = !fullbright;
						if (light_enabled)
						{
							gPipeline.enableLightsDynamic(light_state);
						}
						else
						{
							gPipeline.enableLightsFullbright(light_state);
						}
					}
				}
				else
				{
					if (mat)
					{
						U32 mask = params.mShaderMask;
						llassert(mask < LLMaterial::SHADER_COUNT);
						target_shader = LLPipeline::sUnderWaterRender ? &(gDeferredMaterialWaterProgram[mask]) : &(gDeferredMaterialProgram[mask]);
						if (current_shader != target_shader)
						{
							if(current_shader)
								gPipeline.unbindDeferredShader(*current_shader);
							gPipeline.bindDeferredShader(*target_shader);
						}
					}
					else
					{
						target_shader = params.mFullbright ? fullbright_shader : simple_shader;
						if (LLPipeline::sRenderingHUDs)
						{
							target_shader = fullbright_shader;
						}
						if(current_shader != target_shader)
						{
							if(current_shader)
								gPipeline.unbindDeferredShader(*current_shader);
							gPipeline.bindDeferredShader(*target_shader);
						}
					}
					current_shader = target_shader;
					if(mat)
					{
						current_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, params.mSpecColor.mV[0], params.mSpecColor.mV[1], params.mSpecColor.mV[2], params.mSpecColor.mV[3]);
						current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, params.mEnvIntensity);
						current_shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, params.mFullbright ? 1.f : 0.f);
						if (params.mNormalMap)
						{
							params.mNormalMap->addTextureStats(params.mVSize);
							current_shader->bindTexture(LLShaderMgr::BUMP_MAP, params.mNormalMap);
						}
						if (params.mSpecularMap)
						{
							params.mSpecularMap->addTextureStats(params.mVSize);
							current_shader->bindTexture(LLShaderMgr::SPECULAR_MAP, params.mSpecularMap);
						}
					}
					if (params.mTextureList.size() > 1)
					{
						for (U32 i = 0; i < params.mTextureList.size(); ++i)
						{
							if (params.mTextureList[i].notNull())
							{
								gGL.getTexUnit(i)->bind(params.mTextureList[i], TRUE);
							}
						}
					}
				}
				bool tex_setup = false;
				if(!use_shaders || params.mTextureList.size() <= 1)
				{
					if (params.mTexture.notNull())
					{
						params.mTexture->addTextureStats(params.mVSize);
						if (use_shaders && mat && current_shader)
						{
							current_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, params.mTexture);
						}
						else
						{
							gGL.getTexUnit(0)->bind(params.mTexture, TRUE);
						}
						if (params.mTextureMatrix)
						{
							tex_setup = true;
							gGL.getTexUnit(0)->activate();
							gGL.matrixMode(LLRender::MM_TEXTURE);
							gGL.loadMatrix(*params.mTextureMatrix);
							gPipeline.mTextureMatrixOps++;
						}
					}
					else
					{
						gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					}
				}
				static LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA_PUSH("Alpha Push Verts");
				{
					LL_RECORD_BLOCK_TIME(FTM_RENDER_ALPHA_PUSH);
					gGL.blendFunc((LLRender::eBlendFactor) params.mBlendFuncSrc, (LLRender::eBlendFactor) params.mBlendFuncDst, mAlphaSFactor, mAlphaDFactor);
					params.mVertexBuffer->setBuffer(current_shader ? current_shader->mAttributeMask : mask);
					params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
					gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
				}
				if (current_shader &&
					draw_glow_for_this_partition &&
					(!is_particle_or_hud_particle || params.mHasGlow) &&
					params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE))
				{
					gGL.blendFunc(LLRender::BF_ZERO, LLRender::BF_ONE,
						      LLRender::BF_ONE, LLRender::BF_ONE);
					params.mVertexBuffer->setBuffer(current_shader->mAttributeMask | LLVertexBuffer::MAP_EMISSIVE);
					params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
					gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
					gGL.blendFunc(mColorSFactor, mColorDFactor, mAlphaSFactor, mAlphaDFactor);
				}
				if (tex_setup)
				{
					gGL.getTexUnit(0)->activate();
					gGL.loadIdentity();
					gGL.matrixMode(LLRender::MM_MODELVIEW);
				}
			}
		}
	}
	if (!light_enabled)
	{
		gPipeline.enableLightsDynamic(light_state);
	}
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	LLVertexBuffer::unbind();
}
