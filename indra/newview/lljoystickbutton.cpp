/** 
 * @file lljoystickbutton.cpp
 * @brief LLJoystick class implementation
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
#include "lljoystickbutton.h"
#include "llcoord.h"
#include "indra_constants.h"
#include "llrender.h"
#include "llui.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llmoveview.h"
#include "llglheaders.h"
static LLRegisterWidget<LLJoystickAgentSlide> r1("joystick_slide");
static LLRegisterWidget<LLJoystickAgentTurn> r2("joystick_turn");
const F32 NUDGE_TIME = 0.25f;
const F32 ORBIT_NUDGE_RATE = 0.05f;
LLJoystick::LLJoystick(
	const std::string& name,
	LLRect rect,
	const std::string &default_image,
	const std::string &selected_image,
	EJoystickQuadrant initial_quadrant )
	:
	LLButton(name, rect, default_image, selected_image, LLStringUtil::null, NULL, NULL),
	mInitialQuadrant(initial_quadrant),
	mInitialOffset(0, 0),
	mLastMouse(0, 0),
	mFirstMouse(0, 0),
	mVertSlopNear(0),
	mVertSlopFar(0),
	mHorizSlopNear(0),
	mHorizSlopFar(0),
	mHeldDown(FALSE),
	mHeldDownTimer()
{
	setHeldDownCallback(boost::bind(&LLJoystick::onHeldDownCallback,this));
}
void LLJoystick::updateSlop()
{
	mVertSlopNear = getRect().getHeight();
	mVertSlopFar = getRect().getHeight() * 2;
	mHorizSlopNear = getRect().getWidth();
	mHorizSlopFar = getRect().getWidth() * 2;
	switch (mInitialQuadrant)
	{
	case JQ_ORIGIN:
		mInitialOffset.set(0, 0);
		break;
	case JQ_UP:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = (mVertSlopNear + mVertSlopFar) / 2;
		break;
	case JQ_DOWN:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = - (mVertSlopNear + mVertSlopFar) / 2;
		break;
	case JQ_LEFT:
		mInitialOffset.mX = - (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;
	case JQ_RIGHT:
		mInitialOffset.mX = (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;
	default:
		LL_ERRS() << "LLJoystick::LLJoystick() - bad switch case" << LL_ENDL;
		break;
	}
	return;
}
BOOL LLJoystick::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mLastMouse.set(x, y);
	mFirstMouse.set(x, y);
	mMouseDownTimer.reset();
	return LLButton::handleMouseDown(x, y, mask);
}
BOOL LLJoystick::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		mLastMouse.set(x, y);
		mHeldDown = FALSE;
		onMouseUp();
	}
	return LLButton::handleMouseUp(x, y, mask);
}
BOOL LLJoystick::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		mLastMouse.set(x, y);
	}
	return LLButton::handleHover(x, y, mask);
}
F32 LLJoystick::getElapsedHeldDownTime()
{
	if( mHeldDown )
	{
		return getHeldDownTime();
	}
	else
	{
		return 0.f;
	}
}
void LLJoystick::onHeldDownCallback(void *userdata)
{
	LLJoystick *self = (LLJoystick *)userdata;
	self->mHeldDown = TRUE;
	self->onHeldDown();
}
EJoystickQuadrant LLJoystick::selectQuadrant(LLXMLNodePtr node)
{
	EJoystickQuadrant quadrant = JQ_RIGHT;
	if (node->hasAttribute("quadrant"))
	{
		std::string quadrant_name;
		node->getAttributeString("quadrant", quadrant_name);
		quadrant = quadrantFromName(quadrant_name);
	}
	return quadrant;
}
std::string LLJoystick::nameFromQuadrant(EJoystickQuadrant	quadrant)
{
	if (quadrant == JQ_ORIGIN)	    return std::string("origin");
	else if (quadrant == JQ_UP)	    return std::string("up");
	else if (quadrant == JQ_DOWN)	return std::string("down");
	else if (quadrant == JQ_LEFT)	return std::string("left");
	else if (quadrant == JQ_RIGHT)	return std::string("right");
	else return std::string();
}
EJoystickQuadrant LLJoystick::quadrantFromName(const std::string& sQuadrant)
{
	EJoystickQuadrant quadrant = JQ_RIGHT;
	if (sQuadrant == "origin")
	{
		quadrant = JQ_ORIGIN;
	}
	else if (sQuadrant == "up")
	{
		quadrant = JQ_UP;
	}
	else if (sQuadrant == "down")
	{
		quadrant = JQ_DOWN;
	}
	else if (sQuadrant == "left")
	{
		quadrant = JQ_LEFT;
	}
	else if (sQuadrant == "right")
	{
		quadrant = JQ_RIGHT;
	}
	return quadrant;
}
LLXMLNodePtr LLJoystick::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLButton::getXML();
	node->createChild("quadrant", TRUE)->setStringValue(nameFromQuadrant(mInitialQuadrant));
	return node;
}
void LLJoystickAgentTurn::onHeldDown()
{
	F32 time = getElapsedHeldDownTime();
	updateSlop();
	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;
	float m = (float) (dx)/abs(dy);
	if (m > 1) {
		m = 1;
	}
	else if (m < -1) {
		m = -1;
	}
	gAgent.moveYaw(-LLFloaterMove::getYawRate(time)*m);
	if (dy > mVertSlopFar)
	{
		gAgent.moveAt(1);
	}
	else if (dy > mVertSlopNear)
	{
		if( time < NUDGE_TIME )
		{
			gAgent.moveAtNudge(1);
		}
		else
		{
			gAgent.moveAt(1);
		}
	}
	else if (dy < -mVertSlopFar)
	{
		gAgent.moveAt(-1);
	}
	else if (dy < -mVertSlopNear)
	{
		if( time < NUDGE_TIME )
		{
			gAgent.moveAtNudge(-1);
		}
		else
		{
			gAgent.moveAt(-1);
		}
	}
}
LLXMLNodePtr LLJoystickAgentTurn::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLJoystick::getXML();
	node->setName(LL_JOYSTICK_TURN);
	return node;
}
LLView* LLJoystickAgentTurn::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string	image_unselected;
	if (node->hasAttribute("image_unselected")) node->getAttributeString("image_unselected",image_unselected);
	std::string	image_selected;
	if (node->hasAttribute("image_selected")) node->getAttributeString("image_selected",image_selected);
	EJoystickQuadrant quad = JQ_ORIGIN;
	if (node->hasAttribute("quadrant")) quad = selectQuadrant(node);
	LLJoystickAgentTurn *button = new LLJoystickAgentTurn("button",
		LLRect(),
		image_unselected,
		image_selected,
		quad);
	if (node->hasAttribute("halign"))
	{
		LLFontGL::HAlign halign = selectFontHAlign(node);
		button->setHAlign(halign);
	}
	if (node->hasAttribute("scale_image"))
	{
		BOOL	needsScale = FALSE;
		node->getAttributeBOOL("scale_image",needsScale);
		button->setScaleImage( needsScale );
	}
	button->initFromXML(node, parent);
	return button;
}
void LLJoystickAgentSlide::onMouseUp()
{
	F32 time = getElapsedHeldDownTime();
	if( time < NUDGE_TIME )
	{
		switch (mInitialQuadrant)
		{
		case JQ_LEFT:
			gAgent.moveLeftNudge(1);
			break;
		case JQ_RIGHT:
			gAgent.moveLeftNudge(-1);
			break;
		default:
			break;
		}
	}
}
void LLJoystickAgentSlide::onHeldDown()
{
	updateSlop();
	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;
	if (dx > mHorizSlopNear)
	{
		gAgent.moveLeft(-1);
	}
	else if (dx < -mHorizSlopNear)
	{
		gAgent.moveLeft(1);
	}
	if (dy > mVertSlopFar)
	{
		gAgent.moveAt(1);
	}
	else if (dy > mVertSlopNear)
	{
		gAgent.moveAtNudge(1);
	}
	else if (dy < -mVertSlopFar)
	{
		gAgent.moveAt(-1);
	}
	else if (dy < -mVertSlopNear)
	{
		gAgent.moveAtNudge(-1);
	}
}
LLXMLNodePtr LLJoystickAgentSlide::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLJoystick::getXML();
	node->setName(LL_JOYSTICK_SLIDE);
	return node;
}
LLView* LLJoystickAgentSlide::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string	image_unselected;
	if (node->hasAttribute("image_unselected")) node->getAttributeString("image_unselected",image_unselected);
	std::string	image_selected;
	if (node->hasAttribute("image_selected")) node->getAttributeString("image_selected",image_selected);
	EJoystickQuadrant quad = JQ_ORIGIN;
	if (node->hasAttribute("quadrant")) quad = selectQuadrant(node);
	LLJoystickAgentSlide *button = new LLJoystickAgentSlide("button",
		LLRect(),
		image_unselected,
		image_selected,
		quad);
	if (node->hasAttribute("halign"))
	{
		LLFontGL::HAlign halign = selectFontHAlign(node);
		button->setHAlign(halign);
	}
	if (node->hasAttribute("scale_image"))
	{
		BOOL	needsScale = FALSE;
		node->getAttributeBOOL("scale_image",needsScale);
		button->setScaleImage( needsScale );
	}
	button->initFromXML(node, parent);
	return button;
}
LLJoystickCameraRotate::LLJoystickCameraRotate(const std::string& name, LLRect rect, const std::string &out_img, const std::string &in_img)
	:
	LLJoystick(name, rect, out_img, in_img, JQ_ORIGIN),
	mInLeft( FALSE ),
	mInTop( FALSE ),
	mInRight( FALSE ),
	mInBottom( FALSE )
{ }
void LLJoystickCameraRotate::updateSlop()
{
	mVertSlopNear = 16;
	mVertSlopFar = 32;
	mHorizSlopNear = 16;
	mHorizSlopFar = 32;
	return;
}
BOOL LLJoystickCameraRotate::handleMouseDown(S32 x, S32 y, MASK mask)
{
	updateSlop();
	S32 horiz_center = getRect().getWidth() / 2;
	S32 vert_center = getRect().getHeight() / 2;
	S32 dx = x - horiz_center;
	S32 dy = y - vert_center;
	if (dy > dx && dy > -dx)
	{
		mInitialOffset.mX = 0;
		mInitialOffset.mY = (mVertSlopNear + mVertSlopFar) / 2;
		mInitialQuadrant = JQ_UP;
	}
	else if (dy > dx && dy <= -dx)
	{
		mInitialOffset.mX = - (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		mInitialQuadrant = JQ_LEFT;
	}
	else if (dy <= dx && dy <= -dx)
	{
		mInitialOffset.mX = 0;
		mInitialOffset.mY = - (mVertSlopNear + mVertSlopFar) / 2;
		mInitialQuadrant = JQ_DOWN;
	}
	else
	{
		mInitialOffset.mX = (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		mInitialQuadrant = JQ_RIGHT;
	}
	return LLJoystick::handleMouseDown(x, y, mask);
}
void LLJoystickCameraRotate::onHeldDown()
{
	updateSlop();
	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;
	if (dx > mHorizSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitLeftKey(getOrbitRate());
	}
	else if (dx < -mHorizSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitRightKey(getOrbitRate());
	}
	if (dy > mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitUpKey(getOrbitRate());
	}
	else if (dy < -mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitDownKey(getOrbitRate());
	}
}
F32 LLJoystickCameraRotate::getOrbitRate()
{
	F32 time = getElapsedHeldDownTime();
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
		return rate;
	}
	else
	{
		return 1;
	}
}
void LLJoystickCameraRotate::setToggleState( BOOL left, BOOL top, BOOL right, BOOL bottom )
{
	mInLeft = left;
	mInTop = top;
	mInRight = right;
	mInBottom = bottom;
}
void LLJoystickCameraRotate::draw()
{
	LLGLSUIDefault gls_ui;
	getImageUnselected()->draw( 0, 0 );
	if( mInTop )
	{
		drawRotatedImage( getImageSelected()->getImage(), 0 );
	}
	if( mInRight )
	{
		drawRotatedImage( getImageSelected()->getImage(), 1 );
	}
	if( mInBottom )
	{
		drawRotatedImage( getImageSelected()->getImage(), 2 );
	}
	if( mInLeft )
	{
		drawRotatedImage( getImageSelected()->getImage(), 3 );
	}
	if (sDebugRects)
	{
		drawDebugRect();
	}
}
void LLJoystickCameraRotate::drawRotatedImage( LLTexture* image, S32 rotations )
{
	S32 width = image->getWidth();
	S32 height = image->getHeight();
	F32 uv[][2] =
	{
		{ 1.f, 1.f },
		{ 0.f, 1.f },
		{ 0.f, 0.f },
		{ 1.f, 0.f }
	};
	gGL.getTexUnit(0)->bind(image);
	gGL.color4fv(UI_VERTEX_COLOR.mV);
	gGL.begin(LLRender::TRIANGLE_STRIP);
	{
		gGL.texCoord2fv( uv[ (rotations + 0) % 4]);
		gGL.vertex2i(width, height );
		gGL.texCoord2fv( uv[ (rotations + 1) % 4]);
		gGL.vertex2i(0, height );
		gGL.texCoord2fv( uv[ (rotations + 3) % 4]);
		gGL.vertex2i(width, 0);
		gGL.texCoord2fv( uv[ (rotations + 2) % 4]);
		gGL.vertex2i(0, 0);
	}
	gGL.end();
}
void LLJoystickCameraTrack::onHeldDown()
{
	updateSlop();
	S32 dx = mLastMouse.mX - mFirstMouse.mX + mInitialOffset.mX;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;
	if (dx > mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setPanRightKey(getOrbitRate());
	}
	else if (dx < -mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setPanLeftKey(getOrbitRate());
	}
	if (dy > mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setPanUpKey(getOrbitRate());
	}
	else if (dy < -mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setPanDownKey(getOrbitRate());
	}
}
LLJoystickCameraZoom::LLJoystickCameraZoom(const std::string& name, LLRect rect, const std::string &out_img, const std::string &plus_in_img, const std::string &minus_in_img)
	:
	LLJoystick(name, rect, out_img, LLStringUtil::null, JQ_ORIGIN),
	mInTop( FALSE ),
	mInBottom( FALSE )
{
	mPlusInImage = LLUI::getUIImage(plus_in_img);
	mMinusInImage = LLUI::getUIImage(minus_in_img);
}
BOOL LLJoystickCameraZoom::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLJoystick::handleMouseDown(x, y, mask);
	if( handled )
	{
		if (mFirstMouse.mY > getRect().getHeight() / 2)
		{
			mInitialQuadrant = JQ_UP;
		}
		else
		{
			mInitialQuadrant = JQ_DOWN;
		}
	}
	return handled;
}
void LLJoystickCameraZoom::onHeldDown()
{
	updateSlop();
	const F32 FAST_RATE = 2.5f;
	S32 dy = mLastMouse.mY - mFirstMouse.mY + mInitialOffset.mY;
	if (dy > mVertSlopFar)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitInKey(FAST_RATE);
	}
	else if (dy > mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitInKey(getOrbitRate());
	}
	else if (dy < -mVertSlopFar)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitOutKey(FAST_RATE);
	}
	else if (dy < -mVertSlopNear)
	{
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitOutKey(getOrbitRate());
	}
}
void LLJoystickCameraZoom::setToggleState( BOOL top, BOOL bottom )
{
	mInTop = top;
	mInBottom = bottom;
}
void LLJoystickCameraZoom::draw()
{
	if( mInTop )
	{
		mPlusInImage->draw(0,0);
	}
	else
	if( mInBottom )
	{
		mMinusInImage->draw(0,0);
	}
	else
	{
		getImageUnselected()->draw( 0, 0 );
	}
	if (sDebugRects)
	{
		drawDebugRect();
	}
}
void LLJoystickCameraZoom::updateSlop()
{
	mVertSlopNear = getRect().getHeight() / 4;
	mVertSlopFar = getRect().getHeight() / 2;
	mHorizSlopNear = getRect().getWidth() / 4;
	mHorizSlopFar = getRect().getWidth() / 2;
	switch (mInitialQuadrant)
	{
	case JQ_ORIGIN:
		mInitialOffset.set(0, 0);
		break;
	case JQ_UP:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = (mVertSlopNear + mVertSlopFar) / 2;
		break;
	case JQ_DOWN:
		mInitialOffset.mX = 0;
		mInitialOffset.mY = - (mVertSlopNear + mVertSlopFar) / 2;
		break;
	case JQ_LEFT:
		mInitialOffset.mX = - (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;
	case JQ_RIGHT:
		mInitialOffset.mX = (mHorizSlopNear + mHorizSlopFar) / 2;
		mInitialOffset.mY = 0;
		break;
	default:
		LL_ERRS() << "LLJoystick::LLJoystick() - bad switch case" << LL_ENDL;
		break;
	}
	return;
}
F32 LLJoystickCameraZoom::getOrbitRate()
{
	F32 time = getElapsedHeldDownTime();
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
		return rate;
	}
	else
	{
		return 1;
	}
}
