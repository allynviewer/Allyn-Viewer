/** 
 * @file llviewermediafocus.cpp
 * @brief Governs focus on Media prims
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
#include "llviewerprecompiledheaders.h"
#include "llviewermediafocus.h"
#include "llviewerobjectlist.h"
#include "llpanelprimmediacontrols.h"
#include "llpluginclassmedia.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewermedia.h"
#include "llhudview.h"
#include "lluictrlfactory.h"
#include "lldrawable.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "llweb.h"
#include "llmediaentry.h"
#include "llkeyboard.h"
#include "lltoolmgr.h"
#include "llvovolume.h"
LLViewerMediaFocus::LLViewerMediaFocus()
:	mFocusedObjectFace(0),
	mHoverObjectFace(0)
{
}
LLViewerMediaFocus::~LLViewerMediaFocus()
{
}
void LLViewerMediaFocus::setFocusFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal)
{
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	LLViewerMediaImpl *old_media_impl = getFocusedMediaImpl();
	if(old_media_impl)
	{
		old_media_impl->focus(false);
	}
	LLSelectMgr::getInstance()->deselectAll();
	mSelection = NULL;
	if (media_impl.notNull() && objectp.notNull())
	{
		bool face_auto_zoom = false;
		mFocusedImplID = media_impl->getMediaTextureID();
		mFocusedObjectID = objectp->getID();
		mFocusedObjectFace = face;
		mFocusedObjectNormal = pick_normal;
		mSelection = LLSelectMgr::getInstance()->selectObjectOnly(objectp, face);
		media_impl->setDisabled(false);
		LLTextureEntry* tep = objectp->getTE(face);
		if(tep->hasMedia())
		{
			LLMediaEntry* mep = tep->getMediaData();
			face_auto_zoom = mep->getAutoZoom();
			if(!media_impl->hasMedia())
			{
				std::string url = mep->getCurrentURL().empty() ? mep->getHomeURL() : mep->getCurrentURL();
				media_impl->navigateTo(url, "", true);
			}
		}
		else
		{
			LL_WARNS() << "Can't find media entry for focused face" << LL_ENDL;
		}
		media_impl->focus(true);
		gFocusMgr.setKeyboardFocus(this);
		LLViewerMediaImpl* impl = getFocusedMediaImpl();
		if (impl)
		{
			LLEditMenuHandler::gEditMenuHandler = impl;
		}
		update();
		if(mMediaControls.get())
		{
			if(face_auto_zoom && ! parcel->getMediaPreventCameraZoom())
			{
				mMediaControls.get()->resetZoomLevel(false);
				mMediaControls.get()->nextZoomLevel();
			}
			else
			{
				mMediaControls.get()->resetZoomLevel(false);
			}
		}
	}
	else
	{
		if(hasFocus())
		{
			gFocusMgr.setKeyboardFocus(NULL);
		}
		LLViewerMediaImpl* impl = getFocusedMediaImpl();
		if (LLEditMenuHandler::gEditMenuHandler == impl)
		{
			LLEditMenuHandler::gEditMenuHandler = NULL;
		}
		mFocusedImplID = LLUUID::null;
		if (objectp.notNull())
		{
			mFocusedObjectID = objectp->getID();
			mFocusedObjectFace = face;
		}
		else {
			mFocusedObjectID = LLUUID::null;
			mFocusedObjectFace = 0;
		}
	}
}
void LLViewerMediaFocus::clearFocus()
{
	setFocusFace(NULL, 0, NULL);
}
void LLViewerMediaFocus::setHoverFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal)
{
	if (media_impl.notNull())
	{
		mHoverImplID = media_impl->getMediaTextureID();
		mHoverObjectID = objectp->getID();
		mHoverObjectFace = face;
		mHoverObjectNormal = pick_normal;
	}
	else
	{
		mHoverObjectID = LLUUID::null;
		mHoverObjectFace = 0;
		mHoverImplID = LLUUID::null;
	}
}
void LLViewerMediaFocus::clearHover()
{
	setHoverFace(NULL, 0, NULL);
}
bool LLViewerMediaFocus::getFocus()
{
	if (gFocusMgr.getKeyboardFocus() == this)
	{
		return true;
	}
	return false;
}
void LLViewerMediaFocus::setCameraZoom(LLViewerObject* object, LLVector3 normal, F32 padding_factor, bool zoom_in_only)
{
	if (object)
	{
		gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
		LLBBox bbox = object->getBoundingBoxAgent();
		LLVector3d center = gAgent.getPosGlobalFromAgent(bbox.getCenterAgent());
		F32 height;
		F32 width;
		F32 depth;
		F32 angle_of_view;
		F32 distance;
		F32 aspect_ratio = getBBoxAspectRatio(bbox, normal, &height, &width, &depth);
		F32 camera_aspect = LLViewerCamera::getInstance()->getAspect();
		LL_DEBUGS() << "normal = " << normal << ", aspect_ratio = " << aspect_ratio << ", camera_aspect = " << camera_aspect << LL_ENDL;
		bool invert = (camera_aspect > 1.0f && aspect_ratio > camera_aspect) ||
			(camera_aspect < 1.0f && aspect_ratio < camera_aspect);
		if(camera_aspect < 1.0f || invert)
		{
			angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect());
			distance = width * 0.5 * padding_factor / tan(angle_of_view * 0.5f );
			LL_DEBUGS() << "using width (" << width << "), angle_of_view = " << angle_of_view << ", distance = " << distance << LL_ENDL;
		}
		else
		{
			angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getView());
			distance = height * 0.5 * padding_factor / tan(angle_of_view * 0.5f );
			LL_DEBUGS() << "using height (" << height << "), angle_of_view = " << angle_of_view << ", distance = " << distance << LL_ENDL;
		}
		distance += depth * 0.5;
		LLVector3d camera_pos, target_pos;
		target_pos = center;
		LLVector3d pickNormal = LLVector3d(normal);
		pickNormal.normalize();
        camera_pos = target_pos + pickNormal * distance;
        if (pickNormal == LLVector3d::z_axis || pickNormal == LLVector3d::z_axis_neg)
        {
            LLVector3d cur_camera_pos = LLVector3d(gAgentCamera.getCameraPositionGlobal());
            LLVector3d delta = (cur_camera_pos - camera_pos);
            F64 len = delta.length();
            delta.normalize();
            camera_pos += 0.01 * len * delta;
        }
		if (zoom_in_only &&
		    (dist_vec_squared(gAgentCamera.getCameraPositionGlobal(), target_pos) < dist_vec_squared(camera_pos, target_pos)))
		{
			return;
		}
		gAgentCamera.setCameraPosAndFocusGlobal(camera_pos, target_pos, object->getID() );
	}
	else
	{
		gAgentCamera.setFocusOnAvatar(TRUE, ANIMATE);
	}
}
void LLViewerMediaFocus::onFocusReceived()
{
	LLViewerMediaImpl* media_impl = getFocusedMediaImpl();
	if(media_impl)
		media_impl->focus(true);
	LLFocusableElement::onFocusReceived();
}
void LLViewerMediaFocus::onFocusLost()
{
	LLViewerMediaImpl* media_impl = getFocusedMediaImpl();
	if(media_impl)
		media_impl->focus(false);
	gViewerWindow->focusClient();
	LLFocusableElement::onFocusLost();
}
BOOL LLViewerMediaFocus::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	LLViewerMediaImpl* media_impl = getFocusedMediaImpl();
	if(media_impl)
	{
		media_impl->handleKeyHere(key, mask);
		if (KEY_ESCAPE == key)
		{
			if(mFocusedImplID.notNull())
			{
				if(mMediaControls.get())
				{
					mMediaControls.get()->resetZoomLevel(true);
				}
			}
			clearFocus();
		}
	}
	return true;
}
BOOL LLViewerMediaFocus::handleKeyUp(KEY key, MASK mask, BOOL called_from_parent)
{
    LLViewerMediaImpl* media_impl = getFocusedMediaImpl();
    if (media_impl)
    {
        media_impl->handleKeyUpHere(key, mask);
    }
    return true;
}
BOOL LLViewerMediaFocus::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	LLViewerMediaImpl* media_impl = getFocusedMediaImpl();
	if(media_impl)
		media_impl->handleUnicodeCharHere(uni_char);
	return true;
}
BOOL LLViewerMediaFocus::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	BOOL retval = FALSE;
	LLViewerMediaImpl* media_impl = getFocusedMediaImpl();
	if(media_impl && media_impl->hasMedia())
	{
		media_impl->scrollWheel(x, y, 0, clicks, gKeyboard->currentMask(TRUE));
		retval = TRUE;
	}
	return retval;
}
void LLViewerMediaFocus::update()
{
	if(mFocusedImplID.notNull())
	{
		if(!getFocus())
		{
			if(mMediaControls.get() && mMediaControls.get()->hasFocus())
			{
			}
			else
			{
				clearFocus();
			}
		}
		else if(LLToolMgr::getInstance()->inBuildMode())
		{
			clearFocus();
		}
	}
	LLViewerMediaImpl *media_impl = getFocusedMediaImpl();
	LLViewerObject *viewer_object = getFocusedObject();
	S32 face = mFocusedObjectFace;
	LLVector3 normal = mFocusedObjectNormal;
	if(!media_impl || !viewer_object)
	{
		media_impl = getHoverMediaImpl();
		viewer_object = getHoverObject();
		face = mHoverObjectFace;
		normal = mHoverObjectNormal;
	}
	if(media_impl && viewer_object && !media_impl->isForcedUnloaded())
	{
		if(! mMediaControls.get())
		{
			LLPanelPrimMediaControls* media_controls = new LLPanelPrimMediaControls();
			mMediaControls = media_controls->getHandle();
			gHUDView->addChild(media_controls);
		}
		mMediaControls.get()->setMediaFace(viewer_object, face, media_impl, normal);
	}
	else
	{
		if(mMediaControls.get())
		{
			mMediaControls.get()->setMediaFace(NULL, 0, NULL);
		}
	}
}
F32 LLViewerMediaFocus::getBBoxAspectRatio(const LLBBox& bbox, const LLVector3& normal, F32* height, F32* width, F32* depth)
{
	LLVector3 local_normal = bbox.agentToLocalBasis(normal);
	LLVector3 z_vec = bbox.agentToLocalBasis(LLVector3(0.0f, 0.0f, 1.0f));
	LLVector3 comp1(0.f,0.f,0.f);
	LLVector3 comp2(0.f,0.f,0.f);
	LLVector3 bbox_max = bbox.getExtentLocal();
	F32 dot1 = 0.f;
	F32 dot2 = 0.f;
	LL_DEBUGS() << "bounding box local size = " << bbox_max << ", local_normal = " << local_normal << LL_ENDL;
	local_normal.abs();
	bool XgtY = (local_normal.mV[VX] > local_normal.mV[VY]);
	bool XgtZ = (local_normal.mV[VX] > local_normal.mV[VZ]);
	bool YgtZ = (local_normal.mV[VY] > local_normal.mV[VZ]);
	if(XgtY && XgtZ)
	{
		LL_DEBUGS() << "x component of normal is longest, using y and z" << LL_ENDL;
		comp1.mV[VY] = bbox_max.mV[VY];
		comp2.mV[VZ] = bbox_max.mV[VZ];
		*depth = bbox_max.mV[VX];
	}
	else if(!XgtY && YgtZ)
	{
		LL_DEBUGS() << "y component of normal is longest, using x and z" << LL_ENDL;
		comp1.mV[VX] = bbox_max.mV[VX];
		comp2.mV[VZ] = bbox_max.mV[VZ];
		*depth = bbox_max.mV[VY];
	}
	else
	{
		LL_DEBUGS() << "z component of normal is longest, using x and y" << LL_ENDL;
		comp1.mV[VX] = bbox_max.mV[VX];
		comp2.mV[VY] = bbox_max.mV[VY];
		*depth = bbox_max.mV[VZ];
	}
	dot1 = comp1 * z_vec;
	dot2 = comp2 * z_vec;
	if(fabs(dot1) > fabs(dot2))
	{
		*height = comp1.length();
		*width = comp2.length();
		LL_DEBUGS() << "comp1 = " << comp1 << ", height = " << *height << LL_ENDL;
		LL_DEBUGS() << "comp2 = " << comp2 << ", width = " << *width << LL_ENDL;
	}
	else
	{
		*height = comp2.length();
		*width = comp1.length();
		LL_DEBUGS() << "comp2 = " << comp2 << ", height = " << *height << LL_ENDL;
		LL_DEBUGS() << "comp1 = " << comp1 << ", width = " << *width << LL_ENDL;
	}
	LL_DEBUGS() << "returning " << (*width / *height) << LL_ENDL;
	return *width / *height;
}
bool LLViewerMediaFocus::isFocusedOnFace(LLPointer<LLViewerObject> objectp, S32 face)
{
	return objectp->getID() == mFocusedObjectID && face == mFocusedObjectFace;
}
bool LLViewerMediaFocus::isHoveringOverFace(LLPointer<LLViewerObject> objectp, S32 face)
{
	return objectp->getID() == mHoverObjectID && face == mHoverObjectFace;
}
LLViewerMediaImpl* LLViewerMediaFocus::getFocusedMediaImpl()
{
	return LLViewerMedia::getMediaImplFromTextureID(mFocusedImplID);
}
LLViewerObject* LLViewerMediaFocus::getFocusedObject()
{
	return gObjectList.findObject(mFocusedObjectID);
}
LLViewerMediaImpl* LLViewerMediaFocus::getHoverMediaImpl()
{
	return LLViewerMedia::getMediaImplFromTextureID(mHoverImplID);
}
LLViewerObject* LLViewerMediaFocus::getHoverObject()
{
	return gObjectList.findObject(mHoverObjectID);
}
void LLViewerMediaFocus::focusZoomOnMedia(LLUUID media_id)
{
	LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(media_id);
	if(impl)
	{
		LLVOVolume *obj = impl->getSomeObject();
		if(obj)
		{
			S32 face = obj->getFaceIndexWithMediaImpl(impl, -1);
			LLVector3 normal = obj->getApproximateFaceNormal(face);
			if(normal.isNull())
			{
				normal = LLViewerCamera::getInstance()->getAtAxis();
				normal *= (F32)-1.0f;
			}
			setFocusFace(obj, face, impl, normal);
			if(mMediaControls.get())
			{
				mMediaControls.get()->resetZoomLevel();
				mMediaControls.get()->nextZoomLevel();
			}
		}
	}
}
void LLViewerMediaFocus::unZoom()
{
	if(mMediaControls.get())
	{
		mMediaControls.get()->resetZoomLevel();
	}
}
bool LLViewerMediaFocus::isZoomed() const
{
	return (mMediaControls.get() && mMediaControls.get()->getZoomLevel() != LLPanelPrimMediaControls::ZOOM_NONE);
}
LLUUID LLViewerMediaFocus::getControlsMediaID()
{
	if(getFocusedMediaImpl())
	{
		return mFocusedImplID;
	}
	else if(getHoverMediaImpl())
	{
		return mHoverImplID;
	}
	return LLUUID::null;
}
bool LLViewerMediaFocus::wantsKeyUpKeyDown() const
{
    return true;
}
bool LLViewerMediaFocus::wantsReturnKey() const
{
    return true;
}
