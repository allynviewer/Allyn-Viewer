/** 
 * @file llviewerjoystick.cpp
 * @brief Joystick / NDOF device functionality.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "llviewerjoystick.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llappviewer.h"
#include "llkeyboard.h"
#include "lltoolmgr.h"
#include "llselectmgr.h"
#include "llviewermenu.h"
#include "llvoavatarself.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfocusmgr.h"
#include "rlvhandler.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
constexpr auto  X_I = 1;
constexpr auto  Y_I = 2;
constexpr auto  Z_I = 0;
constexpr auto RX_I = 4;
constexpr auto RY_I = 5;
constexpr auto RZ_I = 3;
constexpr F32 MIN_AFK_TIME = 2.f;
F32  LLViewerJoystick::sLastDelta[] = {0,0,0,0,0,0,0};
F32  LLViewerJoystick::sDelta[] = {0,0,0,0,0,0,0};
enum EControllerType { NONE, SPACE_NAV, XBOX, DS3, UNKNOWN };
static EControllerType sType = NONE;
bool sControlCursor = false;
enum XBoxKeys
{
	XBOX_A_KEY = 0,
	XBOX_B_KEY,
	XBOX_X_KEY,
	XBOX_Y_KEY,
	XBOX_L_BUMP_KEY,
	XBOX_R_BUMP_KEY,
	XBOX_BACK_KEY,
	XBOX_START_KEY,
	XBOX_L_STICK_CLICK,
	XBOX_R_STICK_CLICK
};
bool isOUYA(const std::string& desc) { return desc.find("OUYA") != std::string::npos; }
bool isXboxLike(const std::string& desc)
{
	return boost::algorithm::icontains(desc, "xbox") || isOUYA(desc);
}
bool isDS3Like(const std::string& desc)
{
	return desc.find("MotioninJoy") != std::string::npos;
}
enum DS3Keys
{
	DS3_TRIANGLE_KEY = 0,
	DS3_CIRCLE_KEY,
	DS3_X_KEY,
	DS3_SQUARE_KEY,
	DS3_L1_KEY,
	DS3_R1_KEY,
	DS3_L2_KEY,
	DS3_R2_KEY,
	DS3_SELECT_KEY,
	DS3_L_STICK_CLICK,
	DS3_R_STICK_CLICK,
	DS3_START_KEY,
	DS3_LOGO_KEY
};
void set_joystick_type(const S32& type)
{
	switch (type)
	{
	case 0: sType = SPACE_NAV; break;
	case 1: case 2: sType = XBOX; break;
	case 3: sType = DS3; break;
	default: sType = UNKNOWN; break;
	}
}
S32 get_joystick_type()
{
	switch (sType)
	{
	case SPACE_NAV: return 0;
	case XBOX: return isOUYA(LLViewerJoystick::getInstance()->getDescription()) ? 1 : 2;
	case DS3: return 3;
	default: return -1;
	}
}
constexpr auto MAX_SPACENAVIGATOR_INPUT = 3000.0f;
constexpr auto MAX_JOYSTICK_INPUT_VALUE = MAX_SPACENAVIGATOR_INPUT;
void LLViewerJoystick::updateEnabled(bool autoenable)
{
	if (mDriverState == JDS_UNINITIALIZED)
	{
		gSavedSettings.setBOOL("JoystickEnabled", FALSE );
	}
	else
	{
		if (isLikeSpaceNavigator() && autoenable)
		{
			gSavedSettings.setBOOL("JoystickEnabled", TRUE );
		}
	}
	if (!gSavedSettings.getBOOL("JoystickEnabled"))
	{
		mOverrideCamera = FALSE;
	}
}
void LLViewerJoystick::setOverrideCamera(bool val)
{
	if (!gSavedSettings.getBOOL("JoystickEnabled"))
	{
		mOverrideCamera = FALSE;
	}
	else
	{
		mOverrideCamera = val;
	}
	if (mOverrideCamera)
	{
		gAgentCamera.changeCameraToDefault();
	}
}
#if LIB_NDOF
NDOF_HotPlugResult LLViewerJoystick::HotPlugAddCallback(NDOF_Device *dev)
{
	NDOF_HotPlugResult res = NDOF_DISCARD_HOTPLUGGED;
	LLViewerJoystick* joystick(LLViewerJoystick::getInstance());
	if (joystick->mDriverState == JDS_UNINITIALIZED)
	{
        LL_INFOS() << "HotPlugAddCallback: will use device:" << LL_ENDL;
		ndof_dump(dev);
		joystick->mNdofDev = dev;
        joystick->mDriverState = JDS_INITIALIZED;
        res = NDOF_KEEP_HOTPLUGGED;
	}
	joystick->updateEnabled(true);
    return res;
}
#endif
#if LIB_NDOF
void LLViewerJoystick::HotPlugRemovalCallback(NDOF_Device *dev)
{
	LLViewerJoystick* joystick(LLViewerJoystick::getInstance());
	if (joystick->mNdofDev == dev)
	{
        LL_INFOS() << "HotPlugRemovalCallback: joystick->mNdofDev="
				<< joystick->mNdofDev << "; removed device:" << LL_ENDL;
		ndof_dump(dev);
		joystick->mDriverState = JDS_UNINITIALIZED;
	}
	joystick->updateEnabled(true);
}
#endif
LLViewerJoystick::LLViewerJoystick()
:	mDriverState(JDS_UNINITIALIZED),
	mNdofDev(NULL),
	mResetFlag(false),
	mCameraUpdated(true),
	mOverrideCamera(false),
	mJoystickRun(0)
{
	for (int i = 0; i < 6; i++)
	{
		mAxes[i] = sDelta[i] = sLastDelta[i] = 0.0f;
	}
	memset(mBtn, 0, sizeof(mBtn));
	mPerfScale = 4000.f / gSysCPU.getMHz();
}
LLViewerJoystick::~LLViewerJoystick()
{
	if (mDriverState == JDS_INITIALIZED)
	{
		terminate();
	}
}
void LLViewerJoystick::init(bool autoenable)
{
#if LIB_NDOF
	static bool libinit = false;
	mDriverState = JDS_INITIALIZING;
	if (libinit == false)
	{
		if (ndof_libinit(HotPlugAddCallback,
						 HotPlugRemovalCallback,
						 NULL))
		{
			mDriverState = JDS_UNINITIALIZED;
		}
		else
		{
			libinit = true;
			mNdofDev = ndof_create();
		}
	}
	if (libinit)
	{
		if (mNdofDev)
		{
			mNdofDev->axes_min = (long)-MAX_JOYSTICK_INPUT_VALUE;
			mNdofDev->axes_max = (long)+MAX_JOYSTICK_INPUT_VALUE;
			mNdofDev->absolute = 1;
			if (ndof_init_first(mNdofDev, NULL))
			{
				mDriverState = JDS_UNINITIALIZED;
				LL_WARNS() << "ndof_init_first FAILED" << LL_ENDL;
			}
			else
			{
				mDriverState = JDS_INITIALIZED;
			}
		}
		else
		{
			mDriverState = JDS_UNINITIALIZED;
		}
	}
	if (!autoenable)
	{
		autoenable = gSavedSettings.getString("JoystickInitialized").empty() ? true : false;
	}
	updateEnabled(autoenable);
	const std::string desc(getDescription());
	if (mDriverState == JDS_INITIALIZED)
	{
		sControlCursor = false;
		if (isLikeSpaceNavigator())
		{
			sType = SPACE_NAV;
			if (gSavedSettings.getString("JoystickInitialized") != "SpaceNavigator")
			{
				setSNDefaults();
				gSavedSettings.setString("JoystickInitialized", "SpaceNavigator");
			}
		}
		else if (isXboxLike(desc))
		{
			sType = XBOX;
			bool ouya(isOUYA(desc));
			std::string controller = ouya ? "OUYA" : "XboxController";
			if (gSavedSettings.getString("JoystickInitialized") != controller)
			{
				setSNDefaults(ouya ? 1 : 2);
				gSavedSettings.setString("JoystickInitialized", controller);
			}
		}
		else if (isDS3Like(desc))
		{
			sType = DS3;
			if (gSavedSettings.getString("JoystickInitialized") != "DualShock3")
			{
				setSNDefaults(3);
				gSavedSettings.setString("JoystickInitialized", "DualShock3");
			}
		}
		else
		{
			sType = UNKNOWN;
			gSavedSettings.setString("JoystickInitialized", "UnknownDevice");
		}
	}
	else
	{
		sType = NONE;
	}
	LL_INFOS() << "ndof: mDriverState=" << mDriverState << "; mNdofDev="
			<< mNdofDev << "; libinit=" << libinit << LL_ENDL;
	if (mDriverState == JDS_INITIALIZED)
	{
		LL_INFOS() << "Joystick = " << desc << LL_ENDL;
	}
#endif
}
void LLViewerJoystick::terminate()
{
#if LIB_NDOF
	ndof_libcleanup();
	LL_INFOS() << "Terminated connection with NDOF device." << LL_ENDL;
	mDriverState = JDS_UNINITIALIZED;
#endif
}
void LLViewerJoystick::updateStatus()
{
#if LIB_NDOF
	ndof_update(mNdofDev);
	for (int i=0; i<6; i++)
	{
		mAxes[i] = (F32) mNdofDev->axes[i] / mNdofDev->axes_max;
	}
	for (int i=0; i<16; i++)
	{
		mBtn[i] = mNdofDev->buttons[i];
	}
#endif
}
F32 LLViewerJoystick::getJoystickAxis(U32 axis) const
{
	if (axis < 6)
	{
		return mAxes[axis];
	}
	return 0.f;
}
U32 LLViewerJoystick::getJoystickButton(U32 button) const
{
	if (button < 16)
	{
		return mBtn[button];
	}
	return 0;
}
void LLViewerJoystick::handleRun(F32 inc)
{
	if (inc > gSavedSettings.getF32("JoystickRunThreshold"))
	{
		if (1 == mJoystickRun)
		{
			++mJoystickRun;
			gAgent.setTempRun();
		}
		else if (0 == mJoystickRun)
		{
			++mJoystickRun;
		}
	}
	else
	{
		if (mJoystickRun > 0)
		{
			--mJoystickRun;
			if (0 == mJoystickRun)
			{
				gAgent.clearTempRun();
			}
		}
	}
}
void LLViewerJoystick::agentJump()
{
    gAgent.moveUp(1);
}
void LLViewerJoystick::agentSlide(F32 inc)
{
	if (inc < 0.f)
	{
		gAgent.moveLeft(1);
	}
	else if (inc > 0.f)
	{
		gAgent.moveLeft(-1);
	}
}
void LLViewerJoystick::agentPush(F32 inc)
{
	if (inc < 0.f)
	{
		gAgent.moveAt(1, false);
	}
	else if (inc > 0.f)
	{
		gAgent.moveAt(-1, false);
	}
}
void LLViewerJoystick::agentFly(F32 inc)
{
	if (inc < 0.f)
	{
		if (! (gAgent.getFlying() ||
		       !gAgent.canFly() ||
		       gAgent.upGrabbed() ||
		       !gSavedSettings.getBOOL("AutomaticFly")) )
		{
			gAgent.setFlying(true);
		}
		gAgent.moveUp(1);
	}
	else if (inc > 0.f)
	{
		gAgent.moveUp(-1);
	}
}
void LLViewerJoystick::agentPitch(F32 pitch_inc)
{
	if (pitch_inc < 0)
	{
		gAgent.setControlFlags(AGENT_CONTROL_PITCH_POS);
	}
	else if (pitch_inc > 0)
	{
		gAgent.setControlFlags(AGENT_CONTROL_PITCH_NEG);
	}
	gAgent.pitch(-pitch_inc);
}
void LLViewerJoystick::agentYaw(F32 yaw_inc)
{
	if (gAgentCamera.cameraMouselook() && !gSavedSettings.getBOOL("JoystickMouselookYaw"))
	{
		gAgent.rotate(-yaw_inc, gAgent.getReferenceUpVector());
	}
	else
	{
		if (yaw_inc < 0)
		{
			gAgent.setControlFlags(AGENT_CONTROL_YAW_POS);
		}
		else if (yaw_inc > 0)
		{
			gAgent.setControlFlags(AGENT_CONTROL_YAW_NEG);
		}
		gAgent.yaw(-yaw_inc);
	}
}
S32 linear_ramp(const F32& inc, const F32& prev_inc)
{
	F32 nudge = inc > F_APPROXIMATELY_ZERO ? 1.f : -1.f;
	F32 linear = inc + prev_inc;
	F32 square =  0.f;
	if (abs(linear) > 0.2f)
	{
		square = linear + (0.2f * -nudge);
		square *= abs(square);
	}
	return nudge + linear * 25.f + square * 300.f;
}
void LLViewerJoystick::cursorSlide(F32 inc)
{
	static F32 prev_inc = 0.f;
	if (!is_approx_zero(inc))
	{
		S32 x, y;
		LLUI::getMousePositionScreen(&x, &y);
		x = llclamp(x + linear_ramp(inc, prev_inc), 0, gViewerWindow->getWindowWidthRaw());
		LLUI::setMousePositionScreen(x, y);
	}
	prev_inc = inc;
}
void LLViewerJoystick::cursorPush(F32 inc)
{
	static F32 prev_inc = 0.f;
	if (!is_approx_zero(inc))
	{
		S32 x, y;
		LLUI::getMousePositionScreen(&x, &y);
		y = llclamp(y + linear_ramp(inc, prev_inc), 0, gViewerWindow->getWindowHeightRaw());
		LLUI::setMousePositionScreen(x, y);
	}
	prev_inc = inc;
}
void LLViewerJoystick::cursorZoom(F32 inc)
{
	if (!is_approx_zero(inc))
	{
		static U8 count = 0;
		++count;
		if (count == 3)
		{
			gViewerWindow->handleScrollWheel(inc > F_APPROXIMATELY_ZERO ? 1 : -1);
			count = 0;
		}
	}
}
void LLViewerJoystick::resetDeltas(S32 axis[])
{
	for (U32 i = 0; i < 6; i++)
	{
		sLastDelta[i] = -mAxes[axis[i]];
		sDelta[i] = 0.f;
	}
	sLastDelta[6] = sDelta[6] = 0.f;
	mResetFlag = false;
}
void LLViewerJoystick::moveObjects(bool reset)
{
	static bool toggle_send_to_sim = false;
	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickBuildEnabled"))
	{
		return;
	}
	S32 axis[] =
	{
		gSavedSettings.getS32("JoystickAxis0"),
		gSavedSettings.getS32("JoystickAxis1"),
		gSavedSettings.getS32("JoystickAxis2"),
		gSavedSettings.getS32("JoystickAxis3"),
		gSavedSettings.getS32("JoystickAxis4"),
		gSavedSettings.getS32("JoystickAxis5"),
	};
	if (reset || mResetFlag)
	{
		resetDeltas(axis);
		return;
	}
	F32 axis_scale[] =
	{
		gSavedSettings.getF32("BuildAxisScale0"),
		gSavedSettings.getF32("BuildAxisScale1"),
		gSavedSettings.getF32("BuildAxisScale2"),
		gSavedSettings.getF32("BuildAxisScale3"),
		gSavedSettings.getF32("BuildAxisScale4"),
		gSavedSettings.getF32("BuildAxisScale5"),
	};
	F32 dead_zone[] =
	{
		gSavedSettings.getF32("BuildAxisDeadZone0"),
		gSavedSettings.getF32("BuildAxisDeadZone1"),
		gSavedSettings.getF32("BuildAxisDeadZone2"),
		gSavedSettings.getF32("BuildAxisDeadZone3"),
		gSavedSettings.getF32("BuildAxisDeadZone4"),
		gSavedSettings.getF32("BuildAxisDeadZone5"),
	};
	F32 cur_delta[6];
	F32 time = gFrameIntervalSeconds;
	if (time > .2f)
	{
		time = .2f;
	}
	F32 feather = gSavedSettings.getF32("BuildFeathering");
	bool is_zero = true, absolute = gSavedSettings.getBOOL("Cursor3D");
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[axis[i]];
		F32 tmp = cur_delta[i];
		if (absolute)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;
		is_zero = is_zero && (cur_delta[i] == 0.f);
		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-dead_zone[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i]+dead_zone[i], 0.f);
		}
		cur_delta[i] *= axis_scale[i];
		if (!absolute)
		{
			cur_delta[i] *= time;
		}
		sDelta[i] = sDelta[i] + (cur_delta[i]-sDelta[i])*time*feather;
	}
	U32 upd_type = UPD_NONE;
	LLVector3 v;
	if (!is_zero)
	{
		if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
		{
			gAgent.clearAFK();
		}
		if (sDelta[0] || sDelta[1] || sDelta[2])
		{
			upd_type |= UPD_POSITION;
			v.setVec(sDelta[0], sDelta[1], sDelta[2]);
		}
		if (sDelta[3] || sDelta[4] || sDelta[5])
		{
			upd_type |= UPD_ROTATION;
		}
		if (LLSelectMgr::getInstance()->selectionMove(v, sDelta[3],sDelta[4],sDelta[5], upd_type))
		{
			toggle_send_to_sim = true;
		}
	}
	else if (toggle_send_to_sim)
	{
		LLSelectMgr::getInstance()->sendSelectionMove();
		toggle_send_to_sim = false;
	}
}
void LLViewerJoystick::moveAvatar(bool reset)
{
	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickAvatarEnabled"))
	{
		return;
	}
	S32 axis[] =
	{
		gSavedSettings.getS32("JoystickAxis0"),
		gSavedSettings.getS32("JoystickAxis1"),
		gSavedSettings.getS32("JoystickAxis2"),
		gSavedSettings.getS32("JoystickAxis3"),
		gSavedSettings.getS32("JoystickAxis4"),
		gSavedSettings.getS32("JoystickAxis5")
	};
	if (!sControlCursor)
	{
		if (reset || mResetFlag)
		{
			resetDeltas(axis);
			if (reset)
			{
				gAgent.moveAt(0, true);
			}
			return;
		}
	}
	bool is_zero = true;
	static bool button_held = false;
	if (mBtn[sType == XBOX ? XBOX_L_STICK_CLICK : sType == DS3 ? DS3_L_STICK_CLICK : 1] == 1)
	{
		if (gSavedSettings.getBOOL("AutomaticFly"))
		{
			if (!gAgent.getFlying())
			{
				gAgent.moveUp(1);
			}
			else if (!button_held)
			{
				button_held = true;
				gAgent.setFlying(FALSE);
			}
		}
		else if (!button_held)
		{
			button_held = true;
			gAgent.setFlying(!gAgent.getFlying());
		}
		is_zero = false;
	}
	else
	{
		button_held = false;
	}
	F32 axis_scale[] =
	{
		gSavedSettings.getF32("AvatarAxisScale0"),
		gSavedSettings.getF32("AvatarAxisScale1"),
		gSavedSettings.getF32("AvatarAxisScale2"),
		gSavedSettings.getF32("AvatarAxisScale3"),
		gSavedSettings.getF32("AvatarAxisScale4"),
		gSavedSettings.getF32("AvatarAxisScale5")
	};
	F32 dead_zone[] =
	{
		gSavedSettings.getF32("AvatarAxisDeadZone0"),
		gSavedSettings.getF32("AvatarAxisDeadZone1"),
		gSavedSettings.getF32("AvatarAxisDeadZone2"),
		gSavedSettings.getF32("AvatarAxisDeadZone3"),
		gSavedSettings.getF32("AvatarAxisDeadZone4"),
		gSavedSettings.getF32("AvatarAxisDeadZone5")
	};
	F32 time = gFrameIntervalSeconds;
	if (time > .2f)
	{
		time = .2f;
	}
	F32 feather = gSavedSettings.getF32("AvatarFeathering");
	F32 cur_delta[6];
	F32 val, dom_mov = 0.f;
	U32 dom_axis = Z_I;
#if LIB_NDOF
    bool absolute = (gSavedSettings.getBOOL("Cursor3D") && mNdofDev->absolute);
#else
    bool absolute = false;
#endif
	for (U32 i = 0; i < 6; i++)
	{
		cur_delta[i] = -mAxes[axis[i]];
		if (absolute)
		{
			F32 tmp = cur_delta[i];
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
			sLastDelta[i] = tmp;
		}
		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-dead_zone[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i]+dead_zone[i], 0.f);
		}
        if (i != Z_I && i != RZ_I)
		{
			val = fabs(cur_delta[i]);
			if (val > dom_mov)
			{
				dom_axis = i;
				dom_mov = val;
			}
		}
		is_zero = is_zero && (cur_delta[i] == 0.f);
	}
	if (!is_zero)
	{
		if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
		{
			gAgent.clearAFK();
		}
		setCameraNeedsUpdate(true);
	}
	if (fabs(cur_delta[Z_I]) > .2f * dom_mov
	    || ((dom_axis == RX_I || dom_axis == RY_I)
		&& fabs(cur_delta[Z_I]) > .05f * dom_mov))
	{
		dom_axis = Z_I;
	}
	sDelta[X_I] = -cur_delta[X_I] * axis_scale[X_I];
	sDelta[Y_I] = -cur_delta[Y_I] * axis_scale[Y_I];
	sDelta[Z_I] = -cur_delta[Z_I] * axis_scale[Z_I];
	cur_delta[RX_I] *= -axis_scale[RX_I] * mPerfScale;
	cur_delta[RY_I] *= -axis_scale[RY_I] * mPerfScale;
	if (!absolute)
	{
		cur_delta[RX_I] *= time;
		cur_delta[RY_I] *= time;
	}
	sDelta[RX_I] += (cur_delta[RX_I] - sDelta[RX_I]) * time * feather;
	sDelta[RY_I] += (cur_delta[RY_I] - sDelta[RY_I]) * time * feather;
	if (sControlCursor)
	{
		cursorSlide(sDelta[X_I]);
		cursorPush(-sDelta[Z_I]);
		cursorZoom(sDelta[RX_I]);
		return;
	}
	handleRun((F32) sqrt(sDelta[Z_I]*sDelta[Z_I] + sDelta[X_I]*sDelta[X_I]));
	if (dom_axis == Z_I)
	{
		agentPush(sDelta[Z_I]);
		if (fabs(sDelta[X_I])  > .1f)
		{
			agentSlide(sDelta[X_I]);
		}
		if (fabs(sDelta[Y_I])  > .1f)
		{
			agentFly(sDelta[Y_I]);
		}
		F32 eff_rx = .3f * dead_zone[RX_I];
		F32 eff_ry = .3f * dead_zone[RY_I];
		if (sDelta[RX_I] > 0)
		{
			eff_rx = llmax(sDelta[RX_I] - eff_rx, 0.f);
		}
		else
		{
			eff_rx = llmin(sDelta[RX_I] + eff_rx, 0.f);
		}
		if (sDelta[RY_I] > 0)
		{
			eff_ry = llmax(sDelta[RY_I] - eff_ry, 0.f);
		}
		else
		{
			eff_ry = llmin(sDelta[RY_I] + eff_ry, 0.f);
		}
		if (fabs(eff_rx) > 0.f || fabs(eff_ry) > 0.f)
		{
			if (gAgent.getFlying())
			{
				agentPitch(eff_rx);
				agentYaw(eff_ry);
			}
			else
			{
				agentPitch(eff_rx);
				agentYaw(2.f * eff_ry);
			}
		}
	}
	else
	{
		agentSlide(sDelta[X_I]);
		agentFly(sDelta[Y_I]);
		agentPush(sDelta[Z_I]);
		agentPitch(sDelta[RX_I]);
		agentYaw(sDelta[RY_I]);
	}
}
void LLViewerJoystick::moveFlycam(bool reset)
{
	static LLQuaternion 		sFlycamRotation;
	static LLVector3    		sFlycamPosition;
	static F32          		sFlycamZoom;
	if (!gFocusMgr.getAppHasFocus() || mDriverState != JDS_INITIALIZED
		|| !gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickFlycamEnabled"))
	{
		return;
	}
	S32 axis[] =
	{
		gSavedSettings.getS32("JoystickAxis0"),
		gSavedSettings.getS32("JoystickAxis1"),
		gSavedSettings.getS32("JoystickAxis2"),
		gSavedSettings.getS32("JoystickAxis3"),
		gSavedSettings.getS32("JoystickAxis4"),
		gSavedSettings.getS32("JoystickAxis5"),
		gSavedSettings.getS32("JoystickAxis6")
	};
	bool in_build_mode = LLToolMgr::getInstance()->inBuildMode();
	if (reset || mResetFlag)
	{
		sFlycamPosition = LLViewerCamera::getInstance()->getOrigin();
		sFlycamRotation = LLViewerCamera::getInstance()->getQuaternion();
		sFlycamZoom = LLViewerCamera::getInstance()->getView();
		resetDeltas(axis);
		return;
	}
	F32 axis_scale[] =
	{
		gSavedSettings.getF32("FlycamAxisScale0"),
		gSavedSettings.getF32("FlycamAxisScale1"),
		gSavedSettings.getF32("FlycamAxisScale2"),
		gSavedSettings.getF32("FlycamAxisScale3"),
		gSavedSettings.getF32("FlycamAxisScale4"),
		gSavedSettings.getF32("FlycamAxisScale5"),
		gSavedSettings.getF32("FlycamAxisScale6")
	};
	F32 dead_zone[] =
	{
		gSavedSettings.getF32("FlycamAxisDeadZone0"),
		gSavedSettings.getF32("FlycamAxisDeadZone1"),
		gSavedSettings.getF32("FlycamAxisDeadZone2"),
		gSavedSettings.getF32("FlycamAxisDeadZone3"),
		gSavedSettings.getF32("FlycamAxisDeadZone4"),
		gSavedSettings.getF32("FlycamAxisDeadZone5"),
		gSavedSettings.getF32("FlycamAxisDeadZone6")
	};
	F32 time = gFrameIntervalSeconds;
	if (time > .2f)
	{
		time = .2f;
	}
	F32 cur_delta[7];
	F32 feather = gSavedSettings.getF32("FlycamFeathering");
	bool absolute = gSavedSettings.getBOOL("Cursor3D");
	bool is_zero = true;
	for (U32 i = 0; i < 7; i++)
	{
		cur_delta[i] = -getJoystickAxis(axis[i]);
		F32 tmp = cur_delta[i];
		if (absolute)
		{
			cur_delta[i] = cur_delta[i] - sLastDelta[i];
		}
		sLastDelta[i] = tmp;
		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-dead_zone[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i]+dead_zone[i], 0.f);
		}
		if (in_build_mode)
		{
			if (i == X_I || i == Y_I || i == Z_I)
			{
				static LLCachedControl<F32> build_mode_scale(gSavedSettings,"FlycamBuildModeScale", 1.0);
				cur_delta[i] *= build_mode_scale;
			}
		}
		cur_delta[i] *= axis_scale[i];
		if (!absolute)
		{
			cur_delta[i] *= time;
		}
		sDelta[i] = sDelta[i] + (cur_delta[i]-sDelta[i])*time*feather;
		is_zero = is_zero && (cur_delta[i] == 0.f);
	}
	if (!is_zero && gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}
	sFlycamPosition += LLVector3(sDelta) * sFlycamRotation;
	LLMatrix3 rot_mat(sDelta[3], sDelta[4], sDelta[5]);
	sFlycamRotation = LLQuaternion(rot_mat)*sFlycamRotation;
	if (gSavedSettings.getBOOL("AutoLeveling"))
	{
		LLMatrix3 level(sFlycamRotation);
		LLVector3 x = LLVector3(level.mMatrix[0]);
		LLVector3 y = LLVector3(level.mMatrix[1]);
		LLVector3 z = LLVector3(level.mMatrix[2]);
		y.mV[2] = 0.f;
		y.normVec();
		level.setRows(x,y,z);
		level.orthogonalize();
		LLQuaternion quat(level);
		sFlycamRotation = nlerp(llmin(feather*time,1.f), sFlycamRotation, quat);
	}
	if (gSavedSettings.getBOOL("ZoomDirect"))
	{
		sFlycamZoom = sLastDelta[6]*axis_scale[6]+dead_zone[6];
	}
	else
	{
		sFlycamZoom += sDelta[6];
	}
	LLMatrix3 mat(sFlycamRotation);
	LLViewerCamera::getInstance()->setView(sFlycamZoom);
	LLViewerCamera::getInstance()->setOrigin(sFlycamPosition);
	LLViewerCamera::getInstance()->mXAxis = LLVector3(mat.mMatrix[0]);
	LLViewerCamera::getInstance()->mYAxis = LLVector3(mat.mMatrix[1]);
	LLViewerCamera::getInstance()->mZAxis = LLVector3(mat.mMatrix[2]);
}
bool LLViewerJoystick::toggleFlycam()
{
	if (gRlvHandler.hasBehaviour(RLV_BHVR_CAMDISTMAX) || gRlvHandler.hasBehaviour(RLV_BHVR_CAMUNLOCK)
	|| !gSavedSettings.getBOOL("JoystickEnabled") || !gSavedSettings.getBOOL("JoystickFlycamEnabled"))
	{
		mOverrideCamera = false;
		return false;
	}
	if (!mOverrideCamera)
	{
		gAgentCamera.changeCameraToDefault();
	}
	if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}
	mOverrideCamera = !mOverrideCamera;
	if (mOverrideCamera)
	{
		moveFlycam(true);
	}
	else if (!LLToolMgr::getInstance()->inBuildMode())
	{
		moveAvatar(true);
	}
	else
	{
		setCameraNeedsUpdate(false);
		setNeedsReset(true);
	}
	return true;
}
bool toggleCursor() { sControlCursor = !sControlCursor; return true; }
void LLViewerJoystick::scanJoystick()
{
	if (mDriverState != JDS_INITIALIZED || !gSavedSettings.getBOOL("JoystickEnabled"))
	{
		return;
	}
#if LL_WINDOWS
	if (!mOverrideCamera)
#endif
	updateStatus();
	static long toggle_flycam = 0;
	static bool toggle_cursor = false;
	if (sType == XBOX || sType == DS3)
	{
		bool ds3 = sType == DS3;
		U8 key = ds3 ? (U8)DS3_SELECT_KEY : (U8)XBOX_BACK_KEY;
		if (mBtn[key] == 1)
		{
			if (!toggle_flycam) toggle_flycam = toggleFlycam();
		}
		else
		{
			toggle_flycam = false;
		}
		key = ds3 ? (U8)DS3_START_KEY : (U8)XBOX_START_KEY;
		if (mBtn[key] == 1)
		{
			if (!toggle_cursor) toggle_cursor = toggleCursor();
		}
		else
		{
			toggle_cursor = false;
		}
		static bool right_stick_click_down = false;
		key = ds3 ? (U8)DS3_R_STICK_CLICK : (U8)XBOX_R_STICK_CLICK;
		if (!!mBtn[key] != right_stick_click_down)
		{
			if (right_stick_click_down = mBtn[key])
				gAgentCamera.cameraMouselook() ? gAgentCamera.changeCameraToDefault() : gAgentCamera.changeCameraToMouselook();
		}
		MASK mask = gKeyboard->currentMask(TRUE);
		static bool esc_down = false;
		key = ds3 ? (U8)DS3_TRIANGLE_KEY : (U8)XBOX_Y_KEY;
		if (!!mBtn[key] != esc_down)
		{
			esc_down = mBtn[key];
			(gKeyboard->*(esc_down ? &LLKeyboard::handleTranslatedKeyDown : &LLKeyboard::handleTranslatedKeyDown))(KEY_ESCAPE, mask);
		}
		static bool alt_down = false;
		key = ds3 ? (U8)DS3_X_KEY : (U8)XBOX_A_KEY;
		if (!!mBtn[key] != alt_down)
		{
			gKeyboard->setControllerKey(KEY_ALT, alt_down = mBtn[key]);
		}
		static bool ctrl_down = false;
		key = ds3 ? (U8)DS3_SQUARE_KEY : (U8)XBOX_X_KEY;
		if (!!mBtn[key] != ctrl_down)
		{
			gKeyboard->setControllerKey(KEY_CONTROL, ctrl_down = mBtn[key]);
		}
		static bool shift_down = false;
		key = ds3 ? (U8)DS3_CIRCLE_KEY : (U8)XBOX_B_KEY;
		if (!!mBtn[key] != shift_down)
		{
			gKeyboard->setControllerKey(KEY_SHIFT, shift_down = mBtn[key]);
		}
		LLCoordGL coord;
		LLUI::getMousePositionScreen(&coord.mX, &coord.mY);
		static bool m1_down = false;
		static F32 last_m1 = 0;
		key = ds3 ? (U8)DS3_L1_KEY : (U8)XBOX_L_BUMP_KEY;
		if (!!mBtn[key] != m1_down)
		{
			m1_down = mBtn[key];
			(gViewerWindow->*(m1_down ? &LLViewerWindow::handleMouseDown : &LLViewerWindow::handleMouseUp))(gViewerWindow->getWindow(), coord, mask);
			if (m1_down && gFrameTimeSeconds-last_m1 == 0.5f)
				gViewerWindow->handleDoubleClick(gViewerWindow->getWindow(), coord, mask);
			last_m1 = gFrameTimeSeconds;
		}
		static bool m2_down = false;
		key = ds3 ? (U8)DS3_R1_KEY : (U8)XBOX_R_BUMP_KEY;
		if (!!mBtn[key] != m2_down)
		{
			m2_down = mBtn[key];
			(gViewerWindow->*(m2_down ? &LLViewerWindow::handleRightMouseDown : &LLViewerWindow::handleRightMouseUp))(gViewerWindow->getWindow(), coord, mask);
		}
		if (ds3)
		{
			static bool sit_down = false;
			if (!!mBtn[DS3_LOGO_KEY] != sit_down)
			{
				if (sit_down = mBtn[DS3_LOGO_KEY])
					(gAgentAvatarp && gAgentAvatarp->isSitting()) ? gAgent.standUp() : gAgent.sitDown();
			}
		}
	}
	else
	{
		if (mBtn[0] == 1)
		{
			if (mBtn[0] != toggle_flycam)
			{
				toggle_flycam = toggleFlycam() ? 1 : 0;
			}
		}
		else
		{
			toggle_flycam = 0;
		}
	}
	if (sControlCursor || (!mOverrideCamera && !(LLToolMgr::getInstance()->inBuildMode() && gSavedSettings.getBOOL("JoystickBuildEnabled"))))
	{
		moveAvatar();
	}
}
std::string LLViewerJoystick::getDescription()
{
	std::string res;
#if LIB_NDOF
	if (mDriverState == JDS_INITIALIZED && mNdofDev)
	{
		res = ll_safe_string(mNdofDev->product);
	}
	res = boost::regex_replace(res, boost::regex("^Controller \\((.*)\\)$", boost::regex::perl), "$1");
#endif
	return res;
}
bool LLViewerJoystick::isLikeSpaceNavigator() const
{
#if LIB_NDOF
	return (isJoystickInitialized()
			&& (strncmp(mNdofDev->product, "SpaceNavigator", 14) == 0
				|| strncmp(mNdofDev->product, "SpaceExplorer", 13) == 0
				|| strncmp(mNdofDev->product, "SpaceTraveler", 13) == 0
				|| strncmp(mNdofDev->product, "SpacePilot", 10) == 0));
#else
	return false;
#endif
}
void LLViewerJoystick::setSNDefaults(S32 type)
{
	const float platformScale = 1.f;
	const float platformScaleAvXZ = 2.f;
	const bool is_3d_cursor = true;
	set_joystick_type(type);
	const bool ouya = type == 1;
	const bool xbox = ouya || type == 2;
	const bool ds3 = type == 3;
	LL_INFOS() << "restoring " << (xbox ? ouya ? "OUYA	Game Controller" : "Xbox Controller" : ds3 ? "Dual Shock 3" : "SpaceNavigator") << " defaults..." << LL_ENDL;
	gSavedSettings.setS32("JoystickAxis0", 1);
	gSavedSettings.setS32("JoystickAxis1", ouya ? 3 : 0);
	gSavedSettings.setS32("JoystickAxis2", ouya ? 4 : ds3 ? 3 : 2);
	gSavedSettings.setS32("JoystickAxis3", xbox ? ouya ? 3 : -1 : 4);
	gSavedSettings.setS32("JoystickAxis4", xbox ? 4 : ds3 ? 5 : 3);
	gSavedSettings.setS32("JoystickAxis5", xbox ? ouya ? 0 : 3 : ds3 ? 2 : 5);
	gSavedSettings.setS32("JoystickAxis6", ouya ? 5 : -1);
	const bool game = xbox || ds3;
	gSavedSettings.setBOOL("Cursor3D", !game && is_3d_cursor);
	gSavedSettings.setBOOL("AutoLeveling", true);
	gSavedSettings.setBOOL("ZoomDirect", false);
	gSavedSettings.setF32("AvatarAxisScale0", (xbox ? 0.43f : ds3 ? 0.215f : 1.f) * platformScaleAvXZ);
	gSavedSettings.setF32("AvatarAxisScale1", (xbox  ? 0.43f : ds3 ? 0.215f : 1.f) * platformScaleAvXZ);
	gSavedSettings.setF32("AvatarAxisScale2", xbox ? 0.43f : ds3 ? -0.43f : 1.f);
	gSavedSettings.setF32("AvatarAxisScale4", ds3 ? 0.215f * platformScaleAvXZ : ((xbox ? 4.f : .1f) * platformScale));
	gSavedSettings.setF32("AvatarAxisScale5", ds3 ? 0.215f * platformScaleAvXZ : ((xbox ? 4.f : .1f) * platformScale));
	gSavedSettings.setF32("AvatarAxisScale3", (game  ? 4.f : 0.f) * platformScale);
	gSavedSettings.setF32("BuildAxisScale1", (game ? ouya ? 20.f : 0.8f : .3f) * platformScale);
	gSavedSettings.setF32("BuildAxisScale2", (xbox ? ouya ? 20.f : 0.8f : ds3 ? -0.8f : .3f) * platformScale);
	gSavedSettings.setF32("BuildAxisScale0", (game ? ouya ? 50.f : 1.6f : .3f) * platformScale);
	gSavedSettings.setF32("BuildAxisScale4", (game ? ouya ? 1.8f : 1.f : .3f) * platformScale);
	gSavedSettings.setF32("BuildAxisScale5", (game ? 2.f : .3f) * platformScale);
	gSavedSettings.setF32("BuildAxisScale3", (game ? ouya ? -6.f : 1.f : .3f) * platformScale);
	gSavedSettings.setF32("FlycamAxisScale1", (game ? ouya ? 20.f : 16.f : 2.f) * platformScale);
	gSavedSettings.setF32("FlycamAxisScale2", (game ? ouya ? 20.f : 16.f : ds3 ? -16.f : 2.f) * platformScale);
	gSavedSettings.setF32("FlycamAxisScale0", (game ? ouya ? 50.f : 25.f : 2.1f) * platformScale);
	gSavedSettings.setF32("FlycamAxisScale4", (game ? ouya ? 1.80 : -4.f : .1f) * platformScale);
	gSavedSettings.setF32("FlycamAxisScale5", (game ? 4.f : .15f) * platformScale);
	gSavedSettings.setF32("FlycamAxisScale3", (xbox ? 4.f : ds3 ? 6.f : 0.f) * platformScale);
	gSavedSettings.setF32("FlycamAxisScale6", (game ? 4.f : 0.f) * platformScale);
	gSavedSettings.setF32("AvatarAxisDeadZone0", game ? .2f : .1f);
	gSavedSettings.setF32("AvatarAxisDeadZone1", game ? .2f : .1f);
	gSavedSettings.setF32("AvatarAxisDeadZone2", game ? .2f : .1f);
	gSavedSettings.setF32("AvatarAxisDeadZone3", game ? .2f : 1.f);
	gSavedSettings.setF32("AvatarAxisDeadZone4", game ? .2f : .02f);
	gSavedSettings.setF32("AvatarAxisDeadZone5", game ? .2f : .01f);
	gSavedSettings.setF32("BuildAxisDeadZone0", game ? .02f : .01f);
	gSavedSettings.setF32("BuildAxisDeadZone1", game ? .02f : .01f);
	gSavedSettings.setF32("BuildAxisDeadZone2", game ? .02f : .01f);
	gSavedSettings.setF32("BuildAxisDeadZone3", game ? .02f : .01f);
	gSavedSettings.setF32("BuildAxisDeadZone4", game ? .02f : .01f);
	gSavedSettings.setF32("BuildAxisDeadZone5", game ? .02f : .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone0", game ? .2f : .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone1", game ? .2f : .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone2", game ? .2f : .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone3", game ? .1f : .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone4", game ? .25f : .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone5", game ? .25f : .01f);
	gSavedSettings.setF32("FlycamAxisDeadZone6", game ? .2f : 1.f);
	gSavedSettings.setF32("AvatarFeathering", game ? 3.f : 6.f);
	gSavedSettings.setF32("BuildFeathering", 12.f);
	gSavedSettings.setF32("FlycamFeathering", game ? 1.f : 5.f);
}
