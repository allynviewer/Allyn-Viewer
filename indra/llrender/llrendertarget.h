/** 
 * @file llrendertarget.h
 * @brief Off screen render target abstraction.  Loose wrapper for GL_EXT_framebuffer_objects.
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
#ifndef LL_LLRENDERTARGET_H
#define LL_LLRENDERTARGET_H
#if !LL_MESA_HEADLESS
#include "llgl.h"
#include "llrender.h"
class LLMultisampleBuffer;
class LLRenderTarget
{
public:
	static bool sUseFBO;
	static U32 sBytesAllocated;
	static LLRenderTarget* sCurFBO;
	LLRenderTarget();
	virtual ~LLRenderTarget();
	bool allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil, LLTexUnit::eTextureType usage = LLTexUnit::TT_TEXTURE, bool use_fbo = FALSE);
	void resize(U32 resx, U32 resy);
	void setSampleBuffer(LLMultisampleBuffer* buffer);
	virtual bool addColorAttachment(U32 color_fmt);
	virtual bool allocateDepth();
	virtual void shareDepthBuffer(LLRenderTarget& target);
	virtual void release();
	virtual void bindTarget();
	void clear(U32 mask = 0xFFFFFFFF);
	void getViewport(LLRect& viewport);
	U32 getWidth() const { return mResX; }
	U32 getHeight() const { return mResY; }
	LLTexUnit::eTextureType getUsage(void) const { return mUsage; }
	U32 getTexture(U32 attachment = 0) const;
	U32 getDepth(void) const { return mDepth; }
	bool hasStencil() const { return mStencil; }
	void bindTexture(U32 index, S32 channel);
	void flush(bool fetch_depth = FALSE);
	void copyContents(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter);
	static void copyContentsToFramebuffer(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter);
	bool isComplete() const;
	U32 getFBO() const {return mFBO;}
	static LLRenderTarget* getCurrentBoundTarget() { return sBoundTarget; }
protected:
	friend class LLMultisampleBuffer;
	U32 mResX;
	U32 mResY;
	std::vector<U32> mTex;
	std::vector<LLImageGL::GLTextureName> mTexName;
	std::vector<U32> mInternalFormat;
	U32 mFBO;
	LLRenderTarget* mPreviousFBO;
	LLImageGL::GLTextureName mDepthName;
	U32 mDepth;
	bool mStencil;
	bool mUseDepth;
	bool mRenderDepth;
	LLTexUnit::eTextureType mUsage;
	LLMultisampleBuffer* mSampleBuffer;
	static LLRenderTarget* sBoundTarget;
};
class LLMultisampleBuffer : public LLRenderTarget
{
	U32 mSamples;
	U32 mColorFormat;
public:
	LLMultisampleBuffer();
	virtual ~LLMultisampleBuffer();
	virtual void release();
	virtual void bindTarget();
	void bindTarget(LLRenderTarget* ref);
	virtual bool allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil, LLTexUnit::eTextureType usage, bool use_fbo);
	bool allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil, LLTexUnit::eTextureType usage, bool use_fbo, U32 samples);
	virtual bool addColorAttachment(U32 color_fmt);
	virtual bool allocateDepth();
	void resize(U32 resx, U32 resy);
};
#endif
#endif
