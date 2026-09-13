/** 
 * @file llimagetga.h
 * @brief Image implementation to compresses and decompressed TGA files.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#ifndef LL_LLIMAGETGA_H
#define LL_LLIMAGETGA_H
#include "llimage.h"
class LLImageTGA : public LLImageFormatted
{
protected:
	virtual ~LLImageTGA();
public:
	LLImageTGA();
	LLImageTGA(const std::string& file_name);
	std::string getExtension() { return std::string("tga"); }
	BOOL updateData();
	BOOL decode(LLImageRaw* raw_image, F32 decode_time=0.0);
	BOOL encode(const LLImageRaw* raw_image, F32 encode_time=0.0);
	BOOL			 decodeAndProcess(LLImageRaw* raw_image, F32 domain, F32 weight);
private:
	BOOL			 decodeTruecolor( LLImageRaw* raw_image, BOOL rle, BOOL flipped );
	BOOL			 decodeTruecolorRle8( LLImageRaw* raw_image );
	BOOL			 decodeTruecolorRle15( LLImageRaw* raw_image );
	BOOL			 decodeTruecolorRle24( LLImageRaw* raw_image );
	BOOL			 decodeTruecolorRle32( LLImageRaw* raw_image, BOOL &alpha_opaque );
	void			 decodeTruecolorPixel15( U8* dst, const U8* src );
	BOOL			 decodeTruecolorNonRle( LLImageRaw* raw_image, BOOL &alpha_opaque );
	BOOL			 decodeColorMap( LLImageRaw* raw_image, BOOL rle, BOOL flipped );
	void			 decodeColorMapPixel8(U8* dst, const U8* src);
	void			 decodeColorMapPixel15(U8* dst, const U8* src);
	void			 decodeColorMapPixel24(U8* dst, const U8* src);
	void			 decodeColorMapPixel32(U8* dst, const U8* src);
	bool			 loadFile(const std::string& file_name);
private:
	U32 mDataOffset;
	U8 mIDLength;
	U8 mColorMapType;
	U8 mImageType;
	U8 mColorMapIndexLo;
	U8 mColorMapIndexHi;
	U8 mColorMapLengthLo;
	U8 mColorMapLengthHi;
	U8 mColorMapDepth;
	U8 mXOffsetLo;
	U8 mXOffsetHi;
	U8 mYOffsetLo;
	U8 mYOffsetHi;
	U8 mWidthLo;
	U8 mWidthHi;
	U8 mHeightLo;
	U8 mHeightHi;
	U8 mPixelSize;
	U8 mAttributeBits;
	U8 mOriginRightBit;
	U8 mOriginTopBit;
	U8 mInterleave;
	U8*		mColorMap;
	S32		mColorMapStart;
	S32		mColorMapLength;
	S32		mColorMapBytesPerEntry;
	BOOL	mIs15Bit;
	static const U8 s5to8bits[32];
};
#endif
