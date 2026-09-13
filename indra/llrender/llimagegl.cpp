/** 
 * @file llimagegl.cpp
 * @brief Generic GL image handler
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
#include "linden_common.h"
#include "llimagegl.h"
#include "llerror.h"
#include "llimage.h"
#include "llmath.h"
#include "llgl.h"
#include "llglslshader.h"
#include "llrender.h"
const F32 MIN_TEXTURE_LIFETIME = 10.f;
U32 wpo2(U32 i);
U32 LLImageGL::sUniqueCount				= 0;
U32 LLImageGL::sBindCount				= 0;
S64Bytes LLImageGL::sGlobalTextureMemory(0);
S64Bytes LLImageGL::sBoundTextureMemory(0);
S64Bytes LLImageGL::sCurBoundTextureMemory(0);
S32 LLImageGL::sCount					= 0;
BOOL LLImageGL::sGlobalUseAnisotropic	= FALSE;
F32 LLImageGL::sLastFrameTime			= 0.f;
BOOL LLImageGL::sAllowReadBackRaw       = FALSE ;
LLImageGL* LLImageGL::sDefaultGLTexture = NULL ;
bool LLImageGL::sCompressTextures = false;
std::set<LLImageGL*> LLImageGL::sImageList;
BOOL gAuditTexture = FALSE ;
#define MAX_TEXTURE_LOG_SIZE 22
std::vector<S32> LLImageGL::sTextureLoadedCounter(MAX_TEXTURE_LOG_SIZE + 1) ;
std::vector<S32> LLImageGL::sTextureBoundCounter(MAX_TEXTURE_LOG_SIZE + 1) ;
std::vector<S32> LLImageGL::sTextureCurBoundCounter(MAX_TEXTURE_LOG_SIZE + 1) ;
S32 LLImageGL::sCurTexSizeBar = -1 ;
S32 LLImageGL::sCurTexPickSize = -1 ;
LLPointer<LLImageGL> LLImageGL::sHighlightTexturep = NULL;
S32 LLImageGL::sMaxCategories = 1 ;
std::vector<S64Bytes> LLImageGL::sTextureMemByCategory;
std::vector<S64Bytes> LLImageGL::sTextureMemByCategoryBound ;
std::vector<S64Bytes> LLImageGL::sTextureCurMemByCategoryBound ;
void check_all_images()
{
	for (std::set<LLImageGL*>::iterator iter = LLImageGL::sImageList.begin();
		 iter != LLImageGL::sImageList.end(); iter++)
	{
		LLImageGL* glimage = *iter;
		if (glimage->getTexName() && glimage->isGLTextureCreated())
		{
			gGL.getTexUnit(0)->bind(glimage) ;
			glimage->checkTexSize() ;
			gGL.getTexUnit(0)->unbind(glimage->getTarget()) ;
		}
	}
}
void LLImageGL::checkTexSize(bool forced) const
{
	if ((forced || gDebugGL) && mTarget == GL_TEXTURE_2D)
	{
		{
			GLint vp[4] ;
			glGetIntegerv(GL_VIEWPORT, vp) ;
			llcallstacks << "viewport: " << vp[0] << " : " << vp[1] << " : " << vp[2] << " : " << vp[3] << llcallstacksendl ;
		}
		GLint texname;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &texname);
		BOOL error = FALSE;
		if (texname != getTexName())
		{
			LL_INFOS() << "Bound: " << texname << " Should bind: " << getTexName() << " Default: " << (LLImageGL::sDefaultGLTexture ? LLImageGL::sDefaultGLTexture->getTexName() : 0) << LL_ENDL;
			error = TRUE;
			if (gDebugSession)
			{
				gFailLog << "Invalid texture bound!" << std::endl;
			}
			else
			{
				LL_ERRS() << "Invalid texture bound!" << LL_ENDL;
			}
		}
		stop_glerror() ;
		LLGLint x = 0, y = 0 ;
		glGetTexLevelParameteriv(mTarget, 0, GL_TEXTURE_WIDTH, (GLint*)&x);
		glGetTexLevelParameteriv(mTarget, 0, GL_TEXTURE_HEIGHT, (GLint*)&y) ;
		stop_glerror() ;
		llcallstacks << "w: " << x << " h: " << y << llcallstacksendl ;
		if(!x || !y)
		{
			return ;
		}
		if(x != (mWidth >> mCurrentDiscardLevel) || y != (mHeight >> mCurrentDiscardLevel))
		{
			error = TRUE;
			if (gDebugSession)
			{
				gFailLog << "wrong texture size and discard level!" <<
					mWidth << " Height: " << mHeight << " Current Level: " << (S32)mCurrentDiscardLevel << std::endl;
			}
			else
			{
				LL_ERRS() << "wrong texture size and discard level: width: " <<
					mWidth << " Height: " << mHeight << " Current Level: " << (S32)mCurrentDiscardLevel << LL_ENDL ;
			}
		}
		if (error)
		{
			ll_fail("LLImageGL::checkTexSize failed.");
		}
	}
}
BOOL is_little_endian()
{
	S32 a = 0x12345678;
    U8 *c = (U8*)(&a);
	return (*c == 0x78) ;
}
void LLImageGL::initClass(S32 num_catagories)
{
	sMaxCategories = num_catagories ;
	sTextureMemByCategory.resize(sMaxCategories);
	sTextureMemByCategoryBound.resize(sMaxCategories) ;
	sTextureCurMemByCategoryBound.resize(sMaxCategories) ;
}
void LLImageGL::cleanupClass()
{
	sTextureMemByCategory.clear() ;
	sTextureMemByCategoryBound.clear() ;
	sTextureCurMemByCategoryBound.clear() ;
}
void LLImageGL::setHighlightTexture(S32 category)
{
	const S32 dim = 128;
	sHighlightTexturep = new LLImageGL() ;
	LLPointer<LLImageRaw> image_raw = new LLImageRaw(dim,dim,3);
	U8* data = image_raw->getData();
	for (S32 i = 0; i<dim; i++)
	{
		for (S32 j = 0; j<dim; j++)
		{
			const S32 border = 2;
			if (i<border || j<border || i>=(dim-border) || j>=(dim-border))
			{
				*data++ = 0xff;
				*data++ = 0xff;
				*data++ = 0xff;
			}
			else
			{
				*data++ = 0xff;
				*data++ = 0xff;
				*data++ = 0x00;
			}
		}
	}
	sHighlightTexturep->createGLTexture(0, image_raw, nullptr, TRUE, category);
	image_raw = NULL;
}
S32 LLImageGL::dataFormatBits(S32 dataformat)
{
	switch (dataformat)
	{
	  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	return 4;
	  case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	return 8;
	  case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	return 8;
	  case GL_LUMINANCE:						return 8;
	  case GL_ALPHA:							return 8;
	  case GL_COLOR_INDEX:						return 8;
	  case GL_LUMINANCE_ALPHA:					return 16;
	  case GL_RED:								return 8;
	  case GL_RG:								return 16;
	  case GL_RGB:								return 24;
	  case GL_RGB8:								return 24;
	  case GL_RGBA:								return 32;
	  case GL_BGRA:								return 32;
	  default:
		LL_ERRS() << "LLImageGL::Unknown format: " << dataformat << LL_ENDL;
		return 0;
	}
}
S32 LLImageGL::dataFormatBytes(S32 dataformat, S32 width, S32 height)
{
	if (dataformat >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
		dataformat <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		if (width < 4) width = 4;
		if (height < 4) height = 4;
	}
	S32 bytes ((width*height*dataFormatBits(dataformat)+7)>>3);
	S32 aligned = (bytes+3)&~3;
	return aligned;
}
S32 LLImageGL::dataFormatComponents(S32 dataformat)
{
	switch (dataformat)
	{
	  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	return 3;
	  case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	return 4;
	  case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	return 4;
	  case GL_LUMINANCE:						return 1;
	  case GL_ALPHA:							return 1;
	  case GL_COLOR_INDEX:						return 1;
	  case GL_LUMINANCE_ALPHA:					return 2;
	  case GL_RED:								return 1;
	  case GL_RG:								return 2;
	  case GL_RGB:								return 3;
	  case GL_RGBA:								return 4;
	  case GL_BGRA:								return 4;
	  default:
		LL_ERRS() << "LLImageGL::Unknown format: " << dataformat << LL_ENDL;
		return 0;
	}
}
static LLTrace::BlockTimerStatHandle FTM_IMAGE_UPDATE_STATS("Image Stats");
void LLImageGL::updateStats(F32 current_time)
{
	LL_RECORD_BLOCK_TIME(FTM_IMAGE_UPDATE_STATS);
	sLastFrameTime = current_time;
	sBoundTextureMemory = sCurBoundTextureMemory;
	sCurBoundTextureMemory = S64Bytes(0);
	if(gAuditTexture)
	{
		for(U32 i = 0 ; i < sTextureCurBoundCounter.size() ; i++)
		{
			sTextureBoundCounter[i] = sTextureCurBoundCounter[i] ;
			sTextureCurBoundCounter[i] = 0 ;
		}
		for(U32 i = 0 ; i < sTextureCurMemByCategoryBound.size() ; i++)
		{
			sTextureMemByCategoryBound[i] = sTextureCurMemByCategoryBound[i] ;
			sTextureCurMemByCategoryBound[i] = (S64Bytes)0 ;
		}
	}
}
S32 LLImageGL::updateBoundTexMem(const S32Bytes mem, const S32 ncomponents, S32 category)
{
	if(gAuditTexture && ncomponents > 0 && category > -1)
	{
		sTextureCurBoundCounter[getTextureCounterIndex(mem.value() / ncomponents)]++ ;
		sTextureCurMemByCategoryBound[category] += mem ;
	}
	LLImageGL::sCurBoundTextureMemory += mem ;
	return LLImageGL::sCurBoundTextureMemory.value();
}
void LLImageGL::destroyGL(BOOL save_state)
{
	LLTexUnit::sWhiteTexture = 0;
	if (save_state)
	{
		U32 count = 0;
		sAllowReadBackRaw = true;
		for (std::set<LLImageGL*>::iterator iter = sImageList.begin();
			iter != sImageList.end(); iter++)
		{
			LLImageGL* glimage = *iter;
			GLuint tex = glimage->getTexName();
			if (tex)
			{
				if (glimage->isGLTextureCreated())
				{
					glimage->mSaveData = new LLImageRaw;
					if (glimage->mComponents && glimage->readBackRaw(-1, glimage->mSaveData, false))
					{
						++count;
						glimage->mSaveDiscardLevel = glimage->mCurrentDiscardLevel;
						glimage->destroyGLTexture();
						continue;
					}
				}
			}
			glimage->mSaveData = nullptr;
			glimage->forceToInvalidateGLTexture();
			stop_glerror();
		}
		LL_INFOS() << "Storing " << count << " images..." << LL_ENDL;
		sAllowReadBackRaw = false;
	}
	for (S32 stage = 0; stage < gGLManager.mNumTextureUnits; stage++)
	{
		gGL.getTexUnit(stage)->unbind(LLTexUnit::TT_TEXTURE);
	}
}
void LLImageGL::restoreGL()
{
	U32 count = 0;
	for (std::set<LLImageGL*>::iterator iter = sImageList.begin();
		 iter != sImageList.end(); iter++)
	{
		LLImageGL* glimage = *iter;
		if(glimage->getTexName())
		{
			LL_ERRS() << "tex name is not 0." << LL_ENDL ;
		}
		LLPointer<LLImageRaw> data = glimage->mSaveData;
		glimage->mSaveData = nullptr;
		if (data.notNull() && glimage->getComponents() &&
			data->getComponents() &&
			glimage->mSaveDiscardLevel >= 0 &&
			glimage->createGLTexture(glimage->mSaveDiscardLevel, data, nullptr, TRUE, glimage->getCategory()))
		{
				stop_glerror();
				++count;
		}
		else
		{
			glimage->forceToInvalidateGLTexture();
		}
	}
	LL_INFOS() << "Restored " << count << " images" << LL_ENDL;
}
void LLImageGL::dirtyTexOptions()
{
	for (std::set<LLImageGL*>::iterator iter = sImageList.begin();
		 iter != sImageList.end(); iter++)
	{
		LLImageGL* glimage = *iter;
		glimage->mTexOptionsDirty = true;
		stop_glerror();
	}
}
BOOL LLImageGL::create(LLPointer<LLImageGL>& dest, BOOL usemipmaps)
{
	dest = new LLImageGL(usemipmaps);
	return TRUE;
}
BOOL LLImageGL::create(LLPointer<LLImageGL>& dest, U32 width, U32 height, U8 components, BOOL usemipmaps)
{
	dest = new LLImageGL(width, height, components, usemipmaps);
	return TRUE;
}
BOOL LLImageGL::create(LLPointer<LLImageGL>& dest, const LLImageRaw* imageraw, BOOL usemipmaps)
{
	dest = new LLImageGL(imageraw, usemipmaps);
	return TRUE;
}
LLImageGL::LLImageGL(BOOL usemipmaps, bool allow_compression)
	: mSaveData(0), mSaveDiscardLevel(-1), mIsCompressed(false)
{
	init(usemipmaps, allow_compression);
	setSize(0, 0, 0);
	sImageList.insert(this);
	sCount++;
}
LLImageGL::LLImageGL(U32 width, U32 height, U8 components, BOOL usemipmaps, bool allow_compression)
	: mSaveData(0), mSaveDiscardLevel(-1)
{
	llassert( components <= 4 );
	init(usemipmaps, allow_compression);
	setSize(width, height, components);
	sImageList.insert(this);
	sCount++;
}
LLImageGL::LLImageGL(const LLImageRaw* imageraw, BOOL usemipmaps, bool allow_compression)
	: mSaveData(0), mSaveDiscardLevel(-1)
{
	init(usemipmaps, allow_compression);
	setSize(0, 0, 0);
	sImageList.insert(this);
	sCount++;
	createGLTexture(0, imageraw);
}
LLImageGL::~LLImageGL()
{
	LLImageGL::cleanup();
	sImageList.erase(this);
	delete [] mPickMask;
	mPickMask = NULL;
	sCount--;
}
const S8 INVALID_OFFSET = -99 ;
void LLImageGL::init(BOOL usemipmaps, bool allow_compression)
{
	mTextureMemory = (S32Bytes)0;
	mLastBindTime = 0.f;
	mPickMask = NULL;
	mPickMaskWidth = 0;
	mPickMaskHeight = 0;
	mUseMipMaps = usemipmaps;
	mAllowCompression = allow_compression;
	mHasExplicitFormat = FALSE;
	mAutoGenMips = FALSE;
	mIsMask = FALSE;
	mMaskRMSE = 1.f ;
	mMaskMidPercentile = 1.f;
	mNeedsAlphaAndPickMask = FALSE ;
	mAlphaStride = 0 ;
	mAlphaOffset = INVALID_OFFSET ;
	mGLTextureCreated = FALSE ;
	mTexName.reset();
	mWidth = 0;
	mHeight	= 0;
	mCurrentDiscardLevel = -1;
	mTarget = GL_TEXTURE_2D;
	mBindTarget = LLTexUnit::TT_TEXTURE;
	mHasMipMaps = false;
	mMipLevels = -1;
	mIsResident = 0;
	mComponents = 0;
	mMaxDiscardLevel = MAX_DISCARD_LEVEL;
	mTexOptionsDirty = true;
	mAddressMode = LLTexUnit::TAM_WRAP;
	mFilterOption = LLTexUnit::TFO_ANISOTROPIC;
	mFormatInternal = -1;
	mFormatPrimary = (LLGLenum) 0;
	mFormatType = GL_UNSIGNED_BYTE;
	mFormatSwapBytes = FALSE;
#ifdef DEBUG_MISS
	mMissed	= FALSE;
#endif
	mCategory = -1;
}
void LLImageGL::cleanup()
{
	if (!gGLManager.mIsDisabled)
	{
		destroyGLTexture();
	}
	mSaveData = NULL;
}
static bool check_power_of_two(S32 dim)
{
	if(dim < 0)
	{
		return false ;
	}
	if(!dim)
	{
		return true ;
	}
	return !(dim & (dim - 1)) ;
}
bool LLImageGL::checkSize(S32 width, S32 height)
{
	return check_power_of_two(width) && check_power_of_two(height);
}
void LLImageGL::setSize(S32 width, S32 height, S32 ncomponents, S32 discard_level)
{
	if (width != mWidth || height != mHeight || ncomponents != mComponents)
	{
		if (!checkSize(width,height))
		{
			LL_WARNS() << llformat("Texture has non power of two dimension: %dx%d",width,height) << "  Unless on WhiteCore, beware." << LL_ENDL;
		}
		if (mTexName)
		{
			destroyGLTexture();
		}
		delete [] mPickMask;
		mPickMask = NULL;
		mPickMaskWidth = mPickMaskHeight = 0;
		mWidth = width;
		mHeight = height;
		mComponents = ncomponents;
		if (ncomponents > 0)
		{
			mMaxDiscardLevel = 0;
			while (width > 1 && height > 1 && mMaxDiscardLevel < MAX_DISCARD_LEVEL)
			{
				mMaxDiscardLevel++;
				width >>= 1;
				height >>= 1;
			}
			if(discard_level > 0)
			{
				mMaxDiscardLevel = llmax(mMaxDiscardLevel, (S8)discard_level);
			}
		}
		else
		{
			mMaxDiscardLevel = MAX_DISCARD_LEVEL;
		}
	}
}
void LLImageGL::dump()
{
	LL_INFOS() << "mMaxDiscardLevel " << S32(mMaxDiscardLevel)
			<< " mLastBindTime " << mLastBindTime
			<< " mTarget " << S32(mTarget)
			<< " mBindTarget " << S32(mBindTarget)
			<< " mUseMipMaps " << S32(mUseMipMaps)
			<< " mHasMipMaps " << S32(mHasMipMaps)
			<< " mCurrentDiscardLevel " << S32(mCurrentDiscardLevel)
			<< " mFormatInternal " << S32(mFormatInternal)
			<< " mFormatPrimary " << S32(mFormatPrimary)
			<< " mFormatType " << S32(mFormatType)
			<< " mFormatSwapBytes " << S32(mFormatSwapBytes)
			<< " mHasExplicitFormat " << S32(mHasExplicitFormat)
#if DEBUG_MISS
			<< " mMissed " << mMissed
#endif
			<< LL_ENDL;
	LL_INFOS() << " mTextureMemory " << mTextureMemory
			<< " mTexNames " << getTexName()
			<< " mIsResident " << S32(mIsResident)
			<< LL_ENDL;
}
void LLImageGL::forceUpdateBindStats(void) const
{
	mLastBindTime = sLastFrameTime;
}
BOOL LLImageGL::updateBindStats(S32Bytes tex_mem) const
{
	if (getTexName())
	{
#ifdef DEBUG_MISS
		mMissed = ! getIsResident(TRUE);
#endif
		sBindCount++;
		if (mLastBindTime != sLastFrameTime)
		{
			sUniqueCount++;
			updateBoundTexMem(tex_mem, mComponents, mCategory);
			mLastBindTime = sLastFrameTime;
			return TRUE ;
		}
	}
	return FALSE ;
}
F32 LLImageGL::getTimePassedSinceLastBound()
{
	return sLastFrameTime - mLastBindTime ;
}
void LLImageGL::setExplicitFormat( LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, BOOL swap_bytes )
{
	mHasExplicitFormat = TRUE;
	mFormatInternal = internal_format;
	mFormatPrimary = primary_format;
	if(type_format == 0)
		mFormatType = GL_UNSIGNED_BYTE;
	else
		mFormatType = type_format;
	mFormatSwapBytes = swap_bytes;
	calcAlphaChannelOffsetAndStride() ;
}
void LLImageGL::setImage(const LLImageRaw* imageraw)
{
	llassert((imageraw->getWidth() == getWidth(mCurrentDiscardLevel)) &&
			 (imageraw->getHeight() == getHeight(mCurrentDiscardLevel)) &&
			 (imageraw->getComponents() == getComponents()));
	const U8* rawdata = imageraw->getData();
	setImage(rawdata, FALSE);
}
static LLTrace::BlockTimerStatHandle FTM_SET_IMAGE("setImage");
void LLImageGL::setImage(const U8* data_in, BOOL data_hasmips)
{
	LL_RECORD_BLOCK_TIME(FTM_SET_IMAGE);
	bool is_compressed = false;
	if (mFormatPrimary >= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT && mFormatPrimary <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		is_compressed = true;
	}
	if (mUseMipMaps)
	{
		gGL.getTexUnit(0)->unbind(mBindTarget);
		mHasMipMaps = true;
		mTexOptionsDirty = true;
		setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
	}
	else
	{
		mHasMipMaps = false;
	}
	llverify(gGL.getTexUnit(0)->bind(this));
	mIsCompressed = false;
	if (mUseMipMaps)
	{
		if (data_hasmips)
		{
			for (S32 d=mCurrentDiscardLevel; d<=mMaxDiscardLevel; d++)
			{
				S32 w = getWidth(d);
				S32 h = getHeight(d);
				S32 gl_level = d-mCurrentDiscardLevel;
				mMipLevels = llmax(mMipLevels, gl_level);
				if (d > mCurrentDiscardLevel)
				{
					data_in -= dataFormatBytes(mFormatPrimary, w, h);
				}
				if (is_compressed)
				{
 					S32 tex_size = dataFormatBytes(mFormatPrimary, w, h);
					glCompressedTexImage2DARB(mTarget, gl_level, mFormatPrimary, w, h, 0, tex_size, (GLvoid *)data_in);
					stop_glerror();
					mIsCompressed = true;
				}
				else
				{
					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
						stop_glerror();
					}
					mIsCompressed = LLImageGL::setManualImage(mTarget, gl_level, mFormatInternal, w, h, mFormatPrimary, GL_UNSIGNED_BYTE, (GLvoid*)data_in, mAllowCompression);
					if (gl_level == 0)
					{
						analyzeAlpha(data_in, w, h);
					}
					updatePickMask(w, h, data_in);
					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
						stop_glerror();
					}
					stop_glerror();
				}
				stop_glerror();
			}
		}
		else if (!is_compressed)
		{
			if (mAutoGenMips)
			{
				stop_glerror();
				{
					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
						stop_glerror();
					}
					S32 w = getWidth(mCurrentDiscardLevel);
					S32 h = getHeight(mCurrentDiscardLevel);
					mMipLevels = wpo2(llmax(w, h));
					if (!LLRender::sGLCoreProfile)
					{
						glTexParameteri(mTarget, GL_GENERATE_MIPMAP, GL_TRUE);
					}
					mIsCompressed = LLImageGL::setManualImage(mTarget, 0, mFormatInternal,
								 w, h,
								 mFormatPrimary, mFormatType,
								 data_in, mAllowCompression);
					analyzeAlpha(data_in, w, h);
					stop_glerror();
					updatePickMask(w, h, data_in);
					if(mFormatSwapBytes)
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
						stop_glerror();
					}
					if (LLRender::sGLCoreProfile)
					{
						glGenerateMipmap(mTarget);
					}
					stop_glerror();
				}
			}
			else
			{
				S32 width = getWidth(mCurrentDiscardLevel);
				S32 height = getHeight(mCurrentDiscardLevel);
				S32 nummips = mMaxDiscardLevel - mCurrentDiscardLevel + 1;
				S32 w = width, h = height;
				const U8* prev_mip_data = 0;
				const U8* cur_mip_data = 0;
#ifdef SHOW_ASSERT
				S32 cur_mip_size = 0;
#endif
				mMipLevels = nummips;
				for (int m=0; m<nummips; m++)
				{
					if (m==0)
					{
						cur_mip_data = data_in;
#ifdef SHOW_ASSERT
						cur_mip_size = width * height * mComponents;
#endif
					}
					else
					{
						S32 bytes = w * h * mComponents;
#ifdef SHOW_ASSERT
						llassert(prev_mip_data);
						llassert(cur_mip_size == bytes*4);
#endif
						U8* new_data = new U8[bytes];
						llassert_always(new_data);
						LLImageBase::generateMip(prev_mip_data, new_data, w, h, mComponents);
						cur_mip_data = new_data;
#ifdef SHOW_ASSERT
						cur_mip_size = bytes;
#endif
					}
					llassert(w > 0 && h > 0 && cur_mip_data);
					{
						if(mFormatSwapBytes)
						{
							glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
							stop_glerror();
						}
						mIsCompressed = LLImageGL::setManualImage(mTarget, m, mFormatInternal, w, h, mFormatPrimary, mFormatType, cur_mip_data, mAllowCompression);
						if (m == 0)
						{
							analyzeAlpha(data_in, w, h);
						}
						stop_glerror();
						if (m == 0)
						{
							updatePickMask(w, h, cur_mip_data);
						}
						if(mFormatSwapBytes)
						{
							glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
							stop_glerror();
						}
					}
					if (prev_mip_data && prev_mip_data != data_in)
					{
						delete[] prev_mip_data;
					}
					prev_mip_data = cur_mip_data;
					w >>= 1;
					h >>= 1;
				}
				if (prev_mip_data && prev_mip_data != data_in)
				{
					delete[] prev_mip_data;
					prev_mip_data = NULL;
				}
			}
		}
		else
		{
			LL_ERRS() << "Compressed Image has mipmaps but data does not (can not auto generate compressed mips)" << LL_ENDL;
		}
	}
	else
	{
		mMipLevels = 0;
		S32 w = getWidth();
		S32 h = getHeight();
		if (is_compressed)
		{
			S32 tex_size = dataFormatBytes(mFormatPrimary, w, h);
			glCompressedTexImage2DARB(mTarget, 0, mFormatPrimary, w, h, 0, tex_size, (GLvoid *)data_in);
			stop_glerror();
			mIsCompressed = true;
		}
		else
		{
			if(mFormatSwapBytes)
			{
				glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
				stop_glerror();
			}
			mIsCompressed = LLImageGL::setManualImage(mTarget, 0, mFormatInternal, w, h,
						 mFormatPrimary, mFormatType, (GLvoid *)data_in, mAllowCompression);
			analyzeAlpha(data_in, w, h);
			updatePickMask(w, h, data_in);
			stop_glerror();
			if(mFormatSwapBytes)
			{
				glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
				stop_glerror();
			}
		}
	}
	stop_glerror();
	mGLTextureCreated = true;
}
BOOL LLImageGL::setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, BOOL force_fast_update)
{
	if (!width || !height)
	{
		return TRUE;
	}
	if (!getTexName())
	{
		return FALSE;
	}
	if (datap == NULL)
	{
		return FALSE;
	}
	if (!force_fast_update && x_pos == 0 && y_pos == 0 && width == getWidth() && height == getHeight() && data_width == width && data_height == height)
	{
		setImage(datap, FALSE);
	}
	else
	{
		if (mUseMipMaps)
		{
			dump();
			LL_ERRS() << "setSubImage called with mipmapped image (not supported)" << LL_ENDL;
		}
		llassert_always(mCurrentDiscardLevel == 0);
		llassert_always(x_pos >= 0 && y_pos >= 0);
		if (((x_pos + width) > getWidth()) ||
			(y_pos + height) > getHeight())
		{
			dump();
			LL_ERRS() << "Subimage not wholly in target image!"
				   << " x_pos " << x_pos
				   << " y_pos " << y_pos
				   << " width " << width
				   << " height " << height
				   << " getWidth() " << getWidth()
				   << " getHeight() " << getHeight()
				   << LL_ENDL;
		}
		if ((x_pos + width) > data_width ||
			(y_pos + height) > data_height)
		{
			dump();
			LL_ERRS() << "Subimage not wholly in source image!"
				   << " x_pos " << x_pos
				   << " y_pos " << y_pos
				   << " width " << width
				   << " height " << height
				   << " source_width " << data_width
				   << " source_height " << data_height
				   << LL_ENDL;
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, data_width);
		stop_glerror();
		if(mFormatSwapBytes)
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
			stop_glerror();
		}
		datap += (y_pos * data_width + x_pos) * getComponents();
		BOOL res = gGL.getTexUnit(0)->bindManual(mBindTarget, getTexName());
		if (!res) LL_ERRS() << "LLImageGL::setSubImage(): bindTexture failed" << LL_ENDL;
		stop_glerror();
		glTexSubImage2D(mTarget, 0, x_pos, y_pos,
						width, height, mFormatPrimary, mFormatType, datap);
		gGL.getTexUnit(0)->disable();
		stop_glerror();
		if(mFormatSwapBytes)
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
			stop_glerror();
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		stop_glerror();
		mGLTextureCreated = true;
	}
	return TRUE;
}
BOOL LLImageGL::setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, BOOL force_fast_update)
{
	return setSubImage(imageraw->getData(), imageraw->getWidth(), imageraw->getHeight(), x_pos, y_pos, width, height, force_fast_update);
}
BOOL LLImageGL::setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
	if (gGL.getTexUnit(0)->bind(this, false, true))
	{
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, fb_x, fb_y, x_pos, y_pos, width, height);
		mGLTextureCreated = true;
		stop_glerror();
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
static LLTrace::BlockTimerStatHandle FTM_GENERATE_TEXTURES("generate textures");
void LLImageGL::generateTextures(S32 numTextures, U32 *textures)
{
	LL_RECORD_BLOCK_TIME(FTM_GENERATE_TEXTURES);
	glGenTextures(numTextures, textures);
}
void LLImageGL::deleteTextures(S32 numTextures, U32 *textures, const std::vector<AllocationInfo>& allocationData)
{
	if (gGLManager.mInited)
	{
		for (auto& entry : allocationData)
		{
			texMemoryDeallocated(entry);
		}
		glDeleteTextures(numTextures, textures);
	}
}
void LLImageGL::texMemoryAllocated(const AllocationInfo& entry)
{
	sGlobalTextureMemory += (S64Bytes)entry.size;
	if (gAuditTexture)
	{
		incTextureCounter((S64Bytes)entry.size, entry.components, entry.category);
	}
}
void LLImageGL::texMemoryDeallocated(const AllocationInfo& entry)
{
	sGlobalTextureMemory -= (S64Bytes)entry.size;
	if (gAuditTexture)
	{
		decTextureCounter((S64Bytes)entry.size, entry.components, entry.category);
	}
}
static LLTrace::BlockTimerStatHandle FTM_SET_MANUAL_IMAGE("setManualImage");
bool LLImageGL::setManualImage(U32 target, S32 miplevel, S32 intformat, S32 width, S32 height, U32 pixformat, U32 pixtype, const void *pixels, bool allow_compression)
{
	LL_RECORD_BLOCK_TIME(FTM_SET_MANUAL_IMAGE);
	bool compressed = false;
	std::vector<U32> scratch;
	if (LLRender::sGLCoreProfile)
	{
#ifdef GL_ARB_texture_swizzle
		if(gGLManager.mHasTextureSwizzle)
		{
			if (pixformat == GL_ALPHA)
			{
				const GLint mask[] = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
				pixformat = GL_RED;
				intformat = GL_R8;
			}
			if (pixformat == GL_LUMINANCE)
			{
				const GLint mask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
				pixformat = GL_RED;
				intformat = GL_R8;
			}
			if (pixformat == GL_LUMINANCE_ALPHA)
			{
				const GLint mask[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask);
				pixformat = GL_RG;
				intformat = GL_RG8;
			}
		}
		else
#endif
		{
			if (pixformat == GL_ALPHA && pixtype == GL_UNSIGNED_BYTE)
			{
				scratch.resize(width*height);
				U32 pixel_count = (U32) (width*height);
				for (U32 i = 0; i < pixel_count; i++)
				{
					U8* pix = (U8*) &scratch[i];
					pix[0] = pix[1] = pix[2] = 0;
					pix[3] = ((U8*) pixels)[i];
				}
				pixels = &scratch[0];
				pixformat = GL_RGBA;
				intformat = GL_RGBA8;
			}
			if (pixformat == GL_LUMINANCE_ALPHA && pixtype == GL_UNSIGNED_BYTE)
			{
				scratch.resize(width*height);
				U32 pixel_count = (U32) (width*height);
				for (U32 i = 0; i < pixel_count; i++)
				{
					U8 lum = ((U8*) pixels)[i*2+0];
					U8 alpha = ((U8*) pixels)[i*2+1];
					U8* pix = (U8*) &scratch[i];
					pix[0] = pix[1] = pix[2] = lum;
					pix[3] = alpha;
				}
				pixels = &scratch[0];
				pixformat = GL_RGBA;
				intformat = GL_RGBA8;
			}
			if (pixformat == GL_LUMINANCE && pixtype == GL_UNSIGNED_BYTE)
			{
				scratch.resize(width*height);
				U32 pixel_count = (U32) (width*height);
				for (U32 i = 0; i < pixel_count; i++)
				{
					U8 lum = ((U8*) pixels)[i];
					U8* pix = (U8*) &scratch[i];
					pix[0] = pix[1] = pix[2] = lum;
					pix[3] = 255;
				}
				pixels = &scratch[0];
				pixformat = GL_RGBA;
				intformat = GL_RGB8;
			}
		}
	}
	if (LLImageGL::sCompressTextures && allow_compression)
	{
		compressed = true;
		switch (intformat)
		{
			case GL_RED:
			case GL_R8:
				intformat = GL_COMPRESSED_RED;
				break;
			case GL_RG:
			case GL_RG8:
				intformat = GL_COMPRESSED_RG;
				break;
			case GL_RGB:
			case GL_RGB8:
				intformat = GL_COMPRESSED_RGB;
				break;
			case GL_RGBA:
			case GL_RGBA8:
				intformat = GL_COMPRESSED_RGBA;
				break;
			case GL_LUMINANCE:
			case GL_LUMINANCE8:
				intformat = GL_COMPRESSED_LUMINANCE;
				break;
			case GL_LUMINANCE_ALPHA:
			case GL_LUMINANCE8_ALPHA8:
				intformat = GL_COMPRESSED_LUMINANCE_ALPHA;
				break;
			case GL_ALPHA:
			case GL_ALPHA8:
				intformat = GL_COMPRESSED_ALPHA;
				break;
			default:
				compressed = false;
				LL_WARNS() << "Could not compress format: " << std::hex << intformat << std::dec << LL_ENDL;
				break;
		}
	}
	stop_glerror();
	glTexImage2D(target, miplevel, intformat, width, height, 0, pixformat, pixtype, pixels);
	stop_glerror();
	return compressed;
}
static LLTrace::BlockTimerStatHandle FTM_CREATE_GL_TEXTURE1("createGLTexture()");
BOOL LLImageGL::createGLTexture()
{
	LL_RECORD_BLOCK_TIME(FTM_CREATE_GL_TEXTURE1);
	if (gGLManager.mIsDisabled)
	{
		LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
		return FALSE;
	}
	mGLTextureCreated = false ;
	llassert(gGLManager.mInited);
	stop_glerror();
	mTexName = createTextureName();
	stop_glerror();
	if (!getTexName())
	{
		LL_ERRS() << "LLImageGL::createGLTexture failed to make an empty texture" << LL_ENDL;
	}
	return TRUE ;
}
static LLTrace::BlockTimerStatHandle FTM_CREATE_GL_TEXTURE2("createGLTexture(raw)");
BOOL LLImageGL::createGLTexture(S32 discard_level, const LLImageRaw* imageraw, GLTextureName* usename, BOOL to_create, S32 category)
{
	LL_RECORD_BLOCK_TIME(FTM_CREATE_GL_TEXTURE2);
	if (gGLManager.mIsDisabled)
	{
		LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
		return FALSE;
	}
	mGLTextureCreated = false ;
	llassert(gGLManager.mInited);
	stop_glerror();
	if (discard_level < 0)
	{
		llassert(mCurrentDiscardLevel >= 0);
		discard_level = mCurrentDiscardLevel;
	}
	discard_level = llclamp(discard_level, 0, (S32)mMaxDiscardLevel);
	S32 raw_w = imageraw->getWidth() ;
	S32 raw_h = imageraw->getHeight() ;
	S32 w = raw_w << discard_level;
	S32 h = raw_h << discard_level;
	setSize(w, h, imageraw->getComponents(), discard_level);
	if( !mHasExplicitFormat )
	{
		switch (mComponents)
		{
			case 1:
			mFormatInternal = GL_LUMINANCE8;
			mFormatPrimary = GL_LUMINANCE;
			mFormatType = GL_UNSIGNED_BYTE;
			break;
			case 2:
			mFormatInternal = GL_LUMINANCE8_ALPHA8;
			mFormatPrimary = GL_LUMINANCE_ALPHA;
			mFormatType = GL_UNSIGNED_BYTE;
			break;
			case 3:
			mFormatInternal = GL_RGB8;
			mFormatPrimary = GL_RGB;
			mFormatType = GL_UNSIGNED_BYTE;
			break;
			case 4:
			mFormatInternal = GL_RGBA8;
			mFormatPrimary = GL_RGBA;
			mFormatType = GL_UNSIGNED_BYTE;
			break;
			default:
			LL_DEBUGS("Openjpeg") << "Bad number of components for texture: " << (U32)getComponents() << LL_ENDL;
			to_create = false;
			break;
		}
		calcAlphaChannelOffsetAndStride() ;
	}
	if(!to_create)
	{
		destroyGLTexture();
		mCurrentDiscardLevel = discard_level;
		mLastBindTime = sLastFrameTime;
		return TRUE ;
	}
	setCategory(category);
 	const U8* rawdata = imageraw->getData();
	return createGLTexture(discard_level, rawdata, FALSE, usename);
}
static LLTrace::BlockTimerStatHandle FTM_CREATE_GL_TEXTURE3("createGLTexture3(data)");
BOOL LLImageGL::createGLTexture(S32 discard_level, const U8* data_in, BOOL data_hasmips, GLTextureName* usename)
{
	LL_RECORD_BLOCK_TIME(FTM_CREATE_GL_TEXTURE3);
	llassert(data_in);
	stop_glerror();
	if (discard_level < 0)
	{
		llassert(mCurrentDiscardLevel >= 0);
		discard_level = mCurrentDiscardLevel;
	}
	discard_level = llclamp(discard_level, 0, (S32)mMaxDiscardLevel);
	if (getTexName() != 0 && discard_level == mCurrentDiscardLevel)
	{
		setImage(data_in, data_hasmips);
		return TRUE;
	}
	if (usename && *usename)
	{
		mTexName = *usename;
	}
	else
	{
		mTexName = createTextureName();
		stop_glerror();
		{
			llverify(gGL.getTexUnit(0)->bind(this));
			stop_glerror();
			glTexParameteri(LLTexUnit::getInternalType(mBindTarget), GL_TEXTURE_BASE_LEVEL, 0);
			stop_glerror();
			glTexParameteri(LLTexUnit::getInternalType(mBindTarget), GL_TEXTURE_MAX_LEVEL,  mMaxDiscardLevel-discard_level);
			stop_glerror();
		}
	}
	if (!getTexName())
	{
		LL_ERRS() << "LLImageGL::createGLTexture failed to make texture" << LL_ENDL;
	}
	if (mUseMipMaps)
	{
		mAutoGenMips = gGLManager.mHasMipMapGeneration;
#if LL_DARWIN
		if(gGLManager.mIsGF2or4MX && (mFormatInternal == GL_ALPHA8) && (mFormatPrimary == GL_ALPHA))
		{
			mAutoGenMips = FALSE;
		}
#endif
	}
	mCurrentDiscardLevel = discard_level;
	setImage(data_in, data_hasmips);
	glTexParameteri(LLTexUnit::getInternalType(mBindTarget), GL_TEXTURE_MAX_LEVEL, mMaxDiscardLevel - discard_level);
	gGL.getTexUnit(0)->setHasMipMaps(mHasMipMaps);
	gGL.getTexUnit(0)->setTextureAddressMode(mAddressMode);
	gGL.getTexUnit(0)->setTextureFilteringOption(mFilterOption);
	gGL.getTexUnit(0)->unbind(mBindTarget);
	stop_glerror();
	mTextureMemory = (S32Bytes)getMipBytes(discard_level);
	mTexName->addAllocatedMemory(mTextureMemory, mComponents, mCategory);
	mLastBindTime = sLastFrameTime;
	return TRUE;
}
BOOL LLImageGL::readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok)
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
		if(discard_level < 0)
			return FALSE;
	}
	if (getTexName() == 0 || discard_level < mCurrentDiscardLevel || discard_level > mMaxDiscardLevel )
	{
		return FALSE;
	}
	S32 gl_discard = discard_level - mCurrentDiscardLevel;
	gGL.getTexUnit(0)->unbind(mBindTarget);
	llverify(gGL.getTexUnit(0)->bindManual(mBindTarget, getTexName()));
	checkTexSize() ;
	LLGLint glwidth = 0;
	glGetTexLevelParameteriv(mTarget, gl_discard, GL_TEXTURE_WIDTH, (GLint*)&glwidth);
	if (glwidth == 0)
	{
		return FALSE;
	}
	S32 width = getWidth(discard_level);
	S32 height = getHeight(discard_level);
	S32 ncomponents = getComponents();
	if (ncomponents == 0)
	{
		return FALSE;
	}
	if(width < glwidth)
	{
		LL_WARNS() << "texture size is smaller than it should be." << LL_ENDL ;
		LL_WARNS() << "width: " << width << " glwidth: " << glwidth << " mWidth: " << mWidth <<
			" mCurrentDiscardLevel: " << (S32)mCurrentDiscardLevel << " discard_level: " << (S32)discard_level << LL_ENDL ;
		return FALSE ;
	}
	if (width <= 0 || width > 2048 || height <= 0 || height > 2048 || ncomponents < 1 || ncomponents > 4)
	{
		LL_ERRS() << llformat("LLImageGL::readBackRaw: bogus params: %d x %d x %d",width,height,ncomponents) << LL_ENDL;
	}
	GLenum error ;
	while((error = glGetError()) != GL_NO_ERROR)
	{
		LL_WARNS() << "GL Error happens before reading back texture. Error code: " << error << LL_ENDL ;
	}
	if (mIsCompressed)
	{
		if (!compressed_ok)
		{
			return false;
		}
		LLGLint glbytes;
		glGetTexLevelParameteriv(mTarget, gl_discard, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (GLint*)&glbytes);
		if(!imageraw->allocateDataSize(width, height, ncomponents, glbytes))
		{
			LL_WARNS() << "Memory allocation failed for reading back texture. Size is: " << glbytes << LL_ENDL ;
			LL_WARNS() << "width: " << width << "height: " << height << "components: " << ncomponents << LL_ENDL ;
			return FALSE ;
		}
		glGetCompressedTexImageARB(mTarget, gl_discard, (GLvoid*)(imageraw->getData()));
	}
	else
	{
		if(!imageraw->allocateDataSize(width, height, ncomponents))
		{
			LL_WARNS() << "Memory allocation failed for reading back texture." << LL_ENDL ;
			LL_WARNS() << "width: " << width << "height: " << height << "components: " << ncomponents << LL_ENDL ;
			return FALSE ;
		}
		glGetTexImage(GL_TEXTURE_2D, gl_discard, mFormatPrimary, mFormatType, (GLvoid*)(imageraw->getData()));
	}
	if((error = glGetError()) != GL_NO_ERROR)
	{
		LL_WARNS() << "GL Error happens after reading back texture. Error code: " << error << LL_ENDL ;
		imageraw->deleteData() ;
		while((error = glGetError()) != GL_NO_ERROR)
		{
			LL_WARNS() << "GL Error happens after reading back texture. Error code: " << error << LL_ENDL ;
		}
		return FALSE ;
	}
	return TRUE ;
}
void LLImageGL::deleteDeadTextures()
{
	bool reset = false;
	if (reset)
	{
		gGL.getTexUnit(0)->activate();
	}
}
void LLImageGL::destroyGLTexture()
{
	if (getTexName())
	{
		mTexName.reset();
		mCurrentDiscardLevel = -1 ;
		mGLTextureCreated = FALSE ;
	}
}
void LLImageGL::forceToInvalidateGLTexture()
{
	if (getTexName() != 0)
	{
		destroyGLTexture();
	}
	else
	{
		mCurrentDiscardLevel = -1 ;
	}
}
void LLImageGL::setAddressMode(LLTexUnit::eTextureAddressMode mode)
{
	if (mAddressMode != mode)
	{
		mTexOptionsDirty = true;
		mAddressMode = mode;
	}
	GLuint tex = getTexName();
	if (tex && gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->getCurrTexture() == tex)
	{
		gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->setTextureAddressMode(mode);
		mTexOptionsDirty = false;
	}
}
void LLImageGL::setFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
	if (mFilterOption != option)
	{
		mTexOptionsDirty = true;
		mFilterOption = option;
	}
	GLuint tex = getTexName();
	if (tex && gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->getCurrTexture() == tex)
	{
		gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->setTextureFilteringOption(option);
		mTexOptionsDirty = false;
		stop_glerror();
	}
}
BOOL LLImageGL::getIsResident(BOOL test_now)
{
	if (test_now)
	{
		GLuint tex = getTexName();
		if (tex)
		{
			glAreTexturesResident(1, (GLuint*)&tex, &mIsResident);
		}
		else
		{
			mIsResident = FALSE;
		}
	}
	return mIsResident;
}
S32 LLImageGL::getHeight(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 height = mHeight >> discard_level;
	if (height < 1) height = 1;
	return height;
}
S32 LLImageGL::getWidth(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 width = mWidth >> discard_level;
	if (width < 1) width = 1;
	return width;
}
S32 LLImageGL::getBytes(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 w = mWidth>>discard_level;
	S32 h = mHeight>>discard_level;
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	return dataFormatBytes(mFormatPrimary, w, h);
}
S32 LLImageGL::getMipBytes(S32 discard_level) const
{
	if (discard_level < 0)
	{
		discard_level = mCurrentDiscardLevel;
	}
	S32 w = mWidth>>discard_level;
	S32 h = mHeight>>discard_level;
	S32 res = dataFormatBytes(mFormatPrimary, w, h);
	if (mUseMipMaps)
	{
		while (w > 1 && h > 1)
		{
			w >>= 1; if (w == 0) w = 1;
			h >>= 1; if (h == 0) h = 1;
			res += dataFormatBytes(mFormatPrimary, w, h);
		}
	}
	return res;
}
BOOL LLImageGL::isJustBound() const
{
	return (BOOL)(sLastFrameTime - mLastBindTime < 0.5f);
}
BOOL LLImageGL::getBoundRecently() const
{
	return (BOOL)(sLastFrameTime - mLastBindTime < MIN_TEXTURE_LIFETIME);
}
void LLImageGL::setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target)
{
	mTarget = target;
	mBindTarget = bind_target;
}
void LLImageGL::setNeedsAlphaAndPickMask(BOOL need_mask)
{
	if(mNeedsAlphaAndPickMask != need_mask)
	{
		mNeedsAlphaAndPickMask = need_mask;
		if(mNeedsAlphaAndPickMask)
		{
			mAlphaOffset = 0 ;
		}
		else
		{
			mAlphaOffset = INVALID_OFFSET ;
		}
	}
}
void LLImageGL::calcAlphaChannelOffsetAndStride()
{
	if(mAlphaOffset == INVALID_OFFSET)
	{
		return ;
	}
	mAlphaStride = -1 ;
	switch (mFormatPrimary)
	{
	case GL_LUMINANCE:
	case GL_ALPHA:
		mAlphaStride = 1;
		break;
	case GL_LUMINANCE_ALPHA:
		mAlphaStride = 2;
		break;
	case GL_RGB:
		setNeedsAlphaAndPickMask(FALSE);
		return ;
	case GL_RGBA:
		mAlphaStride = 4;
		break;
	case GL_BGRA_EXT:
		mAlphaStride = 4;
		break;
	default:
		break;
	}
	mAlphaOffset = -1 ;
	if (mFormatType == GL_UNSIGNED_BYTE)
	{
		mAlphaOffset = mAlphaStride - 1 ;
	}
	else if(is_little_endian())
	{
		if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
		{
			mAlphaOffset = 0 ;
		}
		else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
		{
			mAlphaOffset = 3 ;
		}
	}
	else
	{
		if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
		{
			mAlphaOffset = 3 ;
		}
		else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
		{
			mAlphaOffset = 0 ;
		}
	}
	if( mAlphaStride < 1 ||
		mAlphaOffset < 0 ||
		(mFormatPrimary == GL_BGRA_EXT && mFormatType != GL_UNSIGNED_BYTE))
	{
		LL_WARNS() << "Cannot analyze alpha for image with format type " << std::hex << mFormatType << std::dec << LL_ENDL;
		setNeedsAlphaAndPickMask(FALSE);
	}
}
void LLImageGL::analyzeAlpha(const void* data_in, U32 w, U32 h)
{
	if(!mNeedsAlphaAndPickMask)
	{
		return ;
	}
	F64 sum = 0;
	U32 length = w * h;
	U32 alphatotal = 0;
	U32 sample[16];
	memset(sample, 0, sizeof(U32)*16);
	U32 min = 0, max = 0;
	U32 mids = 0;
	if (w >= 2 && h >= 2)
	{
		llassert(w%2 == 0);
		llassert(h%2 == 0);
		const GLubyte* rowstart = ((const GLubyte*) data_in) + mAlphaOffset;
		for (U32 y = 0; y < h; y+=2)
		{
			const GLubyte* current = rowstart;
			for (U32 x = 0; x < w; x+=2)
			{
				const U32 s1 = current[0];
				alphatotal += s1;
				const U32 s2 = current[w * mAlphaStride];
				alphatotal += s2;
				current += mAlphaStride;
				const U32 s3 = current[0];
				alphatotal += s3;
				const U32 s4 = current[w * mAlphaStride];
				alphatotal += s4;
				current += mAlphaStride;
				++sample[s1/16];
				++sample[s2/16];
				++sample[s3/16];
				++sample[s4/16];
				min = std::min(std::min(std::min(std::min(min, s1), s2), s3), s4);
				max = std::max(std::max(std::max(std::max(max, s1), s2), s3), s4);
				mids += (s1 > 2 && s1 < 253) + (s2 > 2 && s2 < 253) + (s3 > 2 && s3 < 253) + (s4 > 2 && s4 < 253);
				const U32 asum = (s1+s2+s3+s4);
				alphatotal += asum;
				sample[asum/(16*4)] += 4;
				S32 avg = (s1+s2+s3+s4)/4;
				if(avg >=128)
				{
					avg-=255;
				}
				sum+=F64(avg*avg*4)/F64(length);
			}
			rowstart += 2 * w * mAlphaStride;
		}
		length *= 2;
	}
	else
	{
		const GLubyte* current = ((const GLubyte*) data_in) + mAlphaOffset;
		for (U32 i = 0; i < length; i++)
		{
			const U32 s1 = *current;
			alphatotal += s1;
			++sample[s1/16];
			current += mAlphaStride;
			min = std::min(min, s1);
			max = std::max(max, s1);
			mids += (s1 > 2 && s1 < 253);
			if(i%2==0)
			{
				const U32 s2 = *current;
				min = std::min(min, s2);
				max = std::max(max, s2);
				mids += (s2 > 2 && s2 < 253);
				S32 avg = (s1+s2)/2;
				if(avg >=128)
					avg-=255;
				sum+=F64(avg*avg*2)/F64(length);
			}
		}
	}
	U32 midrangetotal = 0;
	for (U32 i = 3; i < 13; i++)
	{
		midrangetotal += sample[i];
	}
	U32 lowerhalftotal = 0;
	for (U32 i = 0; i < 8; i++)
	{
		lowerhalftotal += sample[i];
	}
	U32 upperhalftotal = 0;
	for (U32 i = 8; i < 16; i++)
	{
		upperhalftotal += sample[i];
	}
	if (midrangetotal > length/48 ||
	    (lowerhalftotal == length && alphatotal != 0) ||
	    (upperhalftotal == length && alphatotal != 255*length))
	{
		mIsMask = FALSE;
	}
	else
	{
		mIsMask = TRUE;
	}
	mMaskMidPercentile = (F32)mids / (F32)(w * h);
	mMaskRMSE = ((max-min)%255)==0 ? sqrt(sum)/255.0 : FLT_MAX;
}
U32 LLImageGL::createPickMask(S32 pWidth, S32 pHeight)
{
	U32 pick_width = pWidth/2 + 1;
	U32 pick_height = pHeight/2 + 1;
	U32 size = pick_width * pick_height;
	size = (size + 7) / 8;
	mPickMask = new U8[size];
	mPickMaskWidth = pick_width - 1;
	mPickMaskHeight = pick_height - 1;
	memset(mPickMask, 0, sizeof(U8) * size);
	return size;
}
void LLImageGL::freePickMask()
{
	if (mPickMask != NULL)
	{
		delete [] mPickMask;
	}
	mPickMask = NULL;
	mPickMaskWidth = mPickMaskHeight = 0;
}
void LLImageGL::updatePickMask(S32 width, S32 height, const U8* data_in)
{
	if(!mNeedsAlphaAndPickMask)
	{
		return ;
	}
	freePickMask();
	if (mFormatType != GL_UNSIGNED_BYTE ||
	    mFormatPrimary != GL_RGBA)
	{
		return;
	}
#ifdef SHOW_ASSERT
	const U32 pickSize = createPickMask(width, height);
#else
	createPickMask(width, height);
#endif
	U32 pick_bit = 0;
	for (S32 y = 0; y < height; y += 2)
	{
		for (S32 x = 0; x < width; x += 2)
		{
			U8 alpha = data_in[(y*width+x)*4+3];
			if (alpha > 32)
			{
				U32 pick_idx = pick_bit/8;
				U32 pick_offset = pick_bit%8;
				llassert(pick_idx < pickSize);
				mPickMask[pick_idx] |= 1 << pick_offset;
			}
			++pick_bit;
		}
	}
}
BOOL LLImageGL::getMask(const LLVector2 &tc)
{
	BOOL res = TRUE;
	if (mPickMask)
	{
		F32 u,v;
		if (LL_LIKELY(tc.isFinite()))
		{
			u = tc.mV[0] - floorf(tc.mV[0]);
			v = tc.mV[1] - floorf(tc.mV[1]);
		}
		else
		{
			LL_WARNS_ONCE("render") << "Ugh, non-finite u/v in mask pick" << LL_ENDL;
			u = v = 0.f;
		}
		if (LL_UNLIKELY(u < 0.f || u > 1.f ||
				v < 0.f || v > 1.f))
		{
			LL_WARNS_ONCE("render") << "Ugh, u/v out of range in image mask pick" << LL_ENDL;
			u = v = 0.f;
		}
		S32 x = llfloor(u * mPickMaskWidth);
		S32 y = llfloor(v * mPickMaskHeight);
		if (LL_UNLIKELY(x > mPickMaskWidth))
		{
			LL_WARNS_ONCE("render") << "Ooh, width overrun on pick mask read, that coulda been bad." << LL_ENDL;
			x = llmax((U16)0, mPickMaskWidth);
		}
		if (LL_UNLIKELY(y > mPickMaskHeight))
		{
			LL_WARNS_ONCE("render") << "Ooh, height overrun on pick mask read, that woulda been bad." << LL_ENDL;
			y = llmax((U16)0, mPickMaskHeight);
		}
		S32 idx = y*mPickMaskWidth+x;
		S32 offset = idx%8;
		res = mPickMask[idx/8] & (1 << offset) ? TRUE : FALSE;
	}
	return res;
}
void LLImageGL::setCategory(S32 category)
{
	if(!gAuditTexture)
	{
		return ;
	}
	if(mCategory != category)
	{
		if(mCategory > -1)
		{
			sTextureMemByCategory[mCategory] -= mTextureMemory ;
		}
		if(category > -1 && category < sMaxCategories)
		{
			sTextureMemByCategory[category] += mTextureMemory ;
			mCategory = category;
		}
		else
		{
			mCategory = -1 ;
		}
	}
}
S32 LLImageGL::getTextureCounterIndex(U32 val)
{
	if(val < 2)
	{
		return 0 ;
	}
	else if(val >= (1 << MAX_TEXTURE_LOG_SIZE))
	{
		return MAX_TEXTURE_LOG_SIZE ;
	}
	else
	{
		S32 ret = 0 ;
		while(val >>= 1)
		{
			++ret;
		}
		return ret ;
	}
}
void LLImageGL::incTextureCounter(S32Bytes val, S32 ncomponents, S32 category)
{
	sTextureLoadedCounter[getTextureCounterIndex(val.value())]++ ;
	if(category > -1)
		sTextureMemByCategory[category] += (S32Bytes)val * ncomponents ;
}
void LLImageGL::decTextureCounter(S32Bytes val, S32 ncomponents, S32 category)
{
	sTextureLoadedCounter[getTextureCounterIndex(val.value())]-- ;
	if(category > -1)
		sTextureMemByCategory[category] -= (S32Bytes)val * ncomponents ;
}
void LLImageGL::setCurTexSizebar(S32 index, BOOL set_pick_size)
{
	sCurTexSizeBar = index ;
	if(set_pick_size)
	{
		sCurTexPickSize = (1 << index) ;
	}
	else
	{
		sCurTexPickSize = -1 ;
	}
}
void LLImageGL::resetCurTexSizebar()
{
	sCurTexSizeBar = -1 ;
	sCurTexPickSize = -1 ;
}
