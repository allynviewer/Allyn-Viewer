/** 
 * @file llpatchvertexarray.h
 * @brief description of Surface class
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
#ifndef LL_LLPATCHVERTEXARRAY_H
#define LL_LLPATCHVERTEXARRAY_H
class LLPatchVertexArray
{
public:
	LLPatchVertexArray();
	LLPatchVertexArray(U32 surface_width, U32 patch_width, F32 meters_per_grid);
	virtual ~LLPatchVertexArray();
	void create(U32 surface_width, U32 patch_width, F32 meters_per_grid);
	void destroy();
	void init();
public:
	U32 mSurfaceWidth;
	U32 mPatchWidth;
	U32 mPatchOrder;
	U32 *mRenderLevelp;
	U32 *mRenderStridep;
};
#endif
