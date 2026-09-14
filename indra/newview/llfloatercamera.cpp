/** 
 * @file llfloatercamera.cpp
 * @brief Combined camera and movement controls
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
#include "llfloatercamera.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "lldraghandle.h"
#include "lljoystickbutton.h"
#include "llmoveview.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "wlfPanel_AdvSettings.h"
const F32 CAMERA_BUTTON_DELAY = 0.0f;
const F32 MOVE_BUTTON_DELAY = 0.0f;
const S32 CAMERA_CONTROLS_MIN_WIDTH = 240;
const S32 CAMERA_CONTROLS_MOVE_HEIGHT = 56;
const S32 CAMERA_CONTROLS_JOY_HEIGHT = 80;
const S32 CAMERA_CONTROLS_PRESET_HEIGHT = 24;
const S32 CAMERA_CONTROLS_MIN_HEIGHT = 160;
LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:	LLFloater("camera floater"),
	mRotate(nullptr),
	mZoom(nullptr),
	mTrack(nullptr),
	mForwardButton(nullptr),
	mBackwardButton(nullptr),
	mSlideLeftButton(nullptr),
	mSlideRightButton(nullptr),
	mTurnLeftButton(nullptr),
	mTurnRightButton(nullptr),
	mMoveUpButton(nullptr),
	mMoveDownButton(nullptr),
	mRollLeftButton(nullptr),
	mRollRightButton(nullptr)
{
	const BOOL DONT_OPEN = FALSE;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_camera.xml", NULL, DONT_OPEN);
	setIsChrome(TRUE);
	setCanTearOff(FALSE);
	reshape(CAMERA_CONTROLS_MIN_WIDTH, CAMERA_CONTROLS_MIN_HEIGHT);
	setCanClose(TRUE);
	const S32 joy_bottom = CAMERA_CONTROLS_MOVE_HEIGHT;
	const S32 joy_top = joy_bottom + CAMERA_CONTROLS_JOY_HEIGHT;
	const S32 ROTATE_WIDTH = 80;
	const S32 ZOOM_WIDTH = 16;
	const S32 TRACK_WIDTH = 80;
	S32 left = (CAMERA_CONTROLS_MIN_WIDTH - ROTATE_WIDTH - ZOOM_WIDTH - TRACK_WIDTH) / 2;
	mRotate = new LLJoystickCameraRotate(std::string("cam rotate stick"),
										 LLRect( left, joy_top, left + ROTATE_WIDTH, joy_bottom ),
										 std::string("cam_rotate_out.tga"),
										 std::string("cam_rotate_in.tga") );
	mRotate->setFollows(FOLLOWS_BOTTOM | FOLLOWS_LEFT);
	mRotate->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mRotate->setToolTip( getString("rotate_tooltip") );
	mRotate->setSoundFlags(MOUSE_DOWN | MOUSE_UP);
	addChild(mRotate);
	left += ROTATE_WIDTH;
	mZoom = new LLJoystickCameraZoom(
									 std::string("zoom"),
									 LLRect( left, joy_top, left + ZOOM_WIDTH, joy_bottom ),
									 std::string("cam_zoom_out.tga"),
									 std::string("cam_zoom_plus_in.tga"),
									 std::string("cam_zoom_minus_in.tga"));
	mZoom->setFollows(FOLLOWS_BOTTOM | FOLLOWS_LEFT);
	mZoom->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mZoom->setToolTip( getString("zoom_tooltip") );
	mZoom->setSoundFlags(MOUSE_DOWN | MOUSE_UP);
	addChild(mZoom);
	left += ZOOM_WIDTH;
	mTrack = new LLJoystickCameraTrack(std::string("cam track stick"),
									   LLRect( left, joy_top, left + TRACK_WIDTH, joy_bottom ),
									   std::string("cam_tracking_out.tga"),
									   std::string("cam_tracking_in.tga"));
	mTrack->setFollows(FOLLOWS_BOTTOM | FOLLOWS_LEFT);
	mTrack->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mTrack->setToolTip( getString("move_tooltip") );
	mTrack->setSoundFlags(MOUSE_DOWN | MOUSE_UP);
	addChild(mTrack);
	mRollLeftButton = getChild<LLButton>("roll_left_btn");
	mRollLeftButton->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mRollLeftButton->setHeldDownCallback(boost::bind(&LLFloaterCamera::rollLeft, this));
	mRollRightButton = getChild<LLButton>("roll_right_btn");
	mRollRightButton->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mRollRightButton->setHeldDownCallback(boost::bind(&LLFloaterCamera::rollRight, this));
	sendChildToFront(mRollLeftButton);
	sendChildToFront(mRollRightButton);
	mForwardButton = getChild<LLJoystickAgentTurn>("forward btn");
	mForwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mBackwardButton = getChild<LLJoystickAgentTurn>("backward btn");
	mBackwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mSlideLeftButton = getChild<LLJoystickAgentSlide>("slide left btn");
	mSlideLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mSlideRightButton = getChild<LLJoystickAgentSlide>("slide right btn");
	mSlideRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnLeftButton = getChild<LLButton>("turn left btn");
	mTurnLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnLeftButton->setHeldDownCallback(boost::bind(&LLFloaterCamera::turnLeft, this));
	mTurnRightButton = getChild<LLButton>("turn right btn");
	mTurnRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnRightButton->setHeldDownCallback(boost::bind(&LLFloaterCamera::turnRight, this));
	mMoveUpButton = getChild<LLButton>("move up btn");
	mMoveUpButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveUpButton->setHeldDownCallback(boost::bind(&LLFloaterCamera::moveUp, this));
	mMoveDownButton = getChild<LLButton>("move down btn");
	mMoveDownButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveDownButton->setHeldDownCallback(boost::bind(&LLFloaterCamera::moveDown, this));
	childSetAction("rear_view_btn", boost::bind(&LLFloaterCamera::onClickCameraItem, this, std::string("rear_view")));
	childSetAction("front_view_btn", boost::bind(&LLFloaterCamera::onClickCameraItem, this, std::string("front_view")));
	childSetAction("side_view_btn", boost::bind(&LLFloaterCamera::onClickCameraItem, this, std::string("side_view")));
	childSetAction("tpp_view_btn", boost::bind(&LLFloaterCamera::onClickCameraItem, this, std::string("tpp_view")));
	childSetAction("object_view_btn", boost::bind(&LLFloaterCamera::onClickCameraItem, this, std::string("object_view")));
	childSetAction("mouselook_view_btn", boost::bind(&LLFloaterCamera::onClickCameraItem, this, std::string("mouselook_view")));
	childSetAction("reset_view_btn", boost::bind(&LLFloaterCamera::onClickCameraItem, this, std::string("reset_view")));
	childSetAction("roll_left_btn", boost::bind(&LLFloaterCamera::rollLeft, this));
	childSetAction("roll_right_btn", boost::bind(&LLFloaterCamera::rollRight, this));
	if (getDragHandle())
	{
		sendChildToBack(getDragHandle());
	}
	sendChildToFront(getChild<LLButton>("llfloater_close_btn"));
}
void LLFloaterCamera::onOpen()
{
	reshape(CAMERA_CONTROLS_MIN_WIDTH, CAMERA_CONTROLS_MIN_HEIGHT);
	setCanClose(TRUE);
	if (getDragHandle())
	{
		sendChildToBack(getDragHandle());
	}
	sendChildToFront(getChild<LLButton>("llfloater_close_btn"));
	LLFloater::onOpen();
	gSavedSettings.setBOOL("ShowCameraControls", TRUE);
	gSavedSettings.setBOOL("ShowMovementControls", FALSE);
}
void LLFloaterCamera::onClose(bool app_quitting)
{
	if (!app_quitting)
	{
		clearObjectView();
		gSavedSettings.setBOOL("ShowCameraControls", FALSE);
	}
	LLFloater::onClose(app_quitting);
}
void LLFloaterCamera::draw()
{
	updatePresetButtons();
	LLFloater::draw();
}
void LLFloaterCamera::updateMovementButtons()
{
	if (!mForwardButton)
	{
		return;
	}
	mForwardButton->setToggleState( gAgentCamera.getAtKey() > 0 || gAgentCamera.getWalkKey() > 0 );
	mBackwardButton->setToggleState( gAgentCamera.getAtKey() < 0 || gAgentCamera.getWalkKey() < 0 );
	mTurnLeftButton->setToggleState( gAgentCamera.getYawKey() > 0.f );
	mTurnRightButton->setToggleState( gAgentCamera.getYawKey() < 0.f );
	mSlideLeftButton->setToggleState( gAgentCamera.getLeftKey() > 0.f );
	mSlideRightButton->setToggleState( gAgentCamera.getLeftKey() < 0.f );
	mMoveUpButton->setToggleState( gAgentCamera.getUpKey() > 0 );
	mMoveDownButton->setToggleState( gAgentCamera.getUpKey() < 0 );
}
void LLFloaterCamera::updatePresetButtons()
{
	const U32 preset = gSavedSettings.getU32("CameraPreset");
	const bool in_mouselook = gAgentCamera.cameraMouselook();
	const bool object_view = LLToolMgr::getInstance()->usingTransientTool()
		&& LLToolMgr::getInstance()->getCurrentTool() == LLToolCamera::getInstance();
	getChild<LLButton>("rear_view_btn")->setToggleState(!in_mouselook && !object_view && preset == CAMERA_PRESET_REAR_VIEW);
	getChild<LLButton>("front_view_btn")->setToggleState(!in_mouselook && !object_view && preset == CAMERA_PRESET_FRONT_VIEW);
	getChild<LLButton>("side_view_btn")->setToggleState(!in_mouselook && !object_view && preset == CAMERA_PRESET_GROUP_VIEW);
	getChild<LLButton>("tpp_view_btn")->setToggleState(!in_mouselook && !object_view && preset == CAMERA_PRESET_TPP_VIEW);
	getChild<LLButton>("object_view_btn")->setToggleState(object_view && !in_mouselook);
	getChild<LLButton>("mouselook_view_btn")->setToggleState(in_mouselook);
}
void LLFloaterCamera::clearObjectView()
{
	LLToolMgr* tool_mgr = LLToolMgr::getInstance();
	if (tool_mgr->usingTransientTool() && tool_mgr->getCurrentTool() == LLToolCamera::getInstance())
	{
		tool_mgr->clearTransientTool();
	}
}
void LLFloaterCamera::toggleObjectView()
{
	LLToolMgr* tool_mgr = LLToolMgr::getInstance();
	if (tool_mgr->usingTransientTool() && tool_mgr->getCurrentTool() == LLToolCamera::getInstance())
	{
		tool_mgr->clearTransientTool();
	}
	else
	{
		if (gAgentCamera.cameraMouselook())
		{
			gAgentCamera.changeCameraToThirdPerson();
		}
		tool_mgr->setTransientTool(LLToolCamera::getInstance());
	}
}
void LLFloaterCamera::applyCameraPreset(S32 preset)
{
	clearObjectView();
	if (gAgentCamera.cameraMouselook())
	{
		gAgentCamera.changeCameraToThirdPerson();
	}
	gAgentCamera.switchCameraPreset(static_cast<ECameraPreset>(preset));
	if (wlfPanel_AdvSettings::instanceExists())
	{
		wlfPanel_AdvSettings& inst(wlfPanel_AdvSettings::instance());
		if (inst.isExpanded())
		{
			inst.getChildView("Rear")->setValue(preset == CAMERA_PRESET_REAR_VIEW);
			inst.getChildView("Front")->setValue(preset == CAMERA_PRESET_FRONT_VIEW);
			inst.getChildView("Group")->setValue(preset == CAMERA_PRESET_GROUP_VIEW);
		}
	}
}
void LLFloaterCamera::onClickCameraItem(const std::string& name)
{
	if (name == "mouselook_view")
	{
		clearObjectView();
		gAgentCamera.changeCameraToMouselook();
	}
	else if (name == "object_view")
	{
		toggleObjectView();
	}
	else if (name == "reset_view")
	{
		clearObjectView();
		handle_reset_view();
	}
	else if (name == "rear_view")
	{
		applyCameraPreset(CAMERA_PRESET_REAR_VIEW);
	}
	else if (name == "front_view")
	{
		applyCameraPreset(CAMERA_PRESET_FRONT_VIEW);
	}
	else if (name == "side_view")
	{
		applyCameraPreset(CAMERA_PRESET_GROUP_VIEW);
	}
	else if (name == "tpp_view")
	{
		applyCameraPreset(CAMERA_PRESET_TPP_VIEW);
	}
}
void LLFloaterCamera::turnLeft()
{
	gAgent.moveYaw( LLFloaterMove::getYawRate( mTurnLeftButton->getHeldDownTime() ) );
}
void LLFloaterCamera::turnRight()
{
	gAgent.moveYaw( -LLFloaterMove::getYawRate( mTurnRightButton->getHeldDownTime() ) );
}
void LLFloaterCamera::moveUp()
{
	gAgent.moveUp(1);
}
void LLFloaterCamera::moveDown()
{
	gAgent.moveUp(-1);
}
void LLFloaterCamera::rollLeft()
{
	gAgentCamera.setRollLeftKey(1.f);
}
void LLFloaterCamera::rollRight()
{
	gAgentCamera.setRollRightKey(1.f);
}
