/** 
 * @file llpatchvertexarray.cpp
 * @brief Implementation of the LLSurfaceVertexArray class.
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
#include "llviewerprecompiledheaders.h"
#include "llpatchvertexarray.h"
#include "llsurfacepatch.h"
LLPatchVertexArray::LLPatchVertexArray() :
	mSurfaceWidth(0),
	mPatchWidth(0),
	mPatchOrder(0),
	mRenderLevelp(NULL),
	mRenderStridep(NULL)
{
}
LLPatchVertexArray::LLPatchVertexArray(U32 surface_width, U32 patch_width, F32 meters_per_grid) :
	mRenderLevelp(NULL),
	mRenderStridep(NULL)
{
	create(surface_width, patch_width, meters_per_grid);
}
LLPatchVertexArray::~LLPatchVertexArray()
{
	destroy();
}
void LLPatchVertexArray::create(U32 surface_width, U32 patch_width, F32 meters_per_grid)
{
	if (patch_width > surface_width)
	{
		U32 temp = patch_width;
		patch_width = surface_width;
		surface_width = temp;
	}
	U32 power_of_two = 1;
	U32 surface_order = 0;
	while (power_of_two < (surface_width-1))
	{
		power_of_two *= 2;
		surface_order += 1;
	}
	if (power_of_two != (surface_width-1))
		surface_width = power_of_two + 1;
	{
		mSurfaceWidth = surface_width;
		U32 ratio = (surface_width - 1) / patch_width;
		F32 fratio = ((float)(surface_width - 1)) / ((float)(patch_width));
		if ( fratio == (float)(ratio))
		{
			power_of_two = 1;
			U32 patch_order = 0;
			while (power_of_two < patch_width)
			{
				power_of_two *= 2;
				patch_order += 1;
			}
			if (power_of_two != patch_width)
				patch_width = power_of_two;
			mPatchWidth = patch_width;
			mPatchOrder = patch_order;
		}
		else
		{
			mPatchWidth = 0;
			mPatchOrder = 0;
		}
	}
	if (mPatchWidth > 0)
	{
		mRenderLevelp = new U32 [2*mPatchWidth + 1];
		mRenderStridep = new U32 [mPatchOrder + 1];
	}
	if (NULL == mRenderLevelp || NULL == mRenderStridep)
	{
		LL_ERRS() << "mRenderLevelp or mRenderStridep was NULL; we'd crash soon." << LL_ENDL;
		return;
	}
	init();
}
void LLPatchVertexArray::destroy()
{
	if (mPatchWidth == 0)
	{
		return;
	}
	delete [] mRenderLevelp;
	delete [] mRenderStridep;
	mSurfaceWidth = 0;
	mPatchWidth = 0;
	mPatchOrder = 0;
}
void LLPatchVertexArray::init()
{
	U32 j;
	U32 level, stride;
	U32 k;
	stride = mPatchWidth;
	for (level=0; level<mPatchOrder + 1; level++)
	{
		mRenderStridep[level] = stride;
		stride /= 2;
	}
	level = mPatchOrder;
	k = 2;
	mRenderLevelp[0] = mPatchOrder;
	mRenderLevelp[1] = mPatchOrder;
	stride = 2;
	while(stride < 2*mPatchWidth)
	{
		for (j=0; j<k  &&  stride<2*mPatchWidth; j++)
		{
			mRenderLevelp[stride++] = level;
		}
		k *= 2;
		level--;
	}
	mRenderLevelp[2*mPatchWidth] = 0;
}
std::ostream& operator<<(std::ostream &s, const LLPatchVertexArray &va)
{
	U32 i;
	s << "{ \n";
	s << "  mSurfaceWidth = " << va.mSurfaceWidth << "\n";
	s << "  mPatchWidth = " << va.mPatchWidth << "\n";
	s << "  mPatchOrder = " << va.mPatchOrder << "\n";
	s << "  mRenderStridep = \n";
	for (i=0; i<va.mPatchOrder+1; i++)
	{
		s << "    " << i << "    " << va.mRenderStridep[i] << "\n";
	}
	s << "  mRenderLevelp = \n";
	for (i=0; i < 2*va.mPatchWidth + 1; i++)
	{
		s << "    " << i << "    " << va.mRenderLevelp[i] << "\n";
	}
	s << "}";
	return s;
}
