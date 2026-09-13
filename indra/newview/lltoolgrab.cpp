/** 
 * @file lltoolgrab.cpp
 * @brief LLToolGrab class implementation
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
#include "llviewerprecompiledheaders.h"
#include "lltoolgrab.h"
#include "indra_constants.h"
#include "llviewercontrol.h"
#include "llquaternion.h"
#include "llbox.h"
#include "message.h"
#include "llview.h"
#include "llfontgl.h"
#include "llui.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "lldrawable.h"
#include "llfloatertools.h"
#include "llhudeffect.h"
#include "llhudmanager.h"
#include "llregionposition.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "hippolimits.h"
#include "rlvhandler.h"
const S32 SLOP_DIST_SQ = 4;
BOOL gGrabBtnVertical = FALSE;
BOOL gGrabBtnSpin = FALSE;
LLTool* gGrabTransientTool = NULL;
extern BOOL gDebugClicks;
LLToolGrab::LLToolGrab( LLToolComposite* composite )
:	LLTool( std::string("Grab"), composite ),
	mMode( GRAB_INACTIVE ),
	mVerticalDragging( FALSE ),
	mHitLand(FALSE),
	mLastMouseX(0),
	mLastMouseY(0),
	mAccumDeltaX(0),
	mAccumDeltaY(0),
	mHasMoved( FALSE ),
	mOutsideSlop(FALSE),
	mDeselectedThisClick(FALSE),
	mLastFace(0),
	mSpinGrabbing( FALSE ),
	mSpinRotation(),
	mHideBuildHighlight(FALSE)
{ }
LLToolGrab::~LLToolGrab()
{ }
void LLToolGrab::handleSelect()
{
	if(gFloaterTools)
	{
		gFloaterTools->setStatusText("grab");
	}
	gGrabBtnVertical = FALSE;
	gGrabBtnSpin = FALSE;
}
void LLToolGrab::handleDeselect()
{
	if( hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}
}
BOOL LLToolGrab::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		LL_INFOS() << "LLToolGrab handleDoubleClick (becoming mouseDown)" << LL_ENDL;
	}
	return FALSE;
}
BOOL LLToolGrab::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		LL_INFOS() << "LLToolGrab handleMouseDown" << LL_ENDL;
	}
	LLTool::handleMouseDown(x, y, mask);
	if (!gAgent.leftButtonGrabbed())
	{
		gViewerWindow->pickAsync(x, y, mask, pickCallback, TRUE, FALSE, FALSE, TRUE);
	}
	return TRUE;
}
void LLToolGrab::pickCallback(const LLPickInfo& pick_info)
{
	LLToolGrab::getInstance()->mGrabPick = pick_info;
	LLViewerObject	*objectp = pick_info.getObject();
	BOOL extend_select = (pick_info.mKeyMask & MASK_SHIFT);
	if (!extend_select && !LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		LLSelectMgr::getInstance()->deselectAll();
		LLToolGrab::getInstance()->mDeselectedThisClick = TRUE;
	}
	else
	{
		LLToolGrab::getInstance()->mDeselectedThisClick = FALSE;
	}
	if ( (!objectp) || ((rlv_handler_t::isEnabled()) && (!gRlvHandler.canTouch(objectp, pick_info.mObjectOffset))) )
	{
		LLToolGrab::getInstance()->setMouseCapture(TRUE);
		LLToolGrab::getInstance()->mMode = GRAB_NOOBJECT;
		LLToolGrab::getInstance()->mGrabPick.mObjectID.setNull();
	}
	else
	{
		LLToolGrab::getInstance()->handleObjectHit(LLToolGrab::getInstance()->mGrabPick);
	}
}
BOOL LLToolGrab::handleObjectHit(const LLPickInfo& info)
{
	mGrabPick = info;
	LLViewerObject* objectp = mGrabPick.getObject();
	if (gDebugClicks)
	{
		LL_INFOS() << "LLToolGrab handleObjectHit " << info.mMousePt.mX << "," << info.mMousePt.mY << LL_ENDL;
	}
	if (NULL == objectp)
	{
		LL_WARNS() << "objectp was NULL; returning FALSE" << LL_ENDL;
		return FALSE;
	}
	if (objectp->isAvatar())
	{
		if (gGrabTransientTool)
		{
			gBasicToolset->selectTool( gGrabTransientTool );
			gGrabTransientTool = NULL;
		}
		return TRUE;
	}
	setMouseCapture( TRUE );
	LLViewerObject* parent = objectp->getRootEdit();
	BOOL script_touch = (objectp->flagHandleTouch()) || (parent && parent->flagHandleTouch());
	mHideBuildHighlight = script_touch || objectp->flagUsePhysics();
	if (!objectp->flagUsePhysics())
	{
		if (script_touch)
		{
			mMode = GRAB_NONPHYSICAL;
		}
		else
		{
			if (gAgentCamera.cameraMouselook())
			{
				mMode = GRAB_LOCKED;
				gViewerWindow->hideCursor();
				gViewerWindow->moveCursorToCenter();
			}
			else if (objectp->permMove() && !objectp->isPermanentEnforced())
			{
				mMode = GRAB_ACTIVE_CENTER;
				gViewerWindow->hideCursor();
				gViewerWindow->moveCursorToCenter();
			}
			else
			{
				mMode = GRAB_LOCKED;
			}
		}
	}
	else if( objectp->flagCharacter() || !objectp->permMove() || objectp->isPermanentEnforced())
	{
		mMode = GRAB_LOCKED;
	}
	else
	{
		mMode = GRAB_ACTIVE_CENTER;
		gViewerWindow->hideCursor();
		gViewerWindow->moveCursorToCenter();
	}
	mLastMouseX = gViewerWindow->getCurrentMouseX();
	mLastMouseY = gViewerWindow->getCurrentMouseY();
	mAccumDeltaX = 0;
	mAccumDeltaY = 0;
	mHasMoved = FALSE;
	mOutsideSlop = FALSE;
	mVerticalDragging = (info.mKeyMask == MASK_VERTICAL) || gGrabBtnVertical;
	startGrab();
	if ((info.mKeyMask == MASK_SPIN) || gGrabBtnSpin)
	{
		startSpin();
	}
	LLSelectMgr::getInstance()->updateSelectionCenter();
	LLViewerObject *edit_object = info.getObject();
	if (edit_object && info.mPickType != LLPickInfo::PICK_FLORA)
	{
		LLVector3 local_edit_point = gAgent.getPosAgentFromGlobal(info.mPosGlobal);
		local_edit_point -= edit_object->getPositionAgent();
		local_edit_point = local_edit_point * ~edit_object->getRenderRotation();
		gAgentCamera.setPointAt(POINTAT_TARGET_GRAB, edit_object, local_edit_point );
		gAgentCamera.setLookAt(LOOKAT_TARGET_SELECT, edit_object, local_edit_point );
	}
	if (!gViewerWindow->getLeftMouseDown()
		&& gGrabTransientTool
		&& (mMode == GRAB_NONPHYSICAL || mMode == GRAB_LOCKED))
	{
		gBasicToolset->selectTool( gGrabTransientTool );
		gGrabTransientTool = NULL;
	}
	return TRUE;
}
void LLToolGrab::startSpin()
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}
	mSpinGrabbing = TRUE;
	LLViewerObject *root = (LLViewerObject *)objectp->getRoot();
	mSpinRotation = root->getRotation();
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectSpinStart);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addUUIDFast(_PREHASH_ObjectID, mGrabPick.mObjectID );
	msg->sendMessage( objectp->getRegion()->getHost() );
}
void LLToolGrab::stopSpin()
{
	mSpinGrabbing = FALSE;
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}
	LLMessageSystem *msg = gMessageSystem;
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		msg->newMessageFast(_PREHASH_ObjectSpinStop);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
		msg->sendMessage( objectp->getRegion()->getHost() );
		break;
	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		break;
	}
}
void LLToolGrab::startGrab()
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}
	LLViewerObject *root = (LLViewerObject *)objectp->getRoot();
	LLVector3d grab_start_global = root->getPositionGlobal();
	LLVector3d grab_offsetd = root->getPositionGlobal() - objectp->getPositionGlobal();
	LLVector3 grab_offset;
	grab_offset.setVec(grab_offsetd);
	LLQuaternion rotation = root->getRotation();
	rotation.conjQuat();
	grab_offset = grab_offset * rotation;
	mDragStartPointGlobal = grab_start_global;
	mDragStartFromCamera = grab_start_global - gAgentCamera.getCameraPositionGlobal();
	send_ObjectGrab_message(objectp, true, &mGrabPick, grab_offset);
	mGrabOffsetFromCenterInitial = grab_offset;
	mGrabHiddenOffsetFromCamera = mDragStartFromCamera;
	mGrabTimer.reset();
	mLastUVCoords = mGrabPick.mUVCoords;
	mLastSTCoords = mGrabPick.mSTCoords;
	mLastFace = mGrabPick.mObjectFace;
	mLastIntersection = mGrabPick.mIntersection;
	mLastNormal = mGrabPick.mNormal;
	mLastBinormal = mGrabPick.mBinormal;
	mLastGrabPos = LLVector3(-1.f, -1.f, -1.f);
}
BOOL LLToolGrab::handleHover(S32 x, S32 y, MASK mask)
{
	if (!gViewerWindow->getLeftMouseDown())
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
		setMouseCapture(FALSE);
		return TRUE;
	}
	if ( (rlv_handler_t::isEnabled()) && (GRAB_INACTIVE != mMode) && (GRAB_NOOBJECT != mMode) && (hasMouseCapture()) &&
		 (gRlvHandler.hasBehaviour(RLV_BHVR_FARTOUCH)) && (!gRlvHandler.canTouch(mGrabPick.getObject(), mGrabPick.mObjectOffset)) )
	{
		if (gGrabTransientTool)
		{
			gBasicToolset->selectTool(gGrabTransientTool);
			gGrabTransientTool = NULL;
		}
		setMouseCapture(FALSE);
		return TRUE;
	}
	switch( mMode )
	{
	case GRAB_ACTIVE_CENTER:
		handleHoverActive( x, y, mask );
		break;
	case GRAB_NONPHYSICAL:
		handleHoverNonPhysical(x, y, mask);
		break;
	case GRAB_INACTIVE:
		handleHoverInactive( x, y, mask );
		break;
	case GRAB_NOOBJECT:
	case GRAB_LOCKED:
		handleHoverFailed( x, y, mask );
		break;
	}
	mLastMouseX = x;
	mLastMouseY = y;
	return TRUE;
}
const F32 GRAB_SENSITIVITY_X = 0.0075f;
const F32 GRAB_SENSITIVITY_Y = 0.0075f;
void LLToolGrab::handleHoverActive(S32 x, S32 y, MASK mask)
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp || !hasMouseCapture() ) return;
	if (objectp->isDead())
	{
		setMouseCapture(FALSE);
		return;
	}
	if (mSpinGrabbing && !(mask == MASK_SPIN) && !gGrabBtnSpin)
	{
		stopSpin();
	}
	else if (!mSpinGrabbing && (mask == MASK_SPIN) )
	{
		startSpin();
	}
	if (mVerticalDragging && !(mask == MASK_VERTICAL) && !gGrabBtnVertical)
	{
		mVerticalDragging = FALSE;
		mDragStartPointGlobal = gViewerWindow->clickPointInWorldGlobal(x, y, objectp);
		mDragStartFromCamera = mDragStartPointGlobal - gAgentCamera.getCameraPositionGlobal();
	}
	else if (!mVerticalDragging && (mask == MASK_VERTICAL) )
	{
		mVerticalDragging = TRUE;
		mDragStartPointGlobal = gViewerWindow->clickPointInWorldGlobal(x, y, objectp);
		mDragStartFromCamera = mDragStartPointGlobal - gAgentCamera.getCameraPositionGlobal();
	}
	const F32 RADIANS_PER_PIXEL_X = 0.01f;
	const F32 RADIANS_PER_PIXEL_Y = 0.01f;
	S32 dx = x - (gViewerWindow->getWorldViewWidthScaled() / 2);
	S32 dy = y - (gViewerWindow->getWorldViewHeightScaled() / 2);
	if (dx != 0 || dy != 0)
	{
		mAccumDeltaX += dx;
		mAccumDeltaY += dy;
		S32 dist_sq = mAccumDeltaX * mAccumDeltaX + mAccumDeltaY * mAccumDeltaY;
		if (dist_sq > SLOP_DIST_SQ)
		{
			mOutsideSlop = TRUE;
		}
		mHasMoved = TRUE;
		if (mSpinGrabbing)
		{
			LLVector3 up(0.f, 0.f, 1.f);
			LLQuaternion rotation_around_vertical( dx*RADIANS_PER_PIXEL_X, up );
			const LLVector3 &agent_left = LLViewerCamera::getInstance()->getLeftAxis();
			LLQuaternion rotation_around_left( dy*RADIANS_PER_PIXEL_Y, agent_left );
			mSpinRotation = mSpinRotation * rotation_around_vertical;
			mSpinRotation = mSpinRotation * rotation_around_left;
			LLMessageSystem *msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_ObjectSpinUpdate);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
			msg->addQuatFast(_PREHASH_Rotation, mSpinRotation );
			msg->sendMessage( objectp->getRegion()->getHost() );
		}
		else
		{
			LLVector3d x_part;
			x_part.setVec(LLViewerCamera::getInstance()->getLeftAxis());
			x_part.mdV[VZ] = 0.0;
			x_part.normVec();
			LLVector3d y_part;
			if( mVerticalDragging )
			{
				y_part.setVec(LLViewerCamera::getInstance()->getUpAxis());
			}
			else
			{
				y_part = x_part % LLVector3d::z_axis;
				y_part.mdV[VZ] = 0.0;
				y_part.normVec();
			}
			mGrabHiddenOffsetFromCamera = mGrabHiddenOffsetFromCamera
				+ (x_part * (-dx * GRAB_SENSITIVITY_X))
				+ (y_part * ( dy * GRAB_SENSITIVITY_Y));
			F32 dt = mGrabTimer.getElapsedTimeAndResetF32();
			U32 dt_milliseconds = (U32) (1000.f * dt);
			LLVector3d grab_point_global;
			grab_point_global = gAgentCamera.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;
			F32 land_height = LLWorld::getInstance()->resolveLandHeightGlobal(grab_point_global);
			if (grab_point_global.mdV[VZ] < land_height)
			{
				grab_point_global.mdV[VZ] = land_height;
			}
			if (grab_point_global.mdV[VZ] > gHippoLimits->getMaxHeight())
			{
				grab_point_global.mdV[VZ] = gHippoLimits->getMaxHeight();
			}
			grab_point_global = LLWorld::getInstance()->clipToVisibleRegions(mDragStartPointGlobal, grab_point_global);
			mGrabHiddenOffsetFromCamera = grab_point_global - gAgentCamera.getCameraPositionGlobal();
			LLVector3 grab_pos_agent = gAgent.getPosAgentFromGlobal( grab_point_global );
			LLCoordGL grab_center_gl( gViewerWindow->getWorldViewWidthScaled() / 2, gViewerWindow->getWorldViewHeightScaled() / 2);
			LLViewerCamera::getInstance()->projectPosAgentToScreen(grab_pos_agent, grab_center_gl);
			const S32 ROTATE_H_MARGIN = gViewerWindow->getWorldViewWidthScaled() / 20;
			const F32 ROTATE_ANGLE_PER_SECOND = 30.f * DEG_TO_RAD;
			const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;
			if (grab_center_gl.mX < ROTATE_H_MARGIN)
			{
				if (gAgentCamera.getFocusOnAvatar())
				{
					gAgent.yaw(rotate_angle);
				}
				else
				{
					gAgentCamera.cameraOrbitAround(rotate_angle);
				}
			}
			else if (grab_center_gl.mX > gViewerWindow->getWorldViewWidthScaled() - ROTATE_H_MARGIN)
			{
				if (gAgentCamera.getFocusOnAvatar())
				{
					gAgent.yaw(-rotate_angle);
				}
				else
				{
					gAgentCamera.cameraOrbitAround(-rotate_angle);
				}
			}
			if ((grab_center_gl.mY < gViewerWindow->getWorldViewHeightScaled() - 6)
				&& (grab_center_gl.mY > 24))
			{
				LLVector3 grab_pos_region = objectp->getRegion()->getPosRegionFromGlobal( grab_point_global );
				LLMessageSystem *msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_ObjectGrabUpdate);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
				msg->addVector3Fast(_PREHASH_GrabOffsetInitial, mGrabOffsetFromCenterInitial );
				msg->addVector3Fast(_PREHASH_GrabPosition, grab_pos_region );
				msg->addU32Fast(_PREHASH_TimeSinceLast, dt_milliseconds );
				msg->nextBlock("SurfaceInfo");
				msg->addVector3("UVCoord", LLVector3(mGrabPick.mUVCoords));
				msg->addVector3("STCoord", LLVector3(mGrabPick.mSTCoords));
				msg->addS32Fast(_PREHASH_FaceIndex, mGrabPick.mObjectFace);
				msg->addVector3("Position", mGrabPick.mIntersection);
				msg->addVector3("Normal", mGrabPick.mNormal);
				msg->addVector3("Binormal", mGrabPick.mBinormal);
				msg->sendMessage( objectp->getRegion()->getHost() );
			}
		}
		gViewerWindow->moveCursorToCenter();
		LLSelectMgr::getInstance()->updateSelectionCenter();
	}
	if (mHasMoved)
	{
		if (!gAgentCamera.cameraMouselook() &&
			!objectp->isHUDAttachment() &&
			objectp->getRoot() == gAgentAvatarp->getRoot())
		{
			gAgentCamera.setFocusGlobal(gAgentCamera.calcFocusPositionTargetGlobal(), LLUUID::null);
			gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
		}
		else
		{
			gAgentCamera.clearFocusObject();
		}
	}
	gViewerWindow->hideCursor();
	gViewerWindow->setCursor(UI_CURSOR_ARROW);
	LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (active) [cursor hidden]" << LL_ENDL;
}
void LLToolGrab::handleHoverNonPhysical(S32 x, S32 y, MASK mask)
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp || !hasMouseCapture() ) return;
	if (objectp->isDead())
	{
		setMouseCapture(FALSE);
		return;
	}
	LLPickInfo pick = mGrabPick;
	pick.mMousePt = LLCoordGL(x, y);
	pick.getSurfaceInfo();
	F32 dt = mGrabTimer.getElapsedTimeAndResetF32();
	U32 dt_milliseconds = (U32) (1000.f * dt);
	LLVector3 grab_pos_region(0,0,0);
	const BOOL SUPPORT_LLDETECTED_GRAB = TRUE;
	if (SUPPORT_LLDETECTED_GRAB)
	{
		if (mVerticalDragging && !(mask == MASK_VERTICAL) && !gGrabBtnVertical)
		{
			mVerticalDragging = FALSE;
		}
		else if (!mVerticalDragging && (mask == MASK_VERTICAL) )
		{
			mVerticalDragging = TRUE;
		}
		S32 dx = x - mLastMouseX;
		S32 dy = y - mLastMouseY;
		if (dx != 0 || dy != 0)
		{
			mAccumDeltaX += dx;
			mAccumDeltaY += dy;
			S32 dist_sq = mAccumDeltaX * mAccumDeltaX + mAccumDeltaY * mAccumDeltaY;
			if (dist_sq > SLOP_DIST_SQ)
			{
				mOutsideSlop = TRUE;
			}
			mHasMoved = TRUE;
			LLVector3d x_part;
			x_part.setVec(LLViewerCamera::getInstance()->getLeftAxis());
			x_part.mdV[VZ] = 0.0;
			x_part.normVec();
			LLVector3d y_part;
			if( mVerticalDragging )
			{
				y_part.setVec(LLViewerCamera::getInstance()->getUpAxis());
			}
			else
			{
				y_part = x_part % LLVector3d::z_axis;
				y_part.mdV[VZ] = 0.0;
				y_part.normVec();
			}
			mGrabHiddenOffsetFromCamera = mGrabHiddenOffsetFromCamera
				+ (x_part * (-dx * GRAB_SENSITIVITY_X))
				+ (y_part * ( dy * GRAB_SENSITIVITY_Y));
		}
		LLVector3d grab_point_global = gAgentCamera.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;
		grab_pos_region = objectp->getRegion()->getPosRegionFromGlobal( grab_point_global );
	}
	BOOL changed_since_last_update = FALSE;
	if ((pick.mObjectFace != mLastFace) ||
		(pick.mUVCoords != mLastUVCoords) ||
		(pick.mSTCoords != mLastSTCoords) ||
		(pick.mIntersection != mLastIntersection) ||
		(pick.mNormal != mLastNormal) ||
		(pick.mBinormal != mLastBinormal) ||
		(grab_pos_region != mLastGrabPos))
	{
		changed_since_last_update = TRUE;
	}
	if (changed_since_last_update)
	{
		LLMessageSystem *msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ObjectGrabUpdate);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addUUIDFast(_PREHASH_ObjectID, objectp->getID() );
		msg->addVector3Fast(_PREHASH_GrabOffsetInitial, mGrabOffsetFromCenterInitial );
		msg->addVector3Fast(_PREHASH_GrabPosition, grab_pos_region );
		msg->addU32Fast(_PREHASH_TimeSinceLast, dt_milliseconds );
		msg->nextBlock("SurfaceInfo");
		msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
		msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
		msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
		msg->addVector3("Position", pick.mIntersection);
		msg->addVector3("Normal", pick.mNormal);
		msg->addVector3("Binormal", pick.mBinormal);
		msg->sendMessage( objectp->getRegion()->getHost() );
		mLastUVCoords = pick.mUVCoords;
		mLastSTCoords = pick.mSTCoords;
		mLastFace = pick.mObjectFace;
		mLastIntersection = pick.mIntersection;
		mLastNormal= pick.mNormal;
		mLastBinormal= pick.mBinormal;
		mLastGrabPos = grab_pos_region;
	}
	if (pick.mObjectFace != -1)
	{
		LLVector3 local_edit_point = pick.mIntersection;
		local_edit_point -= objectp->getPositionAgent();
		local_edit_point = local_edit_point * ~objectp->getRenderRotation();
		gAgentCamera.setPointAt(POINTAT_TARGET_GRAB, objectp, local_edit_point );
		gAgentCamera.setLookAt(LOOKAT_TARGET_SELECT, objectp, local_edit_point );
	}
	gViewerWindow->setCursor(UI_CURSOR_HAND);
}
void LLToolGrab::handleHoverInactive(S32 x, S32 y, MASK mask)
{
	const F32 ROTATE_ANGLE_PER_SECOND = 40.f * DEG_TO_RAD;
	const F32 rotate_angle = ROTATE_ANGLE_PER_SECOND / gFPSClamped;
	if (gSavedSettings.getBOOL("FullScreen"))
	{
		if (gAgentCamera.cameraThirdPerson() )
		{
			if (x == 0)
			{
				gAgent.yaw(rotate_angle);
			}
			else if (x == (gViewerWindow->getWindowWidth() - 1) )
			{
				gAgent.yaw(-rotate_angle);
			}
		}
	}
	LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (inactive-not over editable object)" << LL_ENDL;
	gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
}
void LLToolGrab::handleHoverFailed(S32 x, S32 y, MASK mask)
{
	if( GRAB_NOOBJECT == mMode )
	{
		gViewerWindow->setCursor(UI_CURSOR_NO);
		LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (not on object)" << LL_ENDL;
	}
	else
	{
		S32 dist_sq = (x-mGrabPick.mMousePt.mX) * (x-mGrabPick.mMousePt.mX) + (y-mGrabPick.mMousePt.mY) * (y-mGrabPick.mMousePt.mY);
		if( mOutsideSlop || dist_sq > SLOP_DIST_SQ )
		{
			mOutsideSlop = TRUE;
			switch( mMode )
			{
			case GRAB_LOCKED:
				gViewerWindow->setCursor(UI_CURSOR_GRABLOCKED);
				LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (grab failed, no move permission)" << LL_ENDL;
				break;
			default:
				llassert(0);
			}
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
			LL_DEBUGS("UserInput") << "hover handled by LLToolGrab (grab failed but within slop)" << LL_ENDL;
		}
	}
}
BOOL LLToolGrab::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLTool::handleMouseUp(x, y, mask);
	if( hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}
	mMode = GRAB_INACTIVE;
	if (gGrabTransientTool)
	{
		gBasicToolset->selectTool( gGrabTransientTool );
		gGrabTransientTool = NULL;
	}
	return TRUE;
}
void LLToolGrab::stopEditing()
{
	if( hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}
}
void LLToolGrab::onMouseCaptureLost()
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		gViewerWindow->showCursor();
		return;
	}
	if( !gAgentCamera.cameraMouselook()
		&& (GRAB_ACTIVE_CENTER == mMode))
	{
		if (objectp->isHUDAttachment())
		{
			S32 x = mGrabPick.mMousePt.mX + mAccumDeltaX;
			S32 y = mGrabPick.mMousePt.mY + mAccumDeltaY;
			LLUI::setMousePositionScreen(x, y);
		}
		else if (mHasMoved)
		{
			LLVector3 grab_point_agent = objectp->getRenderPosition();
			LLCoordGL gl_point;
			if (LLViewerCamera::getInstance()->projectPosAgentToScreen(grab_point_agent, gl_point))
			{
				LLUI::setMousePositionScreen(gl_point.mX, gl_point.mY);
			}
		}
		else
		{
			LLUI::setMousePositionScreen(mGrabPick.mMousePt.mX, mGrabPick.mMousePt.mY);
		}
		gViewerWindow->showCursor();
	}
	stopGrab();
	if (mSpinGrabbing)
	stopSpin();
	mMode = GRAB_INACTIVE;
	mHideBuildHighlight = FALSE;
	mGrabPick.mObjectID.setNull();
	LLSelectMgr::getInstance()->updateSelectionCenter();
	gAgentCamera.setPointAt(POINTAT_TARGET_CLEAR);
	gAgentCamera.setLookAt(LOOKAT_TARGET_CLEAR);
	dialog_refresh_all();
}
void LLToolGrab::stopGrab()
{
	LLViewerObject* objectp = mGrabPick.getObject();
	if (!objectp)
	{
		return;
	}
	LLPickInfo pick = mGrabPick;
	if (mMode == GRAB_NONPHYSICAL)
	{
		S32 x = gViewerWindow->getCurrentMouseX();
		S32 y = gViewerWindow->getCurrentMouseY();
		pick.mMousePt = LLCoordGL(x, y);
		pick.getSurfaceInfo();
	}
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		send_ObjectGrab_message(objectp, false, &pick);
		mVerticalDragging = FALSE;
		break;
	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		break;
	}
	mHideBuildHighlight = FALSE;
}
void LLToolGrab::draw()
{ }
void LLToolGrab::render()
{ }
BOOL LLToolGrab::isEditing()
{
	return (mGrabPick.getObject().notNull());
}
LLViewerObject* LLToolGrab::getEditingObject()
{
	return mGrabPick.getObject();
}
LLVector3d LLToolGrab::getEditingPointGlobal()
{
	return getGrabPointGlobal();
}
LLVector3d LLToolGrab::getGrabPointGlobal()
{
	switch(mMode)
	{
	case GRAB_ACTIVE_CENTER:
	case GRAB_NONPHYSICAL:
	case GRAB_LOCKED:
		return gAgentCamera.getCameraPositionGlobal() + mGrabHiddenOffsetFromCamera;
	case GRAB_NOOBJECT:
	case GRAB_INACTIVE:
	default:
		return gAgent.getPositionGlobal();
	}
}
void send_ObjectGrab_message(LLViewerObject* object, bool grab, const LLPickInfo* const pick, const LLVector3 &grab_offset)
{
	if (!object) return;
	LLMessageSystem	*msg = gMessageSystem;
	msg->newMessageFast(grab ? _PREHASH_ObjectGrab : _PREHASH_ObjectDeGrab);
	msg->nextBlockFast( _PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast( _PREHASH_ObjectData);
	msg->addU32Fast(    _PREHASH_LocalID, object->mLocalID);
	if (grab) msg->addVector3Fast(_PREHASH_GrabOffset, grab_offset);
	if (pick)
	{
		msg->nextBlock("SurfaceInfo");
		msg->addVector3("UVCoord", LLVector3(pick->mUVCoords));
		msg->addVector3("STCoord", LLVector3(pick->mSTCoords));
		msg->addS32Fast(_PREHASH_FaceIndex, pick->mObjectFace);
		msg->addVector3("Position", pick->mIntersection);
		msg->addVector3("Normal", pick->mNormal);
		msg->addVector3("Binormal", pick->mBinormal);
	}
	msg->sendMessage( object->getRegion()->getHost());
}
