/** 
 * @file patch_dct.h
 * @brief Function declarations for DCT and IDCT routines
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#ifndef LL_PATCH_DCT_H
#define LL_PATCH_DCT_H
class LLVector3;
const U8 ZERO_CODE	= 0x0;
const U8 ZERO_EOB = 0x2;
const U8 POSITIVE_VALUE = 0x6;
const U8 NEGATIVE_VALUE = 0x7;
const S8 NORMAL_PATCH_SIZE	= 16;
const S8 LARGE_PATCH_SIZE = 32;
const U8 END_OF_PATCHES = 97;
#define _PATCH_SIZE_16_AND_32_ONLY
class LLGroupHeader
{
public:
	U16	stride;
	U8	patch_size;
	U8	layer_type;
};
class LLPatchHeader
{
public:
	F32	dc_offset;
	U16	range;
	U8	quant_wbits;
	U32	patchids;
};
void init_patch_compressor(S32 patch_size, S32 patch_stride, S32 layer_type);
void prescan_patch(F32 *patch, LLPatchHeader *php, F32 &zmax, F32 &zmin);
void compress_patch(F32 *patch, S32 *cpatch, LLPatchHeader *php, S32 prequant);
void get_patch_group_header(LLGroupHeader *gopp);
void set_group_of_patch_header(LLGroupHeader *gopp);
void init_patch_decompressor(S32 size);
void decompress_patch(F32 *patch, S32 *cpatch, LLPatchHeader *ph);
void decompress_patchv(LLVector3 *v, S32 *cpatch, LLPatchHeader *ph);
#endif
