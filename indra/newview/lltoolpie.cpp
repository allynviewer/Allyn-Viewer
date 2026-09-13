/** 
 * @file lltoolpie.cpp
 * @brief LLToolPie class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
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
#include "lltoolpie.h"
#include "indra_constants.h"
#include "llclickaction.h"
#include "llparcel.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfocusmgr.h"
#include "llfirstuse.h"
#include "llfloaterland.h"
#include "llfloaterscriptdebug.h"
#include "llhoverview.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llmediaentry.h"
#include "llmenugl.h"
#include "llmutelist.h"
#include "llselectmgr.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolselect.h"
#include "lltrans.h"
#include "llviewercamera.h"
#include "llviewerparcelmedia.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llviewermedia.h"
#include "llvoavatarself.h"
#include "llviewermediafocus.h"
#include "llworld.h"
#include "llui.h"
#include "llweb.h"
#include "rlvhandler.h"
extern BOOL gDebugClicks;
static void handle_click_action_play();
static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp);
static ECursorType cursor_from_parcel_media(U8 click_action);
LLToolPie::LLToolPie()
:	LLTool(std::string("Pie")),
	mMouseButtonDown( false ),
	mMouseOutsideSlop( false ),
	mMouseSteerX(-1),
	mMouseSteerY(-1),
	mBlockClickToWalk(false),
	mClickAction(0),
	mClickActionBuyEnabled("ClickActionBuyEnabled"),
	mClickActionPayEnabled("ClickActionPayEnabled")
{
}
BOOL LLToolPie::handleAnyMouseClick(S32 x, S32 y, MASK mask, EClickType clicktype, BOOL down)
{
	BOOL result = LLMouseHandler::handleAnyMouseClick(x, y, mask, clicktype, down);
	return result;
}
BOOL LLToolPie::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseOutsideSlop = FALSE;
	mMouseDownX = x;
	mMouseDownY = y;
    BOOL pick_rigged = true;
	mPick = gViewerWindow->pickImmediate(x, y, TRUE, pick_rigged);
	mPick.mKeyMask = mask;
	mMouseButtonDown = true;
	handleLeftClickPick();
	return TRUE;
}
BOOL LLToolPie::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	mPick = gViewerWindow->pickImmediate(x, y,
                                         FALSE,
                                         mask != MASK_SHIFT,
                                         TRUE);
	mPick.mKeyMask = mask;
	if (gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK || gSavedSettings.getBOOL("LiruMouselookMenu"))
	{
		handleRightClickPick();
	}
	return FALSE;
}
BOOL LLToolPie::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	LLToolMgr::getInstance()->clearTransientTool();
	return LLTool::handleRightMouseUp(x, y, mask);
}
BOOL LLToolPie::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return LLViewerMediaFocus::getInstance()->handleScrollWheel(x, y, clicks);
}
BOOL LLToolPie::handleLeftClickPick()
{
	S32 x = mPick.mMousePt.mX;
	S32 y = mPick.mMousePt.mY;
	MASK mask = mPick.mKeyMask;
	if (mPick.mPickType == LLPickInfo::PICK_PARCEL_WALL)
	{
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getCollisionParcel();
		if (parcel)
		{
			LLViewerParcelMgr::getInstance()->selectCollisionParcel();
			if (parcel->getParcelFlag(PF_USE_PASS_LIST)
				&& !LLViewerParcelMgr::getInstance()->isCollisionBanned())
			{
				void* deselect_when_done = (void*)TRUE;
				LLPanelLandGeneral::onClickBuyPass(deselect_when_done);
			}
			else
			{
				LLFloaterLand::showInstance();
			}
		}
		gFocusMgr.setKeyboardFocus(NULL);
		return LLTool::handleMouseDown(x, y, mask);
	}
	LLViewerObject *object = mPick.getObject();
	LLViewerObject *parent = NULL;
	if (mPick.mPickType != LLPickInfo::PICK_LAND)
	{
		LLViewerParcelMgr::getInstance()->deselectLand();
	}
	if (object)
	{
		parent = object->getRootEdit();
	}
	if (handleMediaClick(mPick))
	{
		return TRUE;
	}
	if (useClickAction(mask, object, parent))
	{
		if ( (rlv_handler_t::isEnabled()) &&
			 (gRlvHandler.hasBehaviour(RLV_BHVR_FARTOUCH)) && (!gRlvHandler.canTouch(object, mPick.mObjectOffset)) )
		{
			return TRUE;
		}
		mClickAction = 0;
		if (object && object->getClickAction())
		{
			mClickAction = object->getClickAction();
		}
		else if (parent && parent->getClickAction())
		{
			mClickAction = parent->getClickAction();
		}
		switch(mClickAction)
		{
		case CLICK_ACTION_TOUCH:
			break;
		case CLICK_ACTION_SIT:
			{
				if (isAgentAvatarValid() && !gAgentAvatarp->isSitting())
				{
					bool disable_click_sit = gSavedSettings.getBOOL("DisableClickSit");
					if (!disable_click_sit)
					{
						if (gSavedSettings.getBOOL("DisableClickSitOtherOwner"))
						{
							disable_click_sit = !object->permYouOwner();
						}
					}
					if (disable_click_sit) return true;
					handle_object_sit_or_stand();
					gFocusMgr.setKeyboardFocus(NULL);
					return TRUE;
				}
			}
		case CLICK_ACTION_PAY:
			if ( mClickActionPayEnabled )
			{
				if ((object && object->flagTakesMoney())
					|| (parent && parent->flagTakesMoney()))
				{
					mClickActionObject = object;
					mLeftClickSelection = LLToolSelect::handleObjectSelection(mPick, FALSE, TRUE);
					if (LLSelectMgr::getInstance()->selectGetAllValid())
					{
						selectionPropertiesReceived();
					}
					return TRUE;
				}
			}
			break;
		case CLICK_ACTION_BUY:
			if ( mClickActionBuyEnabled )
			{
				mClickActionObject = parent;
				mLeftClickSelection = LLToolSelect::handleObjectSelection(mPick, FALSE, TRUE, TRUE);
				if (LLSelectMgr::getInstance()->selectGetAllValid())
				{
					selectionPropertiesReceived();
				}
				return TRUE;
			}
			break;
		case CLICK_ACTION_OPEN:
			if (parent && parent->allowOpen())
			{
				mClickActionObject = parent;
				mLeftClickSelection = LLToolSelect::handleObjectSelection(mPick, FALSE, TRUE, TRUE);
				if (LLSelectMgr::getInstance()->selectGetAllValid())
				{
					selectionPropertiesReceived();
				}
			}
			return TRUE;
		case CLICK_ACTION_PLAY:
			handle_click_action_play();
			return TRUE;
		case CLICK_ACTION_OPEN_MEDIA:
			handle_click_action_open_media(object);
			return TRUE;
		case CLICK_ACTION_ZOOM:
			{
				const F32 PADDING_FACTOR = 2.f;
				LLViewerObject* object = gObjectList.findObject(mPick.mObjectID);
				if (object)
				{
					gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
					LLBBox bbox = object->getBoundingBoxAgent() ;
					F32 angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getAspect() > 1.f ? LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect() : LLViewerCamera::getInstance()->getView());
					F32 distance = bbox.getExtentLocal().magVec() * PADDING_FACTOR / atan(angle_of_view);
					LLVector3 obj_to_cam = LLViewerCamera::getInstance()->getOrigin() - bbox.getCenterAgent();
					obj_to_cam.normVec();
					LLVector3d object_center_global = gAgent.getPosGlobalFromAgent(bbox.getCenterAgent());
					gAgentCamera.setCameraPosAndFocusGlobal(object_center_global + LLVector3d(obj_to_cam * distance),
													  object_center_global,
													  mPick.mObjectID );
				}
			}
			return TRUE;
		default:
			break;
		}
	}
	if (gFocusMgr.getKeyboardFocus())
	{
		mBlockClickToWalk = true;
		gFocusMgr.setKeyboardFocus(NULL);
	}
	BOOL touchable = (object && object->flagHandleTouch())
					 || (parent && parent->flagHandleTouch());
	if (object &&
		!object->isAvatar() &&
		((object->flagUsePhysics() || (parent && !parent->isAvatar() && parent->flagUsePhysics())) || touchable)
		)
	{
		if ( (rlv_handler_t::isEnabled()) && (!gRlvHandler.canTouch(object, mPick.mObjectOffset)) )
		{
			return LLTool::handleMouseDown(x, y, mask);
		}
		gGrabTransientTool = this;
		mMouseButtonDown = false;
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolGrab::getInstance() );
		return LLToolGrab::getInstance()->handleObjectHit( mPick );
	}
	LLHUDIcon* last_hit_hud_icon = mPick.mHUDIcon;
	if (!object && last_hit_hud_icon && last_hit_hud_icon->getSourceObject())
	{
		LLFloaterScriptDebug::show(last_hit_hud_icon->getSourceObject()->getID());
	}
	if (!gSavedSettings.getBOOL("LeftClickShowMenu"))
	{
		if (!mMouseButtonDown)
		{
			return true;
		}
		while( object && object->isAttachment() && !object->flagHandleTouch())
		{
			if (object->isHUDAttachment())
			{
				break;
			}
			object = (LLViewerObject*)object->getParent();
		}
		if (object && object == gAgentAvatarp )
		{
			mMouseButtonDown = false;
			LLToolMgr::getInstance()->setTransientTool(LLToolCamera::getInstance());
			gViewerWindow->hideCursor();
			LLToolCamera::getInstance()->setMouseCapture(TRUE);
			LLToolCamera::getInstance()->pickCallback(mPick);
			if(gSavedSettings.getBOOL("ResetFocusOnSelfClick"))
			{
				gAgentCamera.setFocusOnAvatar(TRUE, TRUE);
			}
			return TRUE;
		}
		LLFirstUse::useLeftClickNoHit();
		return LLTool::handleMouseDown(x, y, mask);
	}
	if (gAgent.leftButtonGrabbed())
	{
		return LLTool::handleMouseDown(x, y, mask);
	}
	LLToolSelect::handleObjectSelection(mPick, FALSE, TRUE);
	handleRightMouseDown(x, y, mask);
	return TRUE;
}
BOOL LLToolPie::useClickAction(MASK mask,
							   LLViewerObject* object,
							   LLViewerObject* parent)
{
	return	mask == MASK_NONE
			&& object
			&& !object->isAttachment()
			&& LLPrimitive::isPrimitive(object->getPCode())
			&& (object->getClickAction()
				|| parent->getClickAction());
}
U8 final_click_action(LLViewerObject* obj)
{
	if (!obj) return CLICK_ACTION_NONE;
	if (obj->isAttachment()) return CLICK_ACTION_NONE;
	U8 click_action = CLICK_ACTION_TOUCH;
	LLViewerObject* parent = obj->getRootEdit();
	if (obj->getClickAction()
	    || (parent && parent->getClickAction()))
	{
		if (obj->getClickAction())
		{
			click_action = obj->getClickAction();
		}
		else if (parent && parent->getClickAction())
		{
			click_action = parent->getClickAction();
		}
	}
	return click_action;
}
ECursorType LLToolPie::cursorFromObject(LLViewerObject* object)
{
	LLViewerObject* parent = NULL;
	if (object)
	{
		parent = object->getRootEdit();
	}
	U8 click_action = final_click_action(object);
	ECursorType cursor = UI_CURSOR_ARROW;
	switch(click_action)
	{
	case CLICK_ACTION_SIT:
		{
			if ( (isAgentAvatarValid() && !gAgentAvatarp->isSitting()) &&
				 ((!rlv_handler_t::isEnabled()) || (gRlvHandler.canSit(object, LLToolPie::getInstance()->getHoverPick().mObjectOffset))) )
			{
				cursor = UI_CURSOR_TOOLSIT;
			}
		}
		break;
	case CLICK_ACTION_BUY:
		if ( mClickActionBuyEnabled )
		{
			LLSelectNode* node = LLSelectMgr::getInstance()->getHoverNode();
			if (!node || node->mSaleInfo.isForSale())
			{
				cursor = UI_CURSOR_TOOLBUY;
			}
		}
		break;
	case CLICK_ACTION_OPEN:
		if (parent && parent->allowOpen())
		{
			cursor = UI_CURSOR_TOOLOPEN;
		}
		break;
	case CLICK_ACTION_PAY:
		if ( mClickActionPayEnabled )
		{
			if ((object && object->flagTakesMoney())
				|| (parent && parent->flagTakesMoney()))
			{
				cursor = UI_CURSOR_TOOLPAY;
			}
		}
		break;
	case CLICK_ACTION_ZOOM:
			cursor = UI_CURSOR_TOOLZOOMIN;
			break;
	case CLICK_ACTION_PLAY:
	case CLICK_ACTION_OPEN_MEDIA:
		cursor = cursor_from_parcel_media(click_action);
		break;
	default:
		break;
	}
	return cursor;
}
void LLToolPie::resetSelection()
{
	mLeftClickSelection = NULL;
	mClickActionObject = NULL;
	mClickAction = 0;
}
void LLToolPie::walkToClickedLocation()
{
	handle_go_to(mPick.mPosGlobal);
}
void LLToolPie::selectionPropertiesReceived()
{
	if (!LLSelectMgr::getInstance()->selectGetAllValid())
	{
		return;
	}
	LLObjectSelection* selection = LLToolPie::getInstance()->getLeftClickSelection();
	if (selection)
	{
		LLViewerObject* selected_object = selection->getPrimaryObject();
		if (selected_object == LLToolPie::getInstance()->getClickActionObject())
		{
			U8 click_action = LLToolPie::getInstance()->getClickAction();
			switch (click_action)
			{
			case CLICK_ACTION_BUY:
				if ( LLToolPie::getInstance()->mClickActionBuyEnabled )
				{
					handle_buy();
				}
				break;
			case CLICK_ACTION_PAY:
				if ( LLToolPie::getInstance()->mClickActionPayEnabled )
				{
					handle_give_money_dialog(selected_object);
				}
				break;
			case CLICK_ACTION_OPEN:
				handle_object_open();
				break;
			default:
				break;
			}
		}
	}
	LLToolPie::getInstance()->resetSelection();
}
BOOL LLToolPie::handleHover(S32 x, S32 y, MASK mask)
{
    BOOL pick_rigged = false;
	mHoverPick = gViewerWindow->pickImmediate(x, y, FALSE, pick_rigged);
	LLViewerObject *parent = NULL;
	LLViewerObject *object = mHoverPick.getObject();
	if ( (object) && (rlv_handler_t::isEnabled()) &&
		( ((gRlvHandler.hasBehaviour(RLV_BHVR_FARTOUCH))) && (!gRlvHandler.canTouch(object, mHoverPick.mObjectOffset)) ||
		  ((gRlvHandler.hasBehaviour(RLV_BHVR_INTERACT)) && (!object->isHUDAttachment())) ) )
	{
		gViewerWindow->setCursor(UI_CURSOR_ARROW);
		return TRUE;
	}
	if (object)
	{
		parent = object->getRootEdit();
	}
	if (handleMediaHover(mHoverPick))
	{
		LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
	}
	else if (!mMouseOutsideSlop
		&& mMouseButtonDown
		&& gSavedSettings.getBOOL("ClickToWalk"))
	{
		S32 delta_x = x - mMouseDownX;
		S32 delta_y = y - mMouseDownY;
		S32 threshold = gSavedSettings.getS32("DragAndDropDistanceThreshold");
		if (delta_x * delta_x + delta_y * delta_y > threshold * threshold)
		{
			startCameraSteering();
			steerCameraWithMouse(x, y);
			gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
		}
	}
	else if (inCameraSteerMode())
	{
		steerCameraWithMouse(x, y);
		gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
	}
	else
	{
		LLPickInfo click_action_pick = gViewerWindow->pickImmediate(x, y, TRUE, pick_rigged);
		LLViewerObject* click_action_object = click_action_pick.getObject();
		if (click_action_object && useClickAction(mask, click_action_object, click_action_object->getRootEdit()))
		{
			ECursorType cursor = cursorFromObject(click_action_object);
			gViewerWindow->setCursor(cursor);
			LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
		}
		else if ( (object) && (rlv_handler_t::isEnabled()) && (!gRlvHandler.canTouch(object)) )
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
		}
		else if ((object && !object->isAvatar() && object->flagUsePhysics())
				 || (parent && !parent->isAvatar() && parent->flagUsePhysics()))
		{
			gViewerWindow->setCursor(UI_CURSOR_TOOLGRAB);
			LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
		}
		else if ( (object && object->flagHandleTouch())
				  || (parent && parent->flagHandleTouch()))
		{
			gViewerWindow->setCursor(UI_CURSOR_HAND);
			LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
			LL_DEBUGS("UserInput") << "hover handled by LLToolPie (inactive)" << LL_ENDL;
		}
	}
	if(!object)
	{
		LLViewerMediaFocus::getInstance()->clearHover();
	}
	return TRUE;
}
BOOL LLToolPie::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLViewerObject* obj = mPick.getObject();
	U8 click_action = final_click_action(obj);
	if (handleMediaMouseUp() || LLViewerMediaFocus::getInstance()->getFocus())
	{
		mBlockClickToWalk = true;
	}
	stopCameraSteering();
	mMouseButtonDown = false;
	if (click_action == CLICK_ACTION_NONE
		&& gSavedSettings.getBOOL("ClickToWalk")
		&& !gAgent.getFlying()
		&& gAgentAvatarp
		&& !gAgentAvatarp->isSitting()
		&& !mBlockClickToWalk
        )
	{
        LLPickInfo savedPick = mPick;
        mPick = gViewerWindow->pickImmediate(savedPick.mMousePt.mX, savedPick.mMousePt.mY,
                                             FALSE ,
                                             FALSE ,
                                             FALSE );
		LLViewerObject* objp = mPick.getObject();
		bool is_in_world = mPick.mObjectID.notNull() && objp && !objp->isHUDAttachment();
		bool is_land = mPick.mPickType == LLPickInfo::PICK_LAND;
		bool pos_non_zero = !mPick.mPosGlobal.isExactlyZero();
		if (pos_non_zero && (is_land || is_in_world))
        {
			while (objp && !objp->isAvatar())
			{
				objp = (LLViewerObject*) objp->getParent();
			}
			if (objp && ((LLVOAvatar*) objp)->isSelf())
			{
				const F64 SELF_CLICK_WALK_DISTANCE = 3.0;
				mPick.mPosGlobal = gAgent.getPositionGlobal() + LLVector3d(LLViewerCamera::instance().getAtAxis()) * SELF_CLICK_WALK_DISTANCE;
			}
			gAgentCamera.setFocusOnAvatar(TRUE, TRUE);
			walkToClickedLocation();
			return TRUE;
		}
        else
        {
            LL_DEBUGS("maint5901") << "walk target was "
                                   << (mPick.mPosGlobal.isExactlyZero() ? "zero" : "not zero")
                                   << ", pick type was " << (mPick.mPickType == LLPickInfo::PICK_LAND ? "land" : "not land")
                                   << ", pick object was " << mPick.mObjectID
                                   << LL_ENDL;
            mPick = savedPick;
        }
	}
	gViewerWindow->setCursor(UI_CURSOR_ARROW);
	if (hasMouseCapture())
	{
		setMouseCapture(FALSE);
	}
	LLToolMgr::getInstance()->clearTransientTool();
	gAgentCamera.setLookAt(LOOKAT_TARGET_CONVERSATION, obj);
	mBlockClickToWalk = false;
	return LLTool::handleMouseUp(x, y, mask);
}
void LLToolPie::stopClickToWalk()
{
	handle_go_to(gAgent.getPositionGlobal());
}
BOOL LLToolPie::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks)
	{
		LL_INFOS() << "LLToolPie handleDoubleClick (becoming mouseDown)" << LL_ENDL;
	}
    if (handleMediaDblClick(mPick))
    {
        return TRUE;
    }
	bool dbl_click_autoplt = gSavedSettings.getBOOL("DoubleClickAutoPilot");
	bool dbl_click_teleport = gSavedSettings.getBOOL("DoubleClickTeleport");
	if (dbl_click_autoplt || dbl_click_teleport)
	{
		LLPickInfo savedPick = mPick;
		mPick = gViewerWindow->pickImmediate(savedPick.mMousePt.mX, savedPick.mMousePt.mY,
                                             FALSE ,
                                             FALSE ,
                                             FALSE );
		LLViewerObject* objp = mPick.getObject();
		LLViewerObject* parentp = objp ? objp->getRootEdit() : NULL;
		bool is_in_world = mPick.mObjectID.notNull() && objp && !objp->isHUDAttachment();
		bool is_land = mPick.mPickType == LLPickInfo::PICK_LAND;
		bool pos_non_zero = !mPick.mPosGlobal.isExactlyZero();
		bool has_touch_handler = (objp && objp->flagHandleTouch()) || (parentp && parentp->flagHandleTouch());
		bool no_click_action = final_click_action(objp) == CLICK_ACTION_NONE;
		if (pos_non_zero && (is_land || (is_in_world && !has_touch_handler && no_click_action)))
		{
			if (dbl_click_autoplt
				&& !gAgent.getFlying()
				&& isAgentAvatarValid()
				&& !gAgentAvatarp->isSitting()
				)
			{
				while (objp && !objp->isAvatar())
				{
					objp = (LLViewerObject*) objp->getParent();
				}
				if (objp && ((LLVOAvatar*) objp)->isSelf())
				{
					const F64 SELF_CLICK_WALK_DISTANCE = 3.0;
					mPick.mPosGlobal = gAgent.getPositionGlobal() + LLVector3d(LLViewerCamera::instance().getAtAxis()) * SELF_CLICK_WALK_DISTANCE;
				}
				gAgentCamera.setFocusOnAvatar(TRUE, TRUE);
				walkToClickedLocation();
				return TRUE;
			}
			else if (dbl_click_teleport)
			{
				LLVector3d pos = mPick.mPosGlobal;
				pos.mdV[VZ] += gAgentAvatarp->getPelvisToFoot();
				gAgent.teleportViaLocationLookAt(pos);
				return TRUE;
			}
		}
		mPick = savedPick;
	}
	return FALSE;
}
void LLToolPie::handleSelect()
{
	mBlockClickToWalk = true;
}
void LLToolPie::handleDeselect()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}
	LLSelectMgr::getInstance()->setHoverObject(NULL);
	LLSelectMgr::getInstance()->validateSelection();
}
LLTool* LLToolPie::getOverrideTool(MASK mask)
{
	static LLCachedControl<bool> enable_grab(gSavedSettings, "EnableGrab", true);
	if (enable_grab)
	{
		if (mask == MASK_CONTROL)
		{
			return LLToolGrab::getInstance();
		}
		else if (mask == (MASK_CONTROL | MASK_SHIFT))
		{
			return LLToolGrab::getInstance();
		}
	}
	return LLTool::getOverrideTool(mask);
}
void LLToolPie::stopEditing()
{
	if(	hasMouseCapture() )
	{
		setMouseCapture( FALSE );
	}
}
void LLToolPie::onMouseCaptureLost()
{
	stopCameraSteering();
	mMouseButtonDown = false;
	handleMediaMouseUp();
}
void LLToolPie::stopCameraSteering()
{
	mMouseOutsideSlop = false;
}
bool LLToolPie::inCameraSteerMode()
{
	return mMouseButtonDown && mMouseOutsideSlop && gSavedSettings.getBOOL("ClickToWalk");
}
BOOL LLToolPie::outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y)
{
	S32 dx = x - start_x;
	S32 dy = y - start_y;
	return (dx <= -2 || 2 <= dx || dy <= -2 || 2 <= dy);
}
void LLToolPie::render()
{
	return;
}
static void handle_click_action_play()
{
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!parcel) return;
	LLViewerMediaImpl::EMediaStatus status = LLViewerParcelMedia::getStatus();
	switch(status)
	{
		case LLViewerMediaImpl::MEDIA_PLAYING:
			LLViewerParcelMedia::pause();
			break;
		case LLViewerMediaImpl::MEDIA_PAUSED:
			LLViewerParcelMedia::start();
			break;
		default:
			LLViewerParcelMedia::play(parcel);
			break;
	}
}
bool LLToolPie::handleMediaClick(const LLPickInfo& pick)
{
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    LLPointer<LLViewerObject> objectp = pick.getObject();
    if (!parcel ||
        objectp.isNull() ||
        pick.mObjectFace < 0 ||
        pick.mObjectFace >= objectp->getNumTEs())
    {
        LLViewerMediaFocus::getInstance()->clearFocus();
        return false;
    }
    const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
    if (!tep)
        return false;
    LLMediaEntry* mep = (tep->hasMedia()) ? tep->getMediaData() : NULL;
    if (!mep)
        return false;
    viewer_media_t media_impl = LLViewerMedia::getMediaImplFromTextureID(mep->getMediaID());
    if (gSavedSettings.getBOOL("MediaOnAPrimUI"))
    {
        if (!LLViewerMediaFocus::getInstance()->isFocusedOnFace(pick.getObject(), pick.mObjectFace) || media_impl.isNull())
        {
            LLViewerMediaFocus::getInstance()->setFocusFace(pick.getObject(), pick.mObjectFace, media_impl, pick.mNormal);
        }
        else
        {
            gFocusMgr.setKeyboardFocus(LLViewerMediaFocus::getInstance());
            LLEditMenuHandler::gEditMenuHandler = LLViewerMediaFocus::instance().getFocusedMediaImpl();
            media_impl->mouseDown(pick.mUVCoords, gKeyboard->currentMask(TRUE));
            mMediaMouseCaptureID = mep->getMediaID();
            setMouseCapture(TRUE);
        }
        return true;
    }
    LLViewerMediaFocus::getInstance()->clearFocus();
    return false;
}
bool LLToolPie::handleMediaDblClick(const LLPickInfo& pick)
{
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    LLPointer<LLViewerObject> objectp = pick.getObject();
    if (!parcel ||
        objectp.isNull() ||
        pick.mObjectFace < 0 ||
        pick.mObjectFace >= objectp->getNumTEs())
    {
        LLViewerMediaFocus::getInstance()->clearFocus();
        return false;
    }
    const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
    if (!tep)
        return false;
    LLMediaEntry* mep = (tep->hasMedia()) ? tep->getMediaData() : NULL;
    if (!mep)
        return false;
    viewer_media_t media_impl = LLViewerMedia::getMediaImplFromTextureID(mep->getMediaID());
    if (gSavedSettings.getBOOL("MediaOnAPrimUI"))
    {
        if (!LLViewerMediaFocus::getInstance()->isFocusedOnFace(pick.getObject(), pick.mObjectFace) || media_impl.isNull())
        {
            LLViewerMediaFocus::getInstance()->setFocusFace(pick.getObject(), pick.mObjectFace, media_impl, pick.mNormal);
        }
        else
        {
            gFocusMgr.setKeyboardFocus(LLViewerMediaFocus::getInstance());
            LLEditMenuHandler::gEditMenuHandler = LLViewerMediaFocus::instance().getFocusedMediaImpl();
            media_impl->mouseDoubleClick(pick.mUVCoords, gKeyboard->currentMask(TRUE));
            mMediaMouseCaptureID = mep->getMediaID();
            setMouseCapture(TRUE);
        }
        return true;
    }
    LLViewerMediaFocus::getInstance()->clearFocus();
    return false;
}
bool LLToolPie::handleMediaHover(const LLPickInfo& pick)
{
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!parcel) return false;
	LLPointer<LLViewerObject> objectp = pick.getObject();
	if ( objectp.isNull() ||
		pick.mObjectFace < 0 ||
		pick.mObjectFace >= objectp->getNumTEs() )
	{
		LLViewerMediaFocus::getInstance()->clearHover();
		return false;
	}
	const LLTextureEntry* tep = objectp->getTE(pick.mObjectFace);
	if(!tep)
		return false;
	const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
	if (mep
		&& gSavedSettings.getBOOL("MediaOnAPrimUI"))
	{
		viewer_media_t media_impl = LLViewerMedia::getMediaImplFromTextureID(mep->getMediaID());
		if(media_impl.notNull())
		{
			if (!LLViewerMediaFocus::getInstance()->isHoveringOverFace(objectp, pick.mObjectFace))
			{
				LLViewerMediaFocus::getInstance()->setHoverFace(objectp, pick.mObjectFace, media_impl, pick.mNormal);
			}
			if (LLViewerMediaFocus::getInstance()->isFocusedOnFace(objectp, pick.mObjectFace))
			{
				media_impl->mouseMove(pick.mUVCoords, gKeyboard->currentMask(TRUE));
				gViewerWindow->setCursor(media_impl->getLastSetCursor());
			}
			else
			{
				gViewerWindow->setCursor(UI_CURSOR_ARROW);
			}
			return true;
		}
	}
	LLViewerMediaFocus::getInstance()->clearHover();
	return false;
}
bool LLToolPie::handleMediaMouseUp()
{
	bool result = false;
	if(mMediaMouseCaptureID.notNull())
	{
		viewer_media_t media_impl = LLViewerMedia::getMediaImplFromTextureID(mMediaMouseCaptureID);
		if(media_impl)
		{
			media_impl->onMouseCaptureLost();
		}
		mMediaMouseCaptureID.setNull();
		result = true;
	}
	return result;
}
static void handle_click_action_open_media(LLPointer<LLViewerObject> objectp)
{
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!parcel) return;
	if (objectp.isNull()) return;
	S32 face = LLToolPie::getInstance()->getPick().mObjectFace;
	if( face < 0 || face >= objectp->getNumTEs() ) return;
	if (LLViewerMedia::getMediaImplFromTextureID(objectp->getTE(face)->getID()) != NULL)
	{
		handle_click_action_play();
		return;
	}
	std::string media_url = std::string ( parcel->getMediaURL () );
	std::string media_type = std::string ( parcel->getMediaType() );
	LLStringUtil::trim(media_url);
	LLWeb::loadURL(media_url);
}
static ECursorType cursor_from_parcel_media(U8 click_action)
{
	ECursorType open_cursor = UI_CURSOR_ARROW;
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!parcel) return open_cursor;
	std::string media_url = std::string ( parcel->getMediaURL () );
	std::string media_type = std::string ( parcel->getMediaType() );
	LLStringUtil::trim(media_url);
	open_cursor = UI_CURSOR_TOOLMEDIAOPEN;
	LLViewerMediaImpl::EMediaStatus status = LLViewerParcelMedia::getStatus();
	switch(status)
	{
		case LLViewerMediaImpl::MEDIA_PLAYING:
			return click_action == CLICK_ACTION_PLAY ? UI_CURSOR_TOOLPAUSE : open_cursor;
		default:
			return UI_CURSOR_TOOLPLAY;
	}
}
BOOL LLToolPie::handleRightClickPick()
{
	S32 x = mPick.mMousePt.mX;
	S32 y = mPick.mMousePt.mY;
	MASK mask = mPick.mKeyMask;
	if (mPick.mPickType != LLPickInfo::PICK_LAND)
	{
		LLViewerParcelMgr::getInstance()->deselectLand();
	}
	LLViewerObject *object = mPick.getObject();
	LLToolSelect::handleObjectSelection(mPick, FALSE, TRUE);
	if (mPick.mPickType == LLPickInfo::PICK_LAND)
	{
		LLParcelSelectionHandle selection = LLViewerParcelMgr::getInstance()->selectParcelAt( mPick.mPosGlobal );
		gMenuHolder->setParcelSelection(selection);
		gPieLand->show(x, y);
		showVisualContextMenuEffect();
	}
	else if (mPick.mObjectID == gAgent.getID() )
	{
		if(!gPieSelf)
		{
			return TRUE ;
		}
		gPieSelf->show(x, y);
	}
	else if (object)
	{
		gMenuHolder->setObjectSelection(LLSelectMgr::getInstance()->getSelection());
		bool is_other_attachment = (object->isAttachment() && !object->isHUDAttachment() && !object->permYouOwner());
		if (object->isAvatar()
			|| is_other_attachment)
		{
			while( object && object->isAttachment())
			{
				object = (LLViewerObject*)object->getParent();
				llassert(object);
			}
			if (!object)
			{
				return TRUE;
			}
			LLVOAvatar* avatar = (LLVOAvatar*)object;
			std::string name = avatar->getFullname();
			std::string mute_msg;
			if (LLMuteList::getInstance()->isMuted(avatar->getID(), name))
			{
				mute_msg = LLTrans::getString("UnmuteAvatar");
			}
			else
			{
				mute_msg = LLTrans::getString("MuteAvatar");
			}
			gMenuHolder->childSetText("Avatar Mute", mute_msg);
			if ( (!rlv_handler_t::isEnabled()) || (!LLSelectMgr::getInstance()->getSelection()->isEmpty()) ||
				 (!gRlvHandler.hasBehaviour(RLV_BHVR_FARTOUCH)) )
			{
				{
					gPieAvatar->show(x, y);
				}
			}
			else
			{
				make_ui_sound("UISndInvalidOp");
			}
		}
		else if (object->isAttachment())
		{
			gPieAttachment->show(x, y);
		}
		else
		{
			std::string name;
			LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
			if (node)
			{
				name = node->mName;
			}
			std::string mute_msg;
			if (LLMuteList::getInstance()->isMuted(object->getID(), name))
			{
				mute_msg = LLTrans::getString("UnmuteObject");
			}
			else
			{
				mute_msg = LLTrans::getString("MuteObject2");
			}
			if ( (!rlv_handler_t::isEnabled()) || (!LLSelectMgr::getInstance()->getSelection()->isEmpty()) ||
				 (!gRlvHandler.hasBehaviour(RLV_BHVR_FARTOUCH)) )
			{
				gMenuHolder->childSetText("Object Mute", mute_msg);
				gPieObject->show(x, y);
				showVisualContextMenuEffect();
			}
			else
			{
				make_ui_sound("UISndInvalidOp");
			}
		}
	}
	LLTool::handleRightMouseDown(x, y, mask);
	return TRUE;
}
void LLToolPie::showVisualContextMenuEffect()
{
	if (gSavedSettings.getBOOL("DisablePointAtAndBeam"))
	{
		return;
	}
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_SPHERE, TRUE);
	effectp->setPositionGlobal(mPick.mPosGlobal);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	effectp->setDuration(0.25f);
}
typedef enum e_near_far
{
	NEAR_INTERSECTION,
	FAR_INTERSECTION
} ENearFar;
bool intersect_ray_with_sphere( const LLVector3& ray_pt, const LLVector3& ray_dir, const LLVector3& sphere_center, F32 sphere_radius, e_near_far near_far, LLVector3& intersection_pt)
{
	LLVector3 sphere_to_ray_start_vec = ray_pt - sphere_center;
	F32 B = 2.f * ray_dir * sphere_to_ray_start_vec;
	F32 C = sphere_to_ray_start_vec.lengthSquared() - (sphere_radius * sphere_radius);
	F32 discriminant = B*B - 4.f*C;
	if (discriminant >= 0.f)
	{
		F32 t0 = (-B - sqrtf(discriminant)) / 2.f;
		if (t0 > 0.f && near_far == NEAR_INTERSECTION)
		{
			intersection_pt = ray_pt + ray_dir * t0;
		}
		else
		{
			F32 t1 = (-B + sqrtf(discriminant)) / 2.f;
			intersection_pt = ray_pt + ray_dir * t1;
		}
		return true;
	}
	else
	{
		return false;
	}
}
void LLToolPie::startCameraSteering()
{
	mMouseOutsideSlop = true;
	mBlockClickToWalk = true;
	if (gAgentCamera.getFocusOnAvatar())
	{
		mSteerPick = mPick;
		LLViewerObject* avatar_object = mSteerPick.getObject();
		while (avatar_object && !avatar_object->isAvatar())
		{
			avatar_object = (LLViewerObject*)avatar_object->getParent();
		}
		if (avatar_object && ((LLVOAvatar*)avatar_object)->isSelf())
		{
			mSteerPick.mPosGlobal = gAgent.getPositionGlobal() + LLVector3d(LLViewerCamera::instance().getAtAxis()) * 3.0;
		}
		if (!mSteerPick.isValid())
		{
			mSteerPick.mPosGlobal = gAgent.getPosGlobalFromAgent(
				LLViewerCamera::instance().getOrigin() + gViewerWindow->mouseDirectionGlobal(mSteerPick.mMousePt.mX, mSteerPick.mMousePt.mY) * 100.f);
		}
		setMouseCapture(TRUE);
		mMouseSteerX = mMouseDownX;
		mMouseSteerY = mMouseDownY;
		const LLVector3 camera_to_rotation_center	= gAgent.getFrameAgent().getOrigin() - LLViewerCamera::instance().getOrigin();
		const LLVector3 rotation_center_to_pick		= gAgent.getPosAgentFromGlobal(mSteerPick.mPosGlobal) - gAgent.getFrameAgent().getOrigin();
		mClockwise = camera_to_rotation_center * rotation_center_to_pick < 0.f;
	}
}
void LLToolPie::steerCameraWithMouse(S32 x, S32 y)
{
	const LLViewerCamera& camera = LLViewerCamera::instance();
	const LLCoordFrame& rotation_frame = gAgent.getFrameAgent();
	const LLVector3 pick_pos = gAgent.getPosAgentFromGlobal(mSteerPick.mPosGlobal);
	const LLVector3 pick_rotation_center = rotation_frame.getOrigin() + parallel_component(pick_pos - rotation_frame.getOrigin(), rotation_frame.getUpAxis());
	const F32 MIN_ROTATION_RADIUS_FRACTION = 0.2f;
	const F32 min_rotation_radius = MIN_ROTATION_RADIUS_FRACTION * dist_vec(pick_rotation_center, camera.getOrigin());;
	const F32 pick_distance_from_rotation_center = llclamp(dist_vec(pick_pos, pick_rotation_center), min_rotation_radius, F32_MAX);
	const LLVector3 camera_to_rotation_center = pick_rotation_center - camera.getOrigin();
	const LLVector3 adjusted_camera_pos = LLViewerCamera::instance().getOrigin() + projected_vec(camera_to_rotation_center, rotation_frame.getUpAxis());
	const F32 camera_distance_from_rotation_center = dist_vec(adjusted_camera_pos, pick_rotation_center);
	LLVector3 mouse_ray = orthogonal_component(gViewerWindow->mouseDirectionGlobal(x, y), rotation_frame.getUpAxis());
	mouse_ray.normalize();
	LLVector3 old_mouse_ray = orthogonal_component(gViewerWindow->mouseDirectionGlobal(mMouseSteerX, mMouseSteerY), rotation_frame.getUpAxis());
	old_mouse_ray.normalize();
	F32 yaw_angle;
	F32 old_yaw_angle;
	LLVector3 mouse_on_sphere;
	LLVector3 old_mouse_on_sphere;
	if (intersect_ray_with_sphere(
			adjusted_camera_pos,
			mouse_ray,
			pick_rotation_center,
			pick_distance_from_rotation_center,
			FAR_INTERSECTION,
			mouse_on_sphere))
	{
		LLVector3 mouse_sphere_offset = mouse_on_sphere - pick_rotation_center;
		yaw_angle = atan2f(mouse_sphere_offset * rotation_frame.getLeftAxis(), mouse_sphere_offset * rotation_frame.getAtAxis());
	}
	else
	{
		yaw_angle = F_PI_BY_TWO + asinf(pick_distance_from_rotation_center / camera_distance_from_rotation_center);
		if (mouse_ray * rotation_frame.getLeftAxis() < 0.f)
		{
			yaw_angle *= -1.f;
		}
	}
	if (intersect_ray_with_sphere(
			adjusted_camera_pos,
			old_mouse_ray,
			pick_rotation_center,
			pick_distance_from_rotation_center,
			FAR_INTERSECTION,
			old_mouse_on_sphere))
	{
		LLVector3 mouse_sphere_offset = old_mouse_on_sphere - pick_rotation_center;
		old_yaw_angle = atan2f(mouse_sphere_offset * rotation_frame.getLeftAxis(), mouse_sphere_offset * rotation_frame.getAtAxis());
	}
	else
	{
		old_yaw_angle = F_PI_BY_TWO + asinf(pick_distance_from_rotation_center / camera_distance_from_rotation_center);
		if (old_mouse_ray * rotation_frame.getLeftAxis() < 0.f)
		{
			old_yaw_angle *= -1.f;
		}
	}
	const F32 delta_angle = yaw_angle - old_yaw_angle;
	if (mClockwise)
	{
		gAgent.yaw(delta_angle);
	}
	else
	{
		gAgent.yaw(-delta_angle);
	}
	mMouseSteerX = x;
	mMouseSteerY = y;
}
