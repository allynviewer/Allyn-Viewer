/** 
 * @file lldynamictexture.cpp
 * @brief Implementation of LLViewerDynamicTexture class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"
#include "lldynamictexture.h"
#include "llglheaders.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llvertexbuffer.h"
#include "llviewerdisplay.h"
#include "llrender.h"
#include "pipeline.h"
#include "llglslshader.h"
LLViewerDynamicTexture::instance_list_t LLViewerDynamicTexture::sInstances[ LLViewerDynamicTexture::ORDER_COUNT ];
S32 LLViewerDynamicTexture::sNumRenders = 0;
LLViewerDynamicTexture::LLViewerDynamicTexture(S32 width, S32 height, S32 components, EOrder order, BOOL clamp) :
	LLViewerTexture(width, height, components, FALSE, false),
	mClamp(clamp)
{
	llassert((1 <= components) && (components <= 4));
	if(gGLManager.mDebugGPU)
	{
		if(components == 3)
		{
			mComponents = 4 ;
		}
	}
	generateGLTexture();
	llassert( 0 <= order && order < ORDER_COUNT );
	LLViewerDynamicTexture::sInstances[ order ].insert(this);
}
LLViewerDynamicTexture::~LLViewerDynamicTexture()
{
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		LLViewerDynamicTexture::sInstances[order].erase(this);
	}
}
S8 LLViewerDynamicTexture::getType() const
{
	return LLViewerTexture::DYNAMIC_TEXTURE ;
}
void LLViewerDynamicTexture::generateGLTexture()
{
	LLViewerTexture::generateGLTexture() ;
	generateGLTexture(-1, 0, 0, FALSE);
}
void LLViewerDynamicTexture::generateGLTexture(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes, const LLColor4U *fill_color)
{
	if (mComponents < 1 || mComponents > 4)
	{
		LL_ERRS() << "Bad number of components in dynamic texture: " << mComponents << LL_ENDL;
	}
	LLPointer<LLImageRaw> raw_image = new LLImageRaw(mFullWidth, mFullHeight, mComponents);
	if (internal_format >= 0)
	{
		setExplicitFormat(internal_format, primary_format, type_format, swap_bytes);
	}
	if(fill_color)
		raw_image->fill(*fill_color);
	createGLTexture(0, raw_image, nullptr, TRUE, LLViewerTexture::DYNAMIC_TEX);
	setAddressMode((mClamp) ? LLTexUnit::TAM_CLAMP : LLTexUnit::TAM_WRAP);
	mGLTexturep->setGLTextureCreated(false);
}
BOOL LLViewerDynamicTexture::render()
{
	return FALSE;
}
void LLViewerDynamicTexture::preRender(BOOL clear_depth)
{
	llassert(mFullHeight <= 512);
	llassert(mFullWidth <= 512);
	if (gGLManager.mHasFramebufferObject && gPipeline.mWaterDis.isComplete() && !gGLManager.mIsATI)
	{
		mOrigin.set(0, 0);
	}
	else
	{
		LLCoordScreen window_pos;
		gViewerWindow->getWindow()->getPosition( &window_pos );
		mOrigin.set(0, gViewerWindow->getWindowHeightRaw() - mFullHeight);
		if (window_pos.mX < 0)
		{
			mOrigin.mX = -window_pos.mX;
		}
		if (window_pos.mY < 0)
		{
			mOrigin.mY += window_pos.mY;
			mOrigin.mY = llmax(mOrigin.mY, 0) ;
		}
	}
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	mCamera.setOrigin(*camera);
	mCamera.setAxes(*camera);
	mCamera.setAspect(camera->getAspect());
	mCamera.setView(camera->getView());
	mCamera.setNear(camera->getNear());
	gGL.setViewport(mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight);
	if (clear_depth)
	{
		gGL.syncContextState();
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}
void LLViewerDynamicTexture::postRender(BOOL success)
{
	{
		if (success)
		{
			if(mGLTexturep.isNull())
			{
				generateGLTexture() ;
			}
			else if(!mGLTexturep->getHasGLTexture())
			{
				generateGLTexture() ;
			}
			else if(mGLTexturep->getDiscardLevel() != 0)
			{
				generateGLTexture() ;
			}
			success = mGLTexturep->setSubImageFromFrameBuffer(0, 0, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight);
		}
	}
	gViewerWindow->setup2DViewport();
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	camera->setOrigin(mCamera);
	camera->setAxes(mCamera);
	camera->setAspect(mCamera.getAspect());
	camera->setView(mCamera.getView());
	camera->setNear(mCamera.getNear());
}
BOOL LLViewerDynamicTexture::updateAllInstances()
{
	sNumRenders = 0;
	if (gGLManager.mIsDisabled || LLPipeline::sMemAllocationThrottled)
	{
		return TRUE;
	}
	bool use_fbo = gGLManager.mHasFramebufferObject && gPipeline.mWaterDis.isComplete() && !gGLManager.mIsATI;
	if (use_fbo)
	{
		gPipeline.mWaterDis.bindTarget();
	}
	LLGLSLShader::bindNoShader();
	BOOL result = FALSE;
	BOOL ret = FALSE ;
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
			 iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLViewerDynamicTexture *dynamicTexture = *iter;
			if (dynamicTexture->needsRender())
			{
				gGL.syncContextState();
				glClear(GL_DEPTH_BUFFER_BIT);
				gDepthDirty = TRUE;
				gGL.color4f(1,1,1,1);
				dynamicTexture->preRender();
				result = FALSE;
				if (dynamicTexture->render())
				{
					ret = TRUE ;
					result = TRUE;
					sNumRenders++;
				}
				gGL.flush();
				LLVertexBuffer::unbind();
				dynamicTexture->postRender(result);
			}
		}
	}
	if (use_fbo)
	{
		gPipeline.mWaterDis.flush();
	}
	return ret;
}
void LLViewerDynamicTexture::destroyGL()
{
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
			 iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLViewerDynamicTexture *dynamicTexture = *iter;
			dynamicTexture->destroyGLTexture() ;
		}
	}
}
void LLViewerDynamicTexture::restoreGL()
{
	if (gGLManager.mIsDisabled)
	{
		return ;
	}
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		for (instance_list_t::iterator iter = LLViewerDynamicTexture::sInstances[order].begin();
			 iter != LLViewerDynamicTexture::sInstances[order].end(); ++iter)
		{
			LLViewerDynamicTexture *dynamicTexture = *iter;
			dynamicTexture->restoreGLTexture() ;
		}
	}
}
