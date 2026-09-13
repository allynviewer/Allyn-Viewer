/*
 * @file llimagepng.cpp
 * @brief LLImageFormatted glue to encode / decode PNG files.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "stdtypes.h"
#include "llerror.h"
#include "llimage.h"
#include "llpngwrapper.h"
#include "llimagepng.h"
LLImagePNG::LLImagePNG()
    : LLImageFormatted(IMG_CODEC_PNG)
{
}
LLImagePNG::~LLImagePNG()
{
}
BOOL LLImagePNG::updateData()
{
    resetLastError();
    if (!getData() || (0 == getDataSize()))
    {
        setLastError("Uninitialized instance of LLImagePNG");
        return FALSE;
    }
	LLPngWrapper pngWrapper;
	if (!pngWrapper.isValidPng(getData()))
	{
		setLastError("LLImagePNG data does not have a valid PNG header!");
		return FALSE;
	}
	LLPngWrapper::ImageInfo infop;
	if (! pngWrapper.readPng(getData(), getDataSize(), NULL, &infop))
	{
		setLastError(pngWrapper.getErrorMessage());
		return FALSE;
	}
	setSize(infop.mWidth, infop.mHeight, infop.mComponents);
	return TRUE;
}
BOOL LLImagePNG::decode(LLImageRaw* raw_image, F32 decode_time)
{
	llassert_always(raw_image);
    resetLastError();
    if (!getData() || (0 == getDataSize()))
    {
        setLastError("LLImagePNG trying to decode an image with no data!");
        return FALSE;
    }
	LLPngWrapper pngWrapper;
	if (!pngWrapper.isValidPng(getData()))
	{
		setLastError("LLImagePNG data does not have a valid PNG header!");
		return FALSE;
	}
	if (! pngWrapper.readPng(getData(), getDataSize(), raw_image))
	{
		setLastError(pngWrapper.getErrorMessage());
		return FALSE;
	}
	return TRUE;
}
BOOL LLImagePNG::encode(const LLImageRaw* raw_image, F32 encode_time)
{
	llassert_always(raw_image);
    resetLastError();
	setSize(raw_image->getWidth(), raw_image->getHeight(), raw_image->getComponents());
	U32 bufferSize = getWidth() * getHeight() * getComponents() + 1024;
    U8* tmpWriteBuffer = new U8[ bufferSize ];
	LLPngWrapper pngWrapper;
	if (! pngWrapper.writePng(raw_image, tmpWriteBuffer))
	{
		setLastError(pngWrapper.getErrorMessage());
		delete[] tmpWriteBuffer;
		return FALSE;
	}
	U32 encodedSize = pngWrapper.getFinalSize();
	allocateData(encodedSize);
	memcpy(getData(), tmpWriteBuffer, encodedSize);
	delete[] tmpWriteBuffer;
	return TRUE;
}
