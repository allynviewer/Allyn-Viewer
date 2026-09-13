/** 
 * @file lltoolmorph.h
 * @brief A tool to select object faces.
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
#ifndef LL_LLTOOLMORPH_H
#define LL_LLTOOLMORPH_H
#include "lltool.h"
#include "m4math.h"
#include "v2math.h"
#include "lldynamictexture.h"
#include "llundo.h"
#include "lltextbox.h"
#include "llstrider.h"
#include "llviewervisualparam.h"
#include "llframetimer.h"
#include "llviewertexture.h"
#include "lluiimage.h"
class LLViewerJointMesh;
class LLPolyMesh;
class LLViewerObject;
class LLVisualParamHint : public LLViewerDynamicTexture
{
protected:
	virtual ~LLVisualParamHint();
public:
	LLVisualParamHint(
		S32 pos_x, S32 pos_y,
		S32 width, S32 height,
		LLViewerJointMesh *mesh,
		LLViewerVisualParam *param,
		LLWearable *wearable,
		F32 param_weight);
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
	S8 getType() const ;
	BOOL					needsRender();
	void					preRender(BOOL clear_depth);
	BOOL					render();
	void					requestUpdate( S32 delay_frames ) {mNeedsUpdate = TRUE; mDelayFrames = delay_frames; }
	void					setUpdateDelayFrames( S32 delay_frames ) { mDelayFrames = delay_frames; }
	void					draw();
	LLViewerVisualParam*	getVisualParam() { return mVisualParam; }
	F32						getVisualParamWeight() { return mVisualParamWeight; }
	BOOL					getVisible() { return mIsVisible; }
	void					setAllowsUpdates( BOOL b ) { mAllowsUpdates = b; }
	const LLRect&			getRect()	{ return mRect; }
	static void				requestHintUpdates( LLVisualParamHint* exception1 = NULL, LLVisualParamHint* exception2 = NULL );
protected:
	BOOL					mNeedsUpdate;
	BOOL					mIsVisible;
	LLViewerJointMesh*		mJointMesh;
	LLViewerVisualParam*	mVisualParam;
	LLWearable*				mWearablePtr;
	F32						mVisualParamWeight;
	BOOL					mAllowsUpdates;
	S32						mDelayFrames;
	LLRect					mRect;
	F32						mLastParamWeight;
	LLUIImagePtr mBackgroundp;
	typedef std::set<LLVisualParamHint*> instance_list_t;
	static instance_list_t sInstances;
};
class LLVisualParamReset : public LLViewerDynamicTexture
{
protected:
	~LLVisualParamReset(){}
public:
	LLVisualParamReset();
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}
	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}
	BOOL render();
	S8 getType() const ;
	static BOOL sDirty;
};
#endif
