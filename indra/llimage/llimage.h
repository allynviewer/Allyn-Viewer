/** 
 * @file llimage.h
 * @brief Object for managing images and their textures.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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
#ifndef LL_LLIMAGE_H
#define LL_LLIMAGE_H
#include "lluuid.h"
#include "llstring.h"
#include "llthread.h"
#include "aithreadsafe.h"
const S32 MIN_IMAGE_MIP =  2;
const S32 MAX_IMAGE_MIP = 11;
const S32 MAX_DISCARD_LEVEL = 5;
const S32 MIN_IMAGE_SIZE = (1<<MIN_IMAGE_MIP);
const S32 MAX_IMAGE_SIZE = (1<<MAX_IMAGE_MIP);
const S32 MIN_IMAGE_AREA = MIN_IMAGE_SIZE * MIN_IMAGE_SIZE;
const S32 MAX_IMAGE_AREA = MAX_IMAGE_SIZE * MAX_IMAGE_SIZE;
const S32 MAX_IMAGE_COMPONENTS = 8;
const S32 MAX_IMAGE_DATA_SIZE = MAX_IMAGE_AREA * MAX_IMAGE_COMPONENTS;
const S32 FIRST_PACKET_SIZE = 600;
const S32 MAX_IMG_PACKET_SIZE = 1000;
class LLImageFormatted;
class LLImageRaw;
class LLColor4U;
class LLPrivateMemoryPool;
typedef enum e_image_codec
{
	IMG_CODEC_INVALID  = 0,
	IMG_CODEC_RGB  = 1,
	IMG_CODEC_J2C  = 2,
	IMG_CODEC_BMP  = 3,
	IMG_CODEC_TGA  = 4,
	IMG_CODEC_JPEG = 5,
	IMG_CODEC_DXT  = 6,
	IMG_CODEC_PNG  = 7,
	IMG_CODEC_EOF  = 8
} EImageCodec;
class LLImage
{
public:
	static void initClass();
	static void cleanupClass();
	static const std::string& getLastError();
	static void setLastError(const std::string& message);
protected:
	static LLMutex* sMutex;
	static std::string sLastErrorMessage;
};
class LLImageBase : public LLThreadSafeRefCount
{
protected:
	virtual ~LLImageBase();
public:
	LLImageBase();
	enum
	{
		TYPE_NORMAL = 0,
		TYPE_AVATAR_BAKE = 1,
	};
	virtual void deleteData();
	virtual U8* allocateData(S32 size = -1);
	virtual U8* reallocateData(S32 size = -1);
	static void deleteData(U8* data) { FREE_MEM(sPrivatePoolp, data); }
	U8* release() { U8* data = mData; mData = NULL; mDataSize = 0; return data; }
	virtual void dump();
	virtual void sanityCheck();
	U16 getWidth() const		{ return mWidth; }
	U16 getHeight() const		{ return mHeight; }
	S8	getComponents() const	{ return mComponents; }
	S32 getDataSize() const		{ return mDataSize; }
	const U8 *getData() const	;
	U8 *getData()				;
	bool isBufferInvalid() ;
	void setSize(S32 width, S32 height, S32 ncomponents);
	U8* allocateDataSize(S32 width, S32 height, S32 ncomponents, S32 size = -1);
	void enableOverSize() {mAllowOverSize = true ;}
	void disableOverSize() {mAllowOverSize = false; }
protected:
	void setDataAndSize(U8 *data, S32 size);
public:
	static void generateMip(const U8 *indata, U8* mipdata, int width, int height, S32 nchannels);
	static F32 calc_download_priority(F32 virtual_size, F32 visible_area, S32 bytes_sent);
	static EImageCodec getCodecFromExtension(const std::string& exten);
	static void createPrivatePool() ;
	static void destroyPrivatePool() ;
	static LLPrivateMemoryPool* getPrivatePool() {return sPrivatePoolp;}
private:
	U8 *mData;
	S32 mDataSize;
	U16 mWidth;
	U16 mHeight;
	S8 mComponents;
	bool mBadBufferAllocation ;
	bool mAllowOverSize ;
	static LLPrivateMemoryPool* sPrivatePoolp ;
};
class LLImageRaw : public LLImageBase
{
protected:
	~LLImageRaw();
public:
	LLImageRaw();
	LLImageRaw(U16 width, U16 height, S8 components);
	LLImageRaw(U8 *data, U16 width, U16 height, S8 components, bool no_copy = false);
	LLImageRaw(LLImageRaw const* src, U16 width, U16 height, U16 crop_offset, bool crop_vertically);
	void deleteData();
	U8* allocateData(S32 size = -1);
	U8* reallocateData(S32 size = -1);
	BOOL resize(U16 width, U16 height, S8 components);
	BOOL setSubImage(U32 x_pos, U32 y_pos, U32 width, U32 height,
					 const U8 *data, U32 stride = 0, BOOL reverse_y = FALSE);
	void clear(U8 r=0, U8 g=0, U8 b=0, U8 a=255);
	void verticalFlip();
	void expandToPowerOfTwo(S32 max_dim = MAX_IMAGE_SIZE, BOOL scale_image = TRUE);
	void contractToPowerOfTwo(S32 max_dim = MAX_IMAGE_SIZE, BOOL scale_image = TRUE);
	void biasedScaleToPowerOfTwo(S32 target_width, S32 target_height, S32 max_dim = MAX_IMAGE_SIZE);
	void biasedScaleToPowerOfTwo(S32 max_dim = MAX_IMAGE_SIZE) { biasedScaleToPowerOfTwo(getWidth(), getHeight(), max_dim); }
	BOOL scale( S32 new_width, S32 new_height, BOOL scale_image = TRUE );
	void fill( const LLColor4U& color );
	LLPointer<LLImageRaw> duplicate();
	void copy( LLImageRaw* src );
	void copyUnscaled( LLImageRaw* src );
	void copyUnscaled4onto3( LLImageRaw* src );
	void copyUnscaled3onto4( LLImageRaw* src );
	void copyUnscaledAlphaMask( LLImageRaw* src, const LLColor4U& fill);
	void copyScaled( LLImageRaw* src );
	void copyScaled3onto4( LLImageRaw* src );
	void copyScaled4onto3( LLImageRaw* src );
	void composite( LLImageRaw* src );
	void compositeScaled4onto3( LLImageRaw* src );
	void compositeUnscaled4onto3( LLImageRaw* src );
	std::string getComment() const { return mComment; }
	std::string mComment;
protected:
	void copyLineScaled( U8* in, U8* out, S32 in_pixel_len, S32 out_pixel_len, S32 in_pixel_step, S32 out_pixel_step );
	void compositeRowScaled4onto3( U8* in, U8* out, S32 in_pixel_len, S32 out_pixel_len );
	U8	fastFractionalMult(U8 a,U8 b);
	void setDataAndSize(U8 *data, S32 width, S32 height, S8 components) ;
public:
	static AIThreadSafeSimpleDC<S64> sGlobalRawMemory;
	static S32 sRawImageCount;
	static S32 sRawImageCachedCount;
	S32 mCacheEntries;
	void setInCache(bool in_cache)
	{
		if(in_cache)
		{
			if(!mCacheEntries)
				sRawImageCachedCount++;
			mCacheEntries++;
		}
		else if(mCacheEntries)
		{
			mCacheEntries--;
			if(!mCacheEntries)
				sRawImageCachedCount--;
		}
	}
};
class LLImageFormatted : public LLImageBase
{
public:
	static LLImageFormatted* createFromType(S8 codec);
	static LLImageFormatted* createFromExtension(const std::string& instring);
protected:
	~LLImageFormatted();
public:
	LLImageFormatted(S8 codec);
	void deleteData();
	U8* allocateData(S32 size = -1);
	U8* reallocateData(S32 size = -1);
	void dump();
	void sanityCheck();
	virtual std::string getExtension() = 0;
	virtual S32 calcHeaderSize() { return 0; };
	virtual S32 calcDataSize(S32 discard_level);
	virtual S32 calcDiscardLevelBytes(S32 bytes);
	virtual S8  getRawDiscardLevel() { return mDiscardLevel; }
	BOOL load(const std::string& filename, int load_size = 0);
	BOOL save(const std::string& filename);
	virtual BOOL updateData() = 0;
 	void setData(U8 *data, S32 size);
 	void appendData(U8 *data, S32 size);
	virtual BOOL decode(LLImageRaw* raw_image, F32 decode_time) = 0;
	virtual BOOL decodeChannels(LLImageRaw* raw_image, F32 decode_time, S32 first_channel, S32 max_channel);
	virtual BOOL encode(const LLImageRaw* raw_image, F32 encode_time) = 0;
	S8 getCodec() const;
	BOOL isDecoding() const { return mDecoding ? TRUE : FALSE; }
	BOOL isDecoded()  const { return mDecoded ? TRUE : FALSE; }
	void setDiscardLevel(S8 discard_level) { mDiscardLevel = discard_level; }
	S8 getDiscardLevel() const { return mDiscardLevel; }
	virtual void resetLastError();
	virtual void setLastError(const std::string& message, const std::string& filename = std::string());
protected:
	BOOL copyData(U8 *data, S32 size);
protected:
	S8 mCodec;
	S8 mDecoding;
	S8 mDecoded;
	S8 mDiscardLevel;
public:
	static S32 sGlobalFormattedMemory;
};
#endif
