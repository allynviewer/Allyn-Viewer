/** 
 * @file llpanelmsgs.h
 * @brief Message popup preferences panel
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#ifndef LL_VIEWERMEDIAFOCUS_H
#define LL_VIEWERMEDIAFOCUS_H
#include "llfocusmgr.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llselectmgr.h"
class LLViewerMediaImpl;
class LLPanelPrimMediaControls;
class LLViewerMediaFocus :
	public LLFocusableElement,
	public LLSingleton<LLViewerMediaFocus>
{
public:
	LLViewerMediaFocus();
	~LLViewerMediaFocus();
	void setFocusFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);
	void clearFocus();
	void setHoverFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);
	void clearHover();
	bool	getFocus();
	BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	BOOL	handleKeyUp(KEY key, MASK mask, BOOL called_from_parent);
	BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	void update();
	static void setCameraZoom(LLViewerObject* object, LLVector3 normal, F32 padding_factor, bool zoom_in_only = false);
	static F32 getBBoxAspectRatio(const LLBBox& bbox, const LLVector3& normal, F32* height, F32* width, F32* depth);
	bool isFocusedOnFace(LLPointer<LLViewerObject> objectp, S32 face);
	bool isHoveringOverFace(LLPointer<LLViewerObject> objectp, S32 face);
	LLViewerMediaImpl* getFocusedMediaImpl();
	LLViewerObject* getFocusedObject();
	S32 getFocusedFace() { return mFocusedObjectFace; }
	LLUUID getFocusedObjectID() { return mFocusedObjectID; }
	LLViewerMediaImpl* getHoverMediaImpl();
	LLViewerObject* getHoverObject();
	S32 getHoverFace() { return mHoverObjectFace; }
	void focusZoomOnMedia(LLUUID media_id);
	bool isZoomed() const;
	void unZoom();
	LLUUID getControlsMediaID();
    virtual bool    wantsKeyUpKeyDown() const;
    virtual bool    wantsReturnKey() const;
protected:
	void	onFocusReceived();
	void	onFocusLost();
private:
	LLHandle<LLPanelPrimMediaControls> mMediaControls;
	LLObjectSelectionHandle mSelection;
	LLUUID mFocusedObjectID;
	S32 mFocusedObjectFace;
	LLUUID mFocusedImplID;
	LLVector3 mFocusedObjectNormal;
	LLUUID mHoverObjectID;
	S32 mHoverObjectFace;
	LLUUID mHoverImplID;
	LLVector3 mHoverObjectNormal;
};
#endif
