/** 
 * @file lltoolgrab.h
 * @brief LLToolGrab class header file
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
#ifndef LL_TOOLGRAB_H
#define LL_TOOLGRAB_H
#include "lltool.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llmemory.h"
#include "lluuid.h"
#include "llviewerwindow.h"
class LLView;
class LLTextBox;
class LLViewerObject;
class LLPickInfo;
void send_ObjectGrab_message(LLViewerObject* object, bool grab, const LLPickInfo* const pick = nullptr, const LLVector3& grab_offset = LLVector3::zero);
class LLToolGrab : public LLTool, public LLSingleton<LLToolGrab>
{
public:
	LLToolGrab( LLToolComposite* composite = NULL );
	~LLToolGrab();
	BOOL	handleHover(S32 x, S32 y, MASK mask);
	BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	void	render();
	void	draw();
	virtual void		handleSelect();
	virtual void		handleDeselect();
	virtual LLViewerObject*	getEditingObject();
	virtual LLVector3d		getEditingPointGlobal();
	virtual BOOL			isEditing();
	virtual void			stopEditing();
	virtual void			onMouseCaptureLost();
	BOOL			hasGrabOffset()  { return TRUE; }
	LLVector3		getGrabOffset(S32 x, S32 y);
	BOOL			handleObjectHit(const LLPickInfo& info);
	BOOL getHideBuildHighlight() { return mHideBuildHighlight; }
	static void		pickCallback(const LLPickInfo& pick_info);
private:
	LLVector3d		getGrabPointGlobal();
	void			startGrab();
	void			stopGrab();
	void			startSpin();
	void			stopSpin();
	void			handleHoverSpin(S32 x, S32 y, MASK mask);
	void			handleHoverActive(S32 x, S32 y, MASK mask);
	void			handleHoverNonPhysical(S32 x, S32 y, MASK mask);
	void			handleHoverInactive(S32 x, S32 y, MASK mask);
	void			handleHoverFailed(S32 x, S32 y, MASK mask);
private:
	enum			EGrabMode { GRAB_INACTIVE, GRAB_ACTIVE_CENTER, GRAB_NONPHYSICAL, GRAB_LOCKED, GRAB_NOOBJECT };
	EGrabMode		mMode;
	BOOL			mVerticalDragging;
	BOOL			mHitLand;
	LLTimer			mGrabTimer;
	LLVector3		mGrabOffsetFromCenterInitial;
	LLVector3d		mGrabHiddenOffsetFromCamera;
	LLVector3d		mDragStartPointGlobal;
	LLVector3d		mDragStartFromCamera;
	LLPickInfo		mGrabPick;
	S32				mLastMouseX;
	S32				mLastMouseY;
	S32				mAccumDeltaX;
	S32				mAccumDeltaY;
	BOOL			mHasMoved;
	BOOL			mOutsideSlop;
	BOOL			mDeselectedThisClick;
	S32             mLastFace;
	LLVector2       mLastUVCoords;
	LLVector2       mLastSTCoords;
	LLVector3       mLastIntersection;
	LLVector3       mLastNormal;
	LLVector3       mLastBinormal;
	LLVector3       mLastGrabPos;
	BOOL			mSpinGrabbing;
	LLQuaternion	mSpinRotation;
	BOOL			mHideBuildHighlight;
};
extern BOOL gGrabBtnVertical;
extern BOOL gGrabBtnSpin;
extern LLTool* gGrabTransientTool;
#endif
