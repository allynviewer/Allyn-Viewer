/** 
 * @file llimagetga.cpp
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
#include "llimagetga.h"
#include "lldir.h"
#include "llerror.h"
#include "llmath.h"
#include "llpointer.h"
const U8 LLImageTGA::s5to8bits[32] =
	{
		0,   8,  16,  25,  33,  41,  49,  58,
	   66,  74,  82,  90,  99, 107, 115, 123,
	  132, 140, 148, 156, 165, 173, 181, 189,
	  197, 206, 214, 222, 230, 239, 247, 255
	};
inline void LLImageTGA::decodeTruecolorPixel15( U8* dst, const U8* src )
{
	U32 t = U32(src[0]) + (U32(src[1]) << 8);
    dst[2] = s5to8bits[t & 0x1F];
    t >>= 5;
    dst[1] = s5to8bits[t & 0x1F];
    t >>= 5;
    dst[0] = s5to8bits[t & 0x1F];
}
LLImageTGA::LLImageTGA()
	: LLImageFormatted(IMG_CODEC_TGA),
	  mColorMap( NULL ),
	  mColorMapStart( 0 ),
	  mColorMapLength( 0 ),
	  mColorMapBytesPerEntry( 0 ),
	  mIs15Bit( FALSE ),
	  mAttributeBits(0),
	  mColorMapDepth(0),
	  mColorMapIndexHi(0),
	  mColorMapIndexLo(0),
	  mColorMapLengthHi(0),
	  mColorMapLengthLo(0),
	  mColorMapType(0),
	  mDataOffset(0),
	  mHeightHi(0),
	  mHeightLo(0),
	  mIDLength(0),
	  mImageType(0),
	  mInterleave(0),
	  mOriginRightBit(0),
	  mOriginTopBit(0),
	  mPixelSize(0),
	  mWidthHi(0),
	  mWidthLo(0),
	  mXOffsetHi(0),
	  mXOffsetLo(0),
	  mYOffsetHi(0),
	  mYOffsetLo(0)
{
}
LLImageTGA::LLImageTGA(const std::string& file_name)
	: LLImageFormatted(IMG_CODEC_TGA),
	  mColorMap( NULL ),
	  mColorMapStart( 0 ),
	  mColorMapLength( 0 ),
	  mColorMapBytesPerEntry( 0 ),
	  mIs15Bit( FALSE )
{
	loadFile(file_name);
}
LLImageTGA::~LLImageTGA()
{
	delete [] mColorMap;
}
BOOL LLImageTGA::updateData()
{
	resetLastError();
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("LLImageTGA uninitialized");
		return FALSE;
	}
	U8	flags;
	U8	junk[256];
	mDataOffset = 0;
	mIDLength = *(getData()+mDataOffset++);
	mColorMapType = *(getData()+mDataOffset++);
	mImageType = *(getData()+mDataOffset++);
	mColorMapIndexLo = *(getData()+mDataOffset++);
	mColorMapIndexHi = *(getData()+mDataOffset++);
	mColorMapLengthLo = *(getData()+mDataOffset++);
	mColorMapLengthHi = *(getData()+mDataOffset++);
	mColorMapDepth = *(getData()+mDataOffset++);
	mXOffsetLo = *(getData()+mDataOffset++);
	mXOffsetHi = *(getData()+mDataOffset++);
	mYOffsetLo = *(getData()+mDataOffset++);
	mYOffsetHi = *(getData()+mDataOffset++);
	mWidthLo = *(getData()+mDataOffset++);
	mWidthHi = *(getData()+mDataOffset++);
	mHeightLo = *(getData()+mDataOffset++);
	mHeightHi = *(getData()+mDataOffset++);
	mPixelSize = *(getData()+mDataOffset++);
	flags = *(getData()+mDataOffset++);
	mAttributeBits = flags & 0xf;
	mOriginRightBit = (flags & 0x10) >> 4;
	mOriginTopBit = (flags & 0x20) >> 5;
	mInterleave = (flags & 0xc0) >> 6;
	switch( mImageType )
	{
	case 0:
		setLastError("Unable to load file.  TGA file contains no image data.");
		return FALSE;
	case 1:
		if( 8 != mPixelSize )
		{
			setLastError("Unable to load file.  Colormapped images must have 8 bits per pixel.");
			return FALSE;
		}
		break;
	case 2:
		break;
	case 3:
		if( 8 != mPixelSize )
		{
			setLastError("Unable to load file.  Monochrome images must have 8 bits per pixel.");
			return FALSE;
		}
		break;
	case 9:
		break;
	case 10:
		break;
	case 11:
		if( 8 != mPixelSize )
		{
			setLastError("Unable to load file.  Monochrome images must have 8 bits per pixel.");
			return FALSE;
		}
		break;
	default:
		setLastError("Unable to load file.  Unrecoginzed TGA image type.");
		return FALSE;
	}
	if (mIDLength)
	{
		memcpy(junk, getData()+mDataOffset, mIDLength);
		mDataOffset += mIDLength;
	}
	S32 color_map_bytes = 0;
	if( (1 == mColorMapType) && (mColorMapDepth > 0) )
	{
		mColorMapStart = (S32(mColorMapIndexHi) << 8) + mColorMapIndexLo;
		mColorMapLength = (S32(mColorMapLengthHi) << 8) + mColorMapLengthLo;
		if( mColorMapDepth > 24 )
		{
			mColorMapBytesPerEntry = 4;
		}
		else
		if( mColorMapDepth > 16 )
		{
			mColorMapBytesPerEntry = 3;
		}
		else
		if( mColorMapDepth > 8 )
		{
			mColorMapBytesPerEntry = 2;
		}
		else
		{
			mColorMapBytesPerEntry = 1;
		}
		color_map_bytes = mColorMapLength * mColorMapBytesPerEntry;
		if ( (1 == mImageType) || (9 == mImageType)  )
		{
			mColorMap = new U8[ color_map_bytes ];
			if (!mColorMap)
			{
				LL_ERRS() << "Out of Memory in BOOL LLImageTGA::updateData()" << LL_ENDL;
				return FALSE;
			}
			memcpy( mColorMap, getData() + mDataOffset, color_map_bytes );
		}
		mDataOffset += color_map_bytes;
	}
	S32 height = (S32(mHeightHi) << 8) + mHeightLo;
	S32 width = (S32(mWidthHi) << 8) + mWidthLo;
	S32 bits_per_pixel;
	if( mColorMap )
	{
		bits_per_pixel = mColorMapDepth;
	}
	else
	{
		bits_per_pixel = mPixelSize;
	}
	S32 components;
	switch(bits_per_pixel)
	{
	case 24:
		components = 3;
		break;
	case 32:
		components = 4;
		mAttributeBits = 8;
		break;
	case 15:
	case 16:
		components = 3;
		mIs15Bit = TRUE;
		break;
	case 8:
		components = 1;
		break;
	default:
		setLastError("Unable to load file. Unknown pixel size.");
		return FALSE;
	}
	setSize(width, height, components);
	return TRUE;
}
BOOL LLImageTGA::decode(LLImageRaw* raw_image, F32 decode_time)
{
	llassert_always(raw_image);
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("LLImageTGA trying to decode an image with no data!");
		return FALSE;
	}
	raw_image->resize(getWidth(), getHeight(), getComponents());
	if( (getComponents() != 1) &&
		(getComponents() != 3) &&
		(getComponents() != 4) )
	{
		setLastError("TGA images with a number of components other than 1, 3, and 4 are not supported.");
		return FALSE;
	}
	if( mOriginRightBit )
	{
		setLastError("TGA images with origin on right side are not supported.");
		return FALSE;
	}
	BOOL flipped = (mOriginTopBit != 0);
	BOOL rle_compressed = ((mImageType & 0x08) != 0);
	if( mColorMap )
	{
		return decodeColorMap( raw_image, rle_compressed, flipped );
	}
	else
	{
		return decodeTruecolor( raw_image, rle_compressed, flipped );
	}
}
BOOL LLImageTGA::decodeTruecolor( LLImageRaw* raw_image, BOOL rle, BOOL flipped )
{
	BOOL success = FALSE;
	BOOL alpha_opaque = FALSE;
	if( rle )
	{
		switch( getComponents() )
		{
		case 1:
			success = decodeTruecolorRle8( raw_image );
			break;
		case 3:
			if( mIs15Bit )
			{
				success = decodeTruecolorRle15( raw_image );
			}
			else
			{
				success = decodeTruecolorRle24( raw_image );
			}
			break;
		case 4:
			success = decodeTruecolorRle32( raw_image, alpha_opaque );
			if (alpha_opaque)
			{
				LLPointer<LLImageRaw> compacted_image = new LLImageRaw(raw_image->getWidth(), raw_image->getHeight(), 3);
				compacted_image->copy(raw_image);
				raw_image->resize(raw_image->getWidth(), raw_image->getHeight(), 3);
				raw_image->copy(compacted_image);
			}
			break;
		}
	}
	else
	{
		BOOL alpha_opaque;
		success = decodeTruecolorNonRle( raw_image, alpha_opaque );
		if (alpha_opaque && raw_image->getComponents() == 4)
		{
			LLPointer<LLImageRaw> compacted_image = new LLImageRaw(raw_image->getWidth(), raw_image->getHeight(), 3);
			compacted_image->copy(raw_image);
			raw_image->resize(raw_image->getWidth(), raw_image->getHeight(), 3);
			raw_image->copy(compacted_image);
		}
	}
	if( success && flipped )
	{
		raw_image->verticalFlip();
	}
	return success;
}
BOOL LLImageTGA::decodeTruecolorNonRle( LLImageRaw* raw_image, BOOL &alpha_opaque )
{
	alpha_opaque = TRUE;
	U8* dst = raw_image->getData();
	U8* src = getData() + mDataOffset;
	S32 pixels = getWidth() * getHeight();
	if (pixels * (mIs15Bit ? 2 : getComponents()) > getDataSize() - (S32)mDataOffset)
	{
		return FALSE;
	}
	if (getComponents() == 4)
	{
		while( pixels-- )
		{
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
			dst[3] = src[3];
			if (dst[3] != 255)
			{
				alpha_opaque = FALSE;
			}
			dst += 4;
			src += 4;
		}
	}
	else if (getComponents() == 3)
	{
		if( mIs15Bit )
		{
			while( pixels-- )
			{
				decodeTruecolorPixel15( dst, src );
				dst += 3;
				src += 2;
			}
		}
		else
		{
			while( pixels-- )
			{
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst += 3;
				src += 3;
			}
		}
	}
	else if (getComponents() == 1)
	{
		memcpy(dst, src, pixels);
	}
	return TRUE;
}
void LLImageTGA::decodeColorMapPixel8( U8* dst, const U8* src )
{
	S32 index = llclamp( *src - mColorMapStart, 0, mColorMapLength - 1 );
	dst[0] = mColorMap[ index ];
}
void LLImageTGA::decodeColorMapPixel15( U8* dst, const U8* src )
{
	S32 index = llclamp( *src - mColorMapStart, 0, mColorMapLength - 1 );
	decodeTruecolorPixel15( dst, mColorMap + 2 * index );
}
void LLImageTGA::decodeColorMapPixel24( U8* dst, const U8* src )
{
	S32 index = 3 * llclamp( *src - mColorMapStart, 0, mColorMapLength - 1 );
	dst[0] = mColorMap[ index + 2 ];
	dst[1] = mColorMap[ index + 1 ];
	dst[2] = mColorMap[ index + 0 ];
}
void LLImageTGA::decodeColorMapPixel32( U8* dst, const U8* src )
{
	S32 index = 4 * llclamp( *src - mColorMapStart, 0, mColorMapLength - 1 );
	dst[0] = mColorMap[ index + 2 ];
	dst[1] = mColorMap[ index + 1 ];
	dst[2] = mColorMap[ index + 0 ];
	dst[3] = mColorMap[ index + 3 ];
}
BOOL LLImageTGA::decodeColorMap( LLImageRaw* raw_image, BOOL rle, BOOL flipped )
{
	if( 8 != mPixelSize )
	{
		return FALSE;
	}
	U8* src = getData() + mDataOffset;
	U8* dst = raw_image->getData();
	void (LLImageTGA::*pixel_decoder)( U8*, const U8* );
	switch( mColorMapBytesPerEntry )
	{
		case 1:	pixel_decoder = &LLImageTGA::decodeColorMapPixel8;  break;
		case 2:	pixel_decoder = &LLImageTGA::decodeColorMapPixel15; break;
		case 3:	pixel_decoder = &LLImageTGA::decodeColorMapPixel24; break;
		case 4:	pixel_decoder = &LLImageTGA::decodeColorMapPixel32; break;
		default: llassert(0); return FALSE;
	}
	if( rle )
	{
		U8* last_dst = dst + getComponents() * (getHeight() * getWidth() - 1);
		while( dst <= last_dst )
		{
			U8 block_header_byte = *src;
			src++;
			U8 block_pixel_count = (block_header_byte & 0x7F) + 1;
			if( block_header_byte & 0x80 )
			{
				do
				{
					(this->*pixel_decoder)( dst, src );
					dst += getComponents();
					block_pixel_count--;
				}
				while( block_pixel_count > 0 );
				src++;
			}
			else
			{
				do
				{
					(this->*pixel_decoder)( dst, src );
					dst += getComponents();
					src++;
					block_pixel_count--;
				}
				while( block_pixel_count > 0 );
			}
		}
		raw_image->verticalFlip();
	}
	else
	{
		S32 src_row_bytes = getWidth();
		S32 dst_row_bytes = getWidth() * getComponents();
		if( flipped )
		{
			U8* src_last_row_start = src + (getHeight() - 1) * src_row_bytes;
			src = src_last_row_start;
			src_row_bytes *= -1;
		}
		S32 i;
		S32 j;
		for( S32 row = 0; row < getHeight(); row++ )
		{
			for( i = 0, j = 0; j < getWidth(); i += getComponents(), j++ )
			{
				(this->*pixel_decoder)( dst + i, src + j );
			}
			dst += dst_row_bytes;
			src += src_row_bytes;
		}
	}
	return TRUE;
}
BOOL LLImageTGA::encode(const LLImageRaw* raw_image, F32 encode_time)
{
	llassert_always(raw_image);
	deleteData();
	setSize(raw_image->getWidth(), raw_image->getHeight(), raw_image->getComponents());
	mIDLength = 0;
	mColorMapType = 0;
	switch( getComponents() )
	{
	case 1:
		mImageType = 3;
		break;
	case 2:
	case 3:
	case 4:
		mImageType = 2;
		break;
	default:
		return FALSE;
	}
	mColorMapIndexLo = 0;
	mColorMapIndexHi = 0;
	mColorMapLengthLo = 0;
	mColorMapLengthHi = 0;
	mColorMapDepth = 0;
	mXOffsetLo = 0;
	mXOffsetHi = 0;
	mYOffsetLo = 0;
	mYOffsetHi = 0;
	mWidthLo = U8(getWidth() & 0xFF);
	mWidthHi = U8((getWidth() >> 8) & 0xFF);
	mHeightLo = U8(getHeight() & 0xFF);
	mHeightHi = U8((getHeight() >> 8) & 0xFF);
	S32 bytes_per_pixel;
	switch( getComponents() )
	{
	case 1:
		bytes_per_pixel = 1;
		break;
	case 3:
		bytes_per_pixel = 3;
		break;
	case 2:
	case 4:
		bytes_per_pixel = 4;
		break;
	default:
		return FALSE;
	}
	mPixelSize = U8(bytes_per_pixel * 8);
	mAttributeBits = (4 == bytes_per_pixel) ? 8 : 0;
	mOriginRightBit = 0;
	mOriginTopBit = 0;
	mInterleave = 0;
	const S32 TGA_HEADER_SIZE = 18;
	const S32 COLOR_MAP_SIZE = 0;
	mDataOffset = TGA_HEADER_SIZE + mIDLength + COLOR_MAP_SIZE;
	S32 pixels = getWidth() * getHeight();
	S32 datasize = mDataOffset + bytes_per_pixel * pixels;
	U8* dst = allocateData(datasize);
	*(dst++) = mIDLength;
	*(dst++) = mColorMapType;
	*(dst++) = mImageType;
	*(dst++) = mColorMapIndexLo;
	*(dst++) = mColorMapIndexHi;
	*(dst++) = mColorMapLengthLo;
	*(dst++) = mColorMapLengthHi;
	*(dst++) = mColorMapDepth;
	*(dst++) = mXOffsetLo;
	*(dst++) = mXOffsetHi;
	*(dst++) = mYOffsetLo;
	*(dst++) = mYOffsetHi;
	*(dst++) = mWidthLo;
	*(dst++) = mWidthHi;
	*(dst++) = mHeightLo;
	*(dst++) = mHeightHi;
	*(dst++) = mPixelSize;
	*(dst++) =
		((mInterleave & 3) << 5) |
		((mOriginTopBit & 1) << 4) |
		((mOriginRightBit & 1) << 3) |
		((mAttributeBits & 0xF) << 0);
	const U8* src = raw_image->getData();
	llassert( dst == getData() + mDataOffset );
	S32 i = 0;
	S32 j = 0;
	switch( getComponents() )
	{
	case 1:
		memcpy( dst, src, bytes_per_pixel * pixels );
		break;
	case 2:
		while( pixels-- )
		{
			dst[i + 0] = src[j + 0];
			dst[i + 1] = src[j + 0];
			dst[i + 2] = src[j + 0];
			dst[i + 3] = src[j + 1];
			i += 4;
			j += 2;
		}
		break;
	case 3:
		while( pixels-- )
		{
			dst[i + 0] = src[i + 2];
			dst[i + 1] = src[i + 1];
			dst[i + 2] = src[i + 0];
			i += 3;
		}
		break;
	case 4:
		while( pixels-- )
		{
			dst[i + 0] = src[i + 2];
			dst[i + 1] = src[i + 1];
			dst[i + 2] = src[i + 0];
			dst[i + 3] = src[i + 3];
			i += 4;
		}
		break;
	}
	return TRUE;
}
BOOL LLImageTGA::decodeTruecolorRle32( LLImageRaw* raw_image, BOOL &alpha_opaque )
{
	llassert( getComponents() == 4 );
	alpha_opaque = TRUE;
	U8* dst = raw_image->getData();
	U32* dst_pixels = (U32*) dst;
	U8* src = getData() + mDataOffset;
	U8* last_src = src + getDataSize();
	U32 rgba;
	U8* rgba_byte_p = (U8*) &rgba;
	U32* last_dst_pixel = dst_pixels + getHeight() * getWidth() - 1;
	while( dst_pixels <= last_dst_pixel )
	{
		if (src >= last_src)
			return FALSE;
		U8 block_header_byte = *src;
		src++;
		U32 block_pixel_count = (block_header_byte & 0x7F) + 1;
		if( block_header_byte & 0x80 )
		{
			if (src + 3 >= last_src)
				return FALSE;
			rgba_byte_p[0] = src[2];
			rgba_byte_p[1] = src[1];
			rgba_byte_p[2] = src[0];
			rgba_byte_p[3] = src[3];
			if (rgba_byte_p[3] != 255)
			{
				alpha_opaque = FALSE;
			}
			src += 4;
			register U32 value = rgba;
			do
			{
				*dst_pixels = value;
				dst_pixels++;
				block_pixel_count--;
			}
			while( block_pixel_count > 0 );
		}
		else
		{
			do
			{
				if (src + 3 >= last_src)
					return FALSE;
				((U8*)dst_pixels)[0] = src[2];
				((U8*)dst_pixels)[1] = src[1];
				((U8*)dst_pixels)[2] = src[0];
				((U8*)dst_pixels)[3] = src[3];
				if (src[3] != 255)
				{
					alpha_opaque = FALSE;
				}
				src += 4;
				dst_pixels++;
				block_pixel_count--;
			}
			while( block_pixel_count > 0 );
		}
	}
	return TRUE;
}
BOOL LLImageTGA::decodeTruecolorRle15( LLImageRaw* raw_image )
{
	llassert( getComponents() == 3 );
	llassert( mIs15Bit );
	U8* dst = raw_image->getData();
	U8* src = getData() + mDataOffset;
	U8* last_src = src + getDataSize();
	U8* last_dst = dst + getComponents() * (getHeight() * getWidth() - 1);
	while( dst <= last_dst )
	{
		if (src >= last_src)
			return FALSE;
		U8 block_header_byte = *src;
		src++;
		U8 block_pixel_count = (block_header_byte & 0x7F) + 1;
		if( block_header_byte & 0x80 )
		{
			do
			{
				if (src + 2 >= last_src)
					return FALSE;
				decodeTruecolorPixel15( dst, src );
				dst += 3;
				block_pixel_count--;
			}
			while( block_pixel_count > 0 );
			src += 2;
		}
		else
		{
			do
			{
				if (src + 2 >= last_src)
					return FALSE;
				decodeTruecolorPixel15( dst, src );
				dst += 3;
				src += 2;
				block_pixel_count--;
			}
			while( block_pixel_count > 0 );
		}
	}
	return TRUE;
}
BOOL LLImageTGA::decodeTruecolorRle24( LLImageRaw* raw_image )
{
	llassert( getComponents() == 3 );
	U8* dst = raw_image->getData();
	U8* src = getData() + mDataOffset;
	U8* last_src = src + getDataSize();
	U8* last_dst = dst + getComponents() * (getHeight() * getWidth() - 1);
	while( dst <= last_dst )
	{
		if (src >= last_src)
			return FALSE;
		U8 block_header_byte = *src;
		src++;
		U8 block_pixel_count = (block_header_byte & 0x7F) + 1;
		if( block_header_byte & 0x80 )
		{
			do
			{
				if (src + 2 >= last_src)
					return FALSE;
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst += 3;
				block_pixel_count--;
			}
			while( block_pixel_count > 0 );
			src += 3;
		}
		else
		{
			do
			{
				if (src + 2 >= last_src)
					return FALSE;
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst += 3;
				src += 3;
				block_pixel_count--;
			}
			while( block_pixel_count > 0 );
		}
	}
	return TRUE;
}
BOOL LLImageTGA::decodeTruecolorRle8( LLImageRaw* raw_image )
{
	llassert( getComponents() == 1 );
	U8* dst = raw_image->getData();
	U8* src = getData() + mDataOffset;
	U8* last_src = src + getDataSize();
	U8* last_dst = dst + getHeight() * getWidth() - 1;
	while( dst <= last_dst )
	{
		if (src >= last_src)
			return FALSE;
		U8 block_header_byte = *src;
		src++;
		U8 block_pixel_count = (block_header_byte & 0x7F) + 1;
		if( block_header_byte & 0x80 )
		{
			if (src >= last_src)
				return FALSE;
			memset( dst, *src, block_pixel_count );
			dst += block_pixel_count;
			src++;
		}
		else
		{
			do
			{
				if (src >= last_src)
					return FALSE;
				*dst = *src;
				dst++;
				src++;
				block_pixel_count--;
			}
			while( block_pixel_count > 0 );
		}
	}
	return TRUE;
}
BOOL LLImageTGA::decodeAndProcess( LLImageRaw* raw_image, F32 domain, F32 weight )
{
	llassert_always(raw_image);
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("LLImageTGA trying to decode an image with no data!");
		return FALSE;
	}
	if( (getComponents() != 1) || (mImageType != 11) || mOriginTopBit || mOriginRightBit )
	{
		LL_ERRS() << "LLImageTGA trying to alpha-gradient process an image that's not a standard RLE, one component image" << LL_ENDL;
		return FALSE;
	}
	raw_image->resize(getWidth(), getHeight(), getComponents());
	U8* dst = raw_image->getData();
	U8* src = getData() + mDataOffset;
	U8* last_dst = dst + getHeight() * getWidth() - 1;
	if( domain > 0 )
	{
		const S32 LUT_LEN = 256;
		U8 lut[LUT_LEN];
		S32 i;
		F32 scale = 1.f / domain;
		F32 offset = (1.f - domain) * llclampf( 1.f - weight );
		F32 bias = -(scale * offset);
		for( i = 0; i < LUT_LEN; i++ )
		{
			lut[i] = (U8)llclampb( 255.f * ( i/255.f * scale + bias ) );
		}
		while( dst <= last_dst )
		{
			U8 block_header_byte = *src;
			src++;
			U8 block_pixel_count = (block_header_byte & 0x7F) + 1;
			if( block_header_byte & 0x80 )
			{
				memset( dst, lut[ *src ], block_pixel_count );
				dst += block_pixel_count;
				src++;
			}
			else
			{
				do
				{
					*dst = lut[ *src ];
					dst++;
					src++;
					block_pixel_count--;
				}
				while( block_pixel_count > 0 );
			}
		}
	}
	else
	{
		const U8 threshold = (U8)(0xFF * llclampf( 1.f - weight ));
		while( dst <= last_dst )
		{
			U8 block_header_byte = *src;
			src++;
			U8 block_pixel_count = (block_header_byte & 0x7F) + 1;
			if( block_header_byte & 0x80 )
			{
				memset( dst, ((*src >= threshold) ? 0xFF : 0), block_pixel_count );
				dst += block_pixel_count;
				src++;
			}
			else
			{
				do
				{
					*dst = (*src >= threshold) ? 0xFF : 0;
					dst++;
					src++;
					block_pixel_count--;
				}
				while( block_pixel_count > 0 );
			}
		}
	}
	return TRUE;
}
bool LLImageTGA::loadFile( const std::string& path )
{
	S32 len = path.size();
	if( len < 5 )
	{
		return false;
	}
	std::string extension = gDirUtilp->getExtension(path);
	if( "tga" != extension )
	{
		return false;
	}
	LLFILE* file = LLFile::fopen(path, "rb");
	if( !file )
	{
		LL_WARNS() << "Couldn't open file " << path << LL_ENDL;
		return false;
	}
	size_t file_size = 0;
	if (!fseek(file, 0, SEEK_END))
	{
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
	}
	U8* buffer = allocateData(file_size);
	S32 bytes_read = fread(buffer, 1, file_size, file);
	if( bytes_read != file_size )
	{
		deleteData();
		LL_WARNS() << "Couldn't read file " << path << LL_ENDL;
		return false;
	}
	fclose( file );
	if( !updateData() )
	{
		LL_WARNS() << "Couldn't decode file " << path << LL_ENDL;
		deleteData();
		return false;
	}
	return true;
}
