/** 
 * @file lltoolselectland.h
 * @brief LLToolSelectLand class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef LL_LLTOOLSELECTLAND_H
#define LL_LLTOOLSELECTLAND_H
#include "lltool.h"
#include "v3dmath.h"
class LLParcelSelection;
class LLToolSelectLand
:	public LLTool, public LLSingleton<LLToolSelectLand>
{
public:
	LLToolSelectLand( );
	virtual ~LLToolSelectLand();
	BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);
	BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL		handleHover(S32 x, S32 y, MASK mask);
	void		render();
	BOOL		isAlwaysRendered()		{ return TRUE; }
	void		handleSelect();
	void		handleDeselect();
protected:
	BOOL			outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);
	void			roundXY(LLVector3d& vec);
protected:
	LLVector3d		mDragStartGlobal;
	LLVector3d		mDragEndGlobal;
	BOOL			mDragEndValid;
	S32				mDragStartX;
	S32				mDragStartY;
	S32				mDragEndX;
	S32				mDragEndY;
	BOOL			mMouseOutsideSlop;
	LLVector3d		mWestSouthBottom;
	LLVector3d		mEastNorthTop;
	LLSafeHandle<LLParcelSelection> mSelection;
};
#endif
