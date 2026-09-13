/** 
 * @file llfloater.cpp
 * @brief LLFloater base class
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
#include "linden_common.h"
#include "llfocusmgr.h"
#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lldraghandle.h"
#include "llfocusmgr.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llkeyboard.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "llresmgr.h"
#include "llui.h"
#include "llwindow.h"
#include "llstl.h"
#include "llcontrol.h"
#include "lltabcontainer.h"
#include "v2math.h"
#include "lltrans.h"
#include "llmultifloater.h"
#include "llfasttimer.h"
#include "airecursive.h"
#include "llnotifications.h"
#include "lllocalcliprect.h"
#include "llgl.h"
#include "llrender.h"
#include <vector>
const S32 MINIMIZED_WIDTH = 160;
const S32 CLOSE_BOX_FROM_TOP = 5;
const S32 CLOSE_BOX_FROM_RIGHT = 18;
const S32 CLOSE_BOX_GAP = 4;
namespace
{
void draw_filled_rounded_rect(S32 left, S32 bottom, S32 right, S32 top, S32 radius)
{
	radius = llclamp(radius, 0, llmin((right - left) / 2, (top - bottom) / 2));
	if (radius <= 0)
	{
		gl_rect_2d(left, top, right, bottom, TRUE);
		return;
	}
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.color4f(1.f, 1.f, 1.f, 1.f);
	gl_rect_2d(left, top - radius, right, bottom + radius, TRUE);
	gl_rect_2d(left + radius, top, right - radius, top - radius, TRUE);
	gl_rect_2d(left + radius, bottom + radius, right - radius, bottom, TRUE);
	const S32 steps = 16;
	gl_circle_2d((F32)(left + radius), (F32)(top - radius), (F32)radius, steps, TRUE);
	gl_circle_2d((F32)(right - radius), (F32)(top - radius), (F32)radius, steps, TRUE);
	gl_circle_2d((F32)(left + radius), (F32)(bottom + radius), (F32)radius, steps, TRUE);
	gl_circle_2d((F32)(right - radius), (F32)(bottom + radius), (F32)radius, steps, TRUE);
}
class LLFloaterShapeClip
{
public:
	LLFloaterShapeClip(S32 width, S32 height, S32 radius)
	:	mActive(width > 0 && height > 0 && radius > 0)
	,	mPrevEnabled(GL_FALSE)
	,	mPrevFunc(GL_ALWAYS)
	,	mPrevRef(0)
	,	mPrevMask(0xFF)
	,	mPrevFail(GL_KEEP)
	,	mPrevZFail(GL_KEEP)
	,	mPrevZPass(GL_KEEP)
	{
		if (!mActive)
		{
			return;
		}
		gGL.flush();
		glGetBooleanv(GL_STENCIL_TEST, &mPrevEnabled);
		glGetIntegerv(GL_STENCIL_FUNC, &mPrevFunc);
		glGetIntegerv(GL_STENCIL_REF, &mPrevRef);
		glGetIntegerv(GL_STENCIL_VALUE_MASK, &mPrevMask);
		glGetIntegerv(GL_STENCIL_FAIL, &mPrevFail);
		glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &mPrevZFail);
		glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &mPrevZPass);
		{
			LLLocalClipRect scissor(LLRect(0, height, width, 0));
			glEnable(GL_STENCIL_TEST);
			glStencilMask(0xFF);
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			gGL.setColorMask(false, false);
			draw_filled_rounded_rect(1, 1, width - 1, height - 1, llmax(1, radius - 1));
			gGL.flush();
			gGL.setColorMask(true, true);
		}
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	~LLFloaterShapeClip()
	{
		if (!mActive)
		{
			return;
		}
		gGL.flush();
		glStencilFunc(mPrevFunc, mPrevRef, (GLuint)mPrevMask);
		glStencilOp(mPrevFail, mPrevZFail, mPrevZPass);
		if (!mPrevEnabled)
		{
			glDisable(GL_STENCIL_TEST);
		}
		else
		{
			glEnable(GL_STENCIL_TEST);
		}
	}
private:
	bool mActive;
	GLboolean mPrevEnabled;
	GLint mPrevFunc;
	GLint mPrevRef;
	GLint mPrevMask;
	GLint mPrevFail;
	GLint mPrevZFail;
	GLint mPrevZPass;
};
}
const S32 TABBED_FLOATER_OFFSET = 0;
std::string	LLFloater::sButtonActiveImageNames[BUTTON_COUNT] =
{
	"UIImgBtnCloseActiveUUID",
	"UIImgBtnRestoreActiveUUID",
	"UIImgBtnMinimizeActiveUUID",
	"UIImgBtnTearOffActiveUUID",
	"UIImgBtnCloseActiveUUID",
};
std::string	LLFloater::sButtonInactiveImageNames[BUTTON_COUNT] =
{
	"UIImgBtnCloseInactiveUUID",
	"UIImgBtnRestoreInactiveUUID",
	"UIImgBtnMinimizeInactiveUUID",
	"UIImgBtnTearOffInactiveUUID",
	"UIImgBtnCloseInactiveUUID",
};
std::string	LLFloater::sButtonPressedImageNames[BUTTON_COUNT] =
{
	"UIImgBtnClosePressedUUID",
	"UIImgBtnRestorePressedUUID",
	"UIImgBtnMinimizePressedUUID",
	"UIImgBtnTearOffPressedUUID",
	"UIImgBtnClosePressedUUID",
};
std::string	LLFloater::sButtonNames[BUTTON_COUNT] =
{
	"llfloater_close_btn",
	"llfloater_restore_btn",
	"llfloater_minimize_btn",
	"llfloater_tear_off_btn",
	"llfloater_edit_btn",
};
std::string	LLFloater::sButtonToolTips[BUTTON_COUNT] =
{
	"BUTTON_CLOSE_WIN",
	"BUTTON_RESTORE",
	"BUTTON_MINIMIZE",
	"BUTTON_TEAR_OFF",
	"BUTTON_EDIT",
};
LLFloater::button_callback LLFloater::sButtonCallbacks[BUTTON_COUNT] =
{
	&LLFloater::onClickClose,
	&LLFloater::onClickMinimize,
	&LLFloater::onClickMinimize,
	&LLFloater::onClickTearOff,
	&LLFloater::onClickEdit,
};
LLMultiFloater* LLFloater::sHostp = NULL;
BOOL			LLFloater::sEditModeEnabled;
LLFloater::handle_map_t	LLFloater::sFloaterMap;
LLFloaterView* gFloaterView = NULL;
void LLFloater::initClass()
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		sButtonToolTips[i] = LLTrans::getString( sButtonToolTips[i] );
	}
	LLControlVariable* ctrl = LLUI::sConfigGroup->getControl("ActiveFloaterTransparency");
	if (ctrl)
	{
		ctrl->getSignal()->connect(boost::bind(&LLFloater::updateActiveFloaterTransparency));
		updateActiveFloaterTransparency();
	}
	ctrl = LLUI::sConfigGroup->getControl("InactiveFloaterTransparency");
	if (ctrl)
	{
		ctrl->getSignal()->connect(boost::bind(&LLFloater::updateInactiveFloaterTransparency));
		updateInactiveFloaterTransparency();
	}
}
LLFloater::LLFloater() :
	LLPanel(), mAutoFocus(TRUE),
	mResizable(FALSE),
	mDragOnLeft(FALSE),
	mMinWidth(0),
	mMinHeight(0)
{
	mAutoFocus = TRUE;
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++)
	{
		mResizeBar[i] = NULL;
		mResizeHandle[i] = NULL;
	}
	mDragHandle = NULL;
	mNotificationContext = new LLFloaterNotificationContext(getHandle());
}
LLFloater::LLFloater(const std::string& name)
:	LLPanel(name), mAutoFocus(TRUE)
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++)
	{
		mResizeBar[i] = NULL;
		mResizeHandle[i] = NULL;
	}
	std::string title;
	initFloater(title, FALSE, DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT, FALSE, TRUE, TRUE);
}
LLFloater::LLFloater(const std::string& name, const LLRect& rect, const std::string& title,
	BOOL resizable,
	S32 min_width,
	S32 min_height,
	BOOL drag_on_left,
	BOOL minimizable,
	BOOL close_btn,
	BOOL bordered)
:	LLPanel(name, rect, bordered), mAutoFocus(TRUE)
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++)
	{
		mResizeBar[i] = NULL;
		mResizeHandle[i] = NULL;
	}
	initFloater( title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);
}
LLFloater::LLFloater(const std::string& name, const std::string& rect_control, const std::string& title,
	BOOL resizable,
	S32 min_width,
	S32 min_height,
	BOOL drag_on_left,
	BOOL minimizable,
	BOOL close_btn,
	BOOL bordered)
:	LLPanel(name, rect_control, bordered), mAutoFocus(TRUE)
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++)
	{
		mResizeBar[i] = NULL;
		mResizeHandle[i] = NULL;
	}
	initFloater( title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);
}
void LLFloater::initFloater(const std::string& title,
					 BOOL resizable, S32 min_width, S32 min_height,
					 BOOL drag_on_left, BOOL minimizable, BOOL close_btn)
{
	mNotificationContext = new LLFloaterNotificationContext(getHandle());
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		if (mButtons[i] != NULL)
		{
			removeChild(mButtons[i]);
			delete mButtons[i];
			mButtons[i] = NULL;
		}
	}
	mButtonScale = 1.f;
	BOOL need_border = hasBorder();
	removeBorder();
	deleteAllChildren();
	if (need_border)
	{
	    addBorder();
	}
	setFocusRoot(!getIsChrome());
	mDragHandle = NULL;
	for (S32 i = 0; i < 4; i++)
	{
		mResizeBar[i] = NULL;
		mResizeHandle[i] = NULL;
	}
	mCanTearOff = TRUE;
	mEditing = FALSE;
	setMouseOpaque(TRUE);
	mFirstLook = TRUE;
	mForeground = FALSE;
	mDragOnLeft = drag_on_left == TRUE;
	setBackgroundVisible(TRUE);
	mMinimized = FALSE;
	mExpandedRect.set(0,0,0,0);
	S32 close_box_size;
	if (close_btn)
	{
		close_box_size = LLFLOATER_CLOSE_BOX_SIZE;
	}
	else
	{
		close_box_size = 0;
	}
	if (drag_on_left)
	{
		LLRect drag_handle_rect;
		drag_handle_rect.setOriginAndSize(
			0, 0,
			DRAG_HANDLE_WIDTH,
			getRect().getHeight() - LLPANEL_BORDER_WIDTH - close_box_size);
		mDragHandle = new LLDragHandleLeft(std::string("drag"), drag_handle_rect, title );
	}
	else
	{
		LLRect drag_handle_rect( 0, getRect().getHeight(), getRect().getWidth(), 0 );
		mDragHandle = new LLDragHandleTop( std::string("Drag Handle"), drag_handle_rect, title );
	}
	addChild(mDragHandle);
	mResizable = resizable;
	mMinWidth = min_width;
	mMinHeight = min_height;
	if( mResizable )
	{
		addResizeCtrls();
	}
	if (close_btn)
	{
		mButtonsEnabled[BUTTON_CLOSE] = TRUE;
	}
	if ( !drag_on_left && minimizable )
	{
		mButtonsEnabled[BUTTON_MINIMIZE] = TRUE;
	}
	mHasBeenDraggedWhileMinimized = FALSE;
	mPreviousMinimizedLeft = 0;
	mPreviousMinimizedBottom = 0;
	buildButtons();
	setVisible(FALSE);
	sFloaterMap[getHandle()] = this;
	if (!getParent())
	{
		gFloaterView->addChild(this);
	}
}
void LLFloater::updateActiveFloaterTransparency()
{
	sActiveControlTransparency = LLUI::sConfigGroup->getF32("ActiveFloaterTransparency");
}
void LLFloater::updateInactiveFloaterTransparency()
{
	sInactiveControlTransparency = LLUI::sConfigGroup->getF32("InactiveFloaterTransparency");
}
void LLFloater::addResizeCtrls()
{
	LLResizeBar::Params p;
	p.name("resizebar_left");
	p.resizing_view(this);
	p.min_size(mMinWidth);
	p.side(LLResizeBar::LEFT);
	mResizeBar[LLResizeBar::LEFT] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::LEFT] );
	p.name("resizebar_top");
	p.min_size(mMinHeight);
	p.side(LLResizeBar::TOP);
	mResizeBar[LLResizeBar::TOP] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::TOP] );
	p.name("resizebar_right");
	p.min_size(mMinWidth);
	p.side(LLResizeBar::RIGHT);
	mResizeBar[LLResizeBar::RIGHT] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::RIGHT] );
	p.name("resizebar_bottom");
	p.min_size(mMinHeight);
	p.side(LLResizeBar::BOTTOM);
	mResizeBar[LLResizeBar::BOTTOM] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::BOTTOM] );
	LLResizeHandle::Params handle_p;
	handle_p.mouse_opaque(false);
	handle_p.min_width(mMinWidth);
	handle_p.min_height(mMinHeight);
	handle_p.corner(LLResizeHandle::RIGHT_BOTTOM);
	mResizeHandle[0] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[0]);
	handle_p.corner(LLResizeHandle::RIGHT_TOP);
	mResizeHandle[1] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[1]);
	handle_p.corner(LLResizeHandle::LEFT_BOTTOM);
	mResizeHandle[2] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[2]);
	handle_p.corner(LLResizeHandle::LEFT_TOP);
	mResizeHandle[3] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[3]);
	layoutResizeCtrls();
}
void LLFloater::layoutResizeCtrls()
{
	LLRect rect;
	const S32 RESIZE_BAR_THICKNESS = 3;
	rect = LLRect( 0, getRect().getHeight(), RESIZE_BAR_THICKNESS, 0);
	mResizeBar[LLResizeBar::LEFT]->setRect(rect);
	rect = LLRect( 0, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_BAR_THICKNESS);
	mResizeBar[LLResizeBar::TOP]->setRect(rect);
	rect = LLRect(getRect().getWidth() - RESIZE_BAR_THICKNESS, getRect().getHeight(), getRect().getWidth(), 0);
	mResizeBar[LLResizeBar::RIGHT]->setRect(rect);
	rect = LLRect(0, RESIZE_BAR_THICKNESS, getRect().getWidth(), 0);
	mResizeBar[LLResizeBar::BOTTOM]->setRect(rect);
	rect = LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT, getRect().getWidth(), 0);
	mResizeHandle[0]->setRect(rect);
	rect = LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_HANDLE_HEIGHT);
	mResizeHandle[1]->setRect(rect);
	rect = LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 );
	mResizeHandle[2]->setRect(rect);
	rect = LLRect( 0, getRect().getHeight(), RESIZE_HANDLE_WIDTH, getRect().getHeight() - RESIZE_HANDLE_HEIGHT );
	mResizeHandle[3]->setRect(rect);
}
void LLFloater::enableResizeCtrls(bool enable, bool width, bool height)
{
	mResizeBar[LLResizeBar::LEFT]->setVisible(enable && width);
	mResizeBar[LLResizeBar::LEFT]->setEnabled(enable && width);
	mResizeBar[LLResizeBar::TOP]->setVisible(enable && height);
	mResizeBar[LLResizeBar::TOP]->setEnabled(enable && height);
	mResizeBar[LLResizeBar::RIGHT]->setVisible(enable && width);
	mResizeBar[LLResizeBar::RIGHT]->setEnabled(enable && width);
	mResizeBar[LLResizeBar::BOTTOM]->setVisible(enable && height);
	mResizeBar[LLResizeBar::BOTTOM]->setEnabled(enable && height);
	for (S32 i = 0; i < 4; ++i)
	{
		mResizeHandle[i]->setVisible(enable && width && height);
		mResizeHandle[i]->setEnabled(enable && width && height);
	}
}
LLFloater::~LLFloater()
{
	delete mNotificationContext;
	mNotificationContext = NULL;
	control_map_t::iterator itor;
	for (itor = mFloaterControls.begin(); itor != mFloaterControls.end(); ++itor)
	{
		delete itor->second;
	}
	mFloaterControls.clear();
	releaseFocus();
	setMinimized( FALSE );
	sFloaterMap.erase(getHandle());
	delete mDragHandle;
	for (S32 i = 0; i < 4; i++)
	{
		delete mResizeBar[i];
		delete mResizeHandle[i];
	}
}
void LLFloater::setVisible( BOOL visible )
{
	LLPanel::setVisible(visible);
	if( visible && mFirstLook )
	{
		mFirstLook = FALSE;
	}
	if( !visible )
	{
		if( gFocusMgr.childIsTopCtrl( this ) )
		{
			gFocusMgr.setTopCtrl(NULL);
		}
		if( gFocusMgr.childHasMouseCapture( this ) )
		{
			gFocusMgr.setMouseCapture(NULL);
		}
	}
	for(handle_set_iter_t dependent_it = mDependents.begin();
		dependent_it != mDependents.end(); )
	{
		LLFloater* floaterp = dependent_it->get();
		if (floaterp)
		{
			floaterp->setVisible(visible);
		}
		++dependent_it;
	}
}
void LLFloater::open()
{
	if (getSoundFlags() != SILENT
		&& !getHost()
		&& !getFloaterHost()
		&& (!getVisible() || isMinimized()))
	{
		make_ui_sound("UISndWindowOpen");
	}
	if (getFloaterHost() != NULL && getHost() == NULL)
	{
		getFloaterHost()->addFloater(this, getFloaterHost()->getVisible());
	}
	else if (getHost() != NULL)
	{
		getHost()->showFloater(this);
	}
	else
	{
		setMinimized(FALSE);
		setVisibleAndFrontmost(mAutoFocus);
	}
	if (!getControlName().empty())
		setControlValue(true);
	onOpen();
}
void LLFloater::close(bool app_quitting)
{
	setMinimized(FALSE);
	if (canClose())
	{
		if (getHost())
		{
			((LLMultiFloater*)getHost())->removeFloater(this);
			gFloaterView->addChild(this);
		}
		if (getSoundFlags() != SILENT
			&& getVisible()
			&& !getHost()
			&& !app_quitting)
		{
			make_ui_sound("UISndWindowClose");
		}
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); )
		{
			LLFloater* floaterp = dependent_it->get();
			if (floaterp)
			{
				++dependent_it;
				floaterp->close();
			}
			else
			{
				mDependents.erase(dependent_it++);
			}
		}
		cleanupHandles();
		gFocusMgr.clearLastFocusForGroup(this);
		if (hasFocus())
		{
			releaseFocus();
			if (isDependent())
			{
				LLFloater* dependee = mDependeeHandle.get();
				if (dependee && !dependee->isDead())
				{
					dependee->setFocus(TRUE);
				}
			}
		}
		LLFloater* dependee = mDependeeHandle.get();
		if (dependee)
		{
			dependee->removeDependentFloater(this);
		}
		for (handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); )
		{
			LLFloater* floaterp = dependent_it->get();
			if (floaterp)
			{
				++dependent_it;
				floaterp->close(app_quitting);
			}
			else
			{
				mDependents.erase(dependent_it++);
			}
		}
		cleanupHandles();
		if (!app_quitting && !getControlName().empty())
			setControlValue(false);
		onClose(app_quitting);
	}
}
void LLFloater::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
}
void LLFloater::releaseFocus()
{
	if( gFocusMgr.childIsTopCtrl( this ) )
	{
		gFocusMgr.setTopCtrl(NULL);
	}
	if( gFocusMgr.childHasKeyboardFocus( this ) )
	{
		gFocusMgr.setKeyboardFocus(NULL);
	}
	if( gFocusMgr.childHasMouseCapture( this ) )
	{
		gFocusMgr.setMouseCapture(NULL);
	}
}
void LLFloater::setResizeLimits( S32 min_width, S32 min_height )
{
	mMinWidth = min_width;
	mMinHeight = min_height;
	for( S32 i = 0; i < 4; i++ )
	{
		if( mResizeBar[i] )
		{
			if (i == LLResizeBar::LEFT || i == LLResizeBar::RIGHT)
			{
				mResizeBar[i]->setResizeLimits( min_width, S32_MAX );
			}
			else
			{
				mResizeBar[i]->setResizeLimits( min_height, S32_MAX );
			}
		}
		if( mResizeHandle[i] )
		{
			mResizeHandle[i]->setResizeLimits( min_width, min_height );
		}
	}
}
void LLFloater::center()
{
	if(getHost())
	{
		return;
	}
	centerWithin(gFloaterView->getRect());
}
void LLFloater::applyRectControl()
{
	if (!getRectControl().empty())
	{
		const LLRect& rect = LLUI::sConfigGroup->getRect(getRectControl());
		translate( rect.mLeft - getRect().mLeft, rect.mBottom - getRect().mBottom);
		if (mResizable)
		{
			reshape(llmax(mMinWidth, rect.getWidth()), llmax(mMinHeight, rect.getHeight()));
		}
	}
	applyRoundedContentInsets();
}
bool LLFloater::isFloaterChromeChild(LLView* child) const
{
	if (!child)
	{
		return true;
	}
	if (child == mDragHandle)
	{
		return true;
	}
	for (S32 i = 0; i < BUTTON_COUNT; ++i)
	{
		if (child == mButtons[i])
		{
			return true;
		}
	}
	for (S32 i = 0; i < 4; ++i)
	{
		if (child == mResizeBar[i] || child == mResizeHandle[i])
		{
			return true;
		}
	}
	return false;
}
void LLFloater::applyRoundedContentInsets()
{
	if (isMinimized() || getIsChrome() || getHost())
	{
		return;
	}
	const S32 w = getRect().getWidth();
	const S32 h = getRect().getHeight();
	if (w < 32 || h < 32)
	{
		return;
	}
	const S32 pad = LLFLOATER_CONTENT_PAD;
	const bool has_title_bar = !getCurrentTitle().empty() && !mDragOnLeft;
	const S32 top_pad = has_title_bar ? 4 : pad;
	const S32 top_limit = has_title_bar
		? h - LLFLOATER_HEADER_SIZE - top_pad
		: h - pad;
	const S32 left_limit = mDragOnLeft ? DRAG_HANDLE_WIDTH + pad : pad;
	const S32 right_limit = w - pad;
	const S32 bottom_limit = pad;
	if (top_limit <= bottom_limit + 8 || right_limit <= left_limit + 8)
	{
		return;
	}
	const child_list_t* children = getChildList();
	if (!children)
	{
		return;
	}
	std::vector<LLView*> content;
	content.reserve(children->size());
	for (LLView* child : *children)
	{
		if (isFloaterChromeChild(child))
		{
			continue;
		}
		const LLRect& r = child->getRect();
		if (has_title_bar && r.mBottom >= h - LLFLOATER_HEADER_SIZE)
		{
			continue;
		}
		content.push_back(child);
	}
	if (content.empty())
	{
		return;
	}
	S32 content_top = content.front()->getRect().mTop;
	S32 content_left = content.front()->getRect().mLeft;
	S32 reserved_bottom = bottom_limit;
	for (LLView* child : content)
	{
		const LLRect& r = child->getRect();
		content_top = llmax(content_top, r.mTop);
		content_left = llmin(content_left, r.mLeft);
		if (child->followsBottom() && !child->followsTop())
		{
			reserved_bottom = llmax(reserved_bottom, r.mTop + 3);
		}
	}
	const S32 dx = (content_left < left_limit) ? (left_limit - content_left) : 0;
	const S32 dy = (content_top > top_limit) ? (top_limit - content_top) : 0;
	if (dx != 0 || dy != 0)
	{
		for (LLView* child : content)
		{
			if (child->followsBottom() && !child->followsTop())
			{
				if (dx != 0)
				{
					child->translate(dx, 0);
				}
				continue;
			}
			child->translate(dx, dy);
		}
	}
	for (LLView* child : content)
	{
		if (child->followsBottom() && !child->followsTop())
		{
			LLRect r = child->getRect();
			if (r.mBottom < 0)
			{
				child->translate(0, -r.mBottom);
			}
			continue;
		}
		LLRect r = child->getRect();
		const LLRect old = r;
		const bool stretch_h = child->followsLeft() && child->followsRight();
		const bool stretch_v = child->followsTop() && child->followsBottom();
		if (stretch_h)
		{
			if (r.mLeft < left_limit) r.mLeft = left_limit;
			if (r.mRight > right_limit) r.mRight = right_limit;
		}
		if (stretch_v)
		{
			if (r.mTop > top_limit) r.mTop = top_limit;
			if (r.mBottom < reserved_bottom) r.mBottom = reserved_bottom;
		}
		if (r == old || r.getWidth() < 8 || r.getHeight() < 8)
		{
			continue;
		}
		child->translate(r.mLeft - old.mLeft, r.mBottom - old.mBottom);
		if (r.getWidth() != old.getWidth() || r.getHeight() != old.getHeight())
		{
			child->reshape(r.getWidth(), r.getHeight(), TRUE);
		}
	}
}
void LLFloater::applyTitle()
{
	if (!mDragHandle)
	{
		return;
	}
	if (isMinimized() && !mShortTitle.empty())
	{
		mDragHandle->setTitle( mShortTitle );
	}
	else
	{
		mDragHandle->setTitle ( mTitle );
	}
}
const std::string& LLFloater::getCurrentTitle() const
{
	return mDragHandle ? mDragHandle->getTitle() : LLStringUtil::null;
}
void LLFloater::setTitle( const std::string& title )
{
	if(mTitle == title)
		return;
	mTitle = title;
	applyTitle();
}
std::string LLFloater::getTitle() const
{
	if (mTitle.empty())
	{
		return mDragHandle ? mDragHandle->getTitle() : LLStringUtil::null;
	}
	else
	{
		return mTitle;
	}
}
void LLFloater::setShortTitle( const std::string& short_title )
{
	mShortTitle = short_title;
	applyTitle();
}
std::string LLFloater::getShortTitle() const
{
	if (mShortTitle.empty())
	{
		return mDragHandle ? mDragHandle->getTitle() : LLStringUtil::null;
	}
	else
	{
		return mShortTitle;
	}
}
BOOL LLFloater::canSnapTo(const LLView* other_view)
{
	if (NULL == other_view)
	{
		LL_WARNS() << "other_view is NULL" << LL_ENDL;
		return FALSE;
	}
	if (other_view != getParent())
	{
		const LLFloater* other_floaterp = dynamic_cast<const LLFloater*>(other_view);
		if (other_floaterp && other_floaterp->getSnapTarget() == getHandle() && mDependents.find(other_floaterp->getHandle()) != mDependents.end())
		{
			return FALSE;
		}
	}
	return LLPanel::canSnapTo(other_view);
}
void LLFloater::setSnappedTo(const LLView* snap_view)
{
	if (!snap_view || snap_view == getParent())
	{
		clearSnapTarget();
	}
	else
	{
		const LLFloater* floaterp = dynamic_cast<const LLFloater*>(snap_view);
		if (floaterp)
		{
			setSnapTarget(floaterp->getHandle());
		}
	}
}
void LLFloater::handleReshape(const LLRect& new_rect, bool by_user)
{
	const LLRect old_rect = getRect();
	LLView::handleReshape(new_rect, by_user);
	if (!isMinimized())
	{
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); ++dependent_it)
		{
			LLFloater* floaterp = dependent_it->get();
			if (floaterp && floaterp->getSnapTarget() == getHandle())
			{
				S32 delta_x = 0;
				S32 delta_y = 0;
				LLRect dependent_rect = floaterp->getRect();
				if (dependent_rect.mLeft - getRect().mLeft >= old_rect.getWidth() ||
					dependent_rect.mRight == getRect().mLeft + old_rect.getWidth())
				{
					delta_x += new_rect.getWidth() - old_rect.getWidth();
				}
				if (dependent_rect.mBottom - getRect().mBottom >= old_rect.getHeight() ||
					dependent_rect.mTop == getRect().mBottom + old_rect.getHeight())
				{
					delta_y += new_rect.getHeight() - old_rect.getHeight();
				}
				delta_x += new_rect.mLeft - old_rect.mLeft;
				delta_y += new_rect.mBottom - old_rect.mBottom;
				dependent_rect.translate(delta_x, delta_y);
				floaterp->setShape(dependent_rect, by_user);
			}
		}
	}
	else
	{
		if ((new_rect.mLeft != old_rect.mLeft) ||
			(new_rect.mBottom != old_rect.mBottom))
		{
			mHasBeenDraggedWhileMinimized = TRUE;
		}
	}
}
void LLFloater::setMinimized(BOOL minimize)
{
	if (minimize == mMinimized) return;
	if (minimize)
	{
		mExpandedRect = getRect();
		if (mHasBeenDraggedWhileMinimized)
		{
			setOrigin(mPreviousMinimizedLeft, mPreviousMinimizedBottom);
		}
		else
		{
			S32 left, bottom;
			gFloaterView->getMinimizePosition(&left, &bottom);
			setOrigin( left, bottom );
		}
		if (mButtonsEnabled[BUTTON_MINIMIZE])
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = FALSE;
			mButtonsEnabled[BUTTON_RESTORE] = TRUE;
		}
		if (mDragHandle)
		{
			mDragHandle->setVisible(TRUE);
		}
		setBorderVisible(TRUE);
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end();
			++dependent_it)
		{
			LLFloater* floaterp = dependent_it->get();
			if (floaterp)
			{
				if (floaterp->isMinimizeable())
				{
					floaterp->setMinimized(TRUE);
				}
				else if (!floaterp->isMinimized())
				{
					floaterp->setVisible(FALSE);
				}
			}
		}
		releaseFocus();
		gFocusMgr.removeKeyboardFocusWithoutCallback(this);
		for (S32 i = 0; i < 4; i++)
		{
			if (mResizeBar[i] != NULL)
			{
				mResizeBar[i]->setEnabled(FALSE);
			}
			if (mResizeHandle[i] != NULL)
			{
				mResizeHandle[i]->setEnabled(FALSE);
			}
		}
		mMinimized = TRUE;
		reshape( MINIMIZED_WIDTH, LLFLOATER_HEADER_SIZE, TRUE);
	}
	else
	{
		if (mHasBeenDraggedWhileMinimized)
		{
			const LLRect& currentRect = getRect();
			mPreviousMinimizedLeft = currentRect.mLeft;
			mPreviousMinimizedBottom = currentRect.mBottom;
		}
		setOrigin( mExpandedRect.mLeft, mExpandedRect.mBottom );
		if (mButtonsEnabled[BUTTON_RESTORE])
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = TRUE;
			mButtonsEnabled[BUTTON_RESTORE] = FALSE;
		}
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end();
			++dependent_it)
		{
			LLFloater* floaterp = dependent_it->get();
			if (floaterp)
			{
				floaterp->setMinimized(FALSE);
				floaterp->setVisible(TRUE);
			}
		}
		for (S32 i = 0; i < 4; i++)
		{
			if (mResizeBar[i] != NULL)
			{
				mResizeBar[i]->setEnabled(isResizable());
			}
			if (mResizeHandle[i] != NULL)
			{
				mResizeHandle[i]->setEnabled(isResizable());
			}
		}
		mMinimized = FALSE;
		reshape( mExpandedRect.getWidth(), mExpandedRect.getHeight(), TRUE );
	}
	applyTitle ();
	make_ui_sound("UISndWindowClose");
	updateButtons();
}
void LLFloater::setFocus( BOOL b )
{
	if (b && getIsChrome())
	{
		return;
	}
	LLUICtrl* last_focus = gFocusMgr.getLastFocusForGroup(this);
	BOOL child_had_focus = gFocusMgr.childHasKeyboardFocus(this);
	LLPanel::setFocus(b);
	if (b)
	{
		LLFloaterView * parent = dynamic_cast<LLFloaterView *>(getParent());
		if (!getHost() && parent && !parent->getCycleMode())
		{
			if (!isFrontmost())
			{
				setFrontmost();
			}
		}
		if (last_focus && !child_had_focus &&
			last_focus->isInEnabledChain() &&
			last_focus->isInVisibleChain())
		{
			last_focus->setFocus(TRUE);
		}
	}
	updateTransparency(b ? TT_ACTIVE : TT_INACTIVE);
}
void LLFloater::setIsChrome(BOOL is_chrome)
{
	if (is_chrome)
	{
		setFocus(FALSE);
		setFocusRoot(FALSE);
	}
	if (mDragHandle)
		mDragHandle->setTitleVisible(!is_chrome);
	LLPanel::setIsChrome(is_chrome);
}
void LLFloater::setTitleVisible(bool visible)
{
	if (mDragHandle)
		mDragHandle->setTitleVisible(visible);
}
void LLFloater::setForeground(BOOL front)
{
	if (front != mForeground)
	{
		mForeground = front;
		if (mDragHandle)
			mDragHandle->setForeground( front );
		if (!front)
		{
			releaseFocus();
		}
		setBackgroundOpaque( front );
		updateTransparency(front || getIsChrome() ? TT_ACTIVE : TT_INACTIVE);
	}
}
void LLFloater::cleanupHandles()
{
	for(handle_set_iter_t dependent_it = mDependents.begin();
		dependent_it != mDependents.end(); )
	{
		LLFloater* floaterp = dependent_it->get();
		if (!floaterp)
		{
			mDependents.erase(dependent_it++);
		}
		else
		{
			++dependent_it;
		}
	}
}
void LLFloater::setHost(LLMultiFloater* host)
{
	if (mHostHandle.isDead() && host)
	{
		mButtonScale = 0.9f;
		if (mCanTearOff)
		{
			mButtonsEnabled[BUTTON_TEAR_OFF] = TRUE;
		}
	}
	else if (!mHostHandle.isDead() && !host)
	{
		mButtonScale = 1.f;
	}
	updateButtons();
	if (host)
	{
		mHostHandle = host->getHandle();
		mLastHostHandle = host->getHandle();
	}
	else
	{
		mHostHandle.markDead();
	}
}
void LLFloater::moveResizeHandlesToFront()
{
	for( S32 i = 0; i < 4; i++ )
	{
		if( mResizeBar[i] )
		{
			sendChildToFront(mResizeBar[i]);
		}
	}
	for( S32 i = 0; i < 4; i++ )
	{
		if( mResizeHandle[i] )
		{
			sendChildToFront(mResizeHandle[i]);
		}
	}
}
BOOL LLFloater::isFrontmost()
{
	return gFloaterView && gFloaterView->getFrontmost() == this && getVisible();
}
void LLFloater::addDependentFloater(LLFloater* floaterp, BOOL reposition)
{
	mDependents.insert(floaterp->getHandle());
	floaterp->mDependeeHandle = getHandle();
	if (reposition)
	{
		floaterp->setRect(gFloaterView->findNeighboringPosition(this, floaterp));
		floaterp->setSnapTarget(getHandle());
	}
	gFloaterView->adjustToFitScreen(floaterp, FALSE);
	if (floaterp->isFrontmost())
	{
		gFloaterView->bringToFront(floaterp);
	}
}
void LLFloater::addDependentFloater(LLHandle<LLFloater> dependent, BOOL reposition)
{
	LLFloater* dependent_floaterp = dependent.get();
	if(dependent_floaterp)
	{
		addDependentFloater(dependent_floaterp, reposition);
	}
}
void LLFloater::removeDependentFloater(LLFloater* floaterp)
{
	mDependents.erase(floaterp->getHandle());
	floaterp->mDependeeHandle = LLHandle<LLFloater>();
}
BOOL LLFloater::offerClickToButton(S32 x, S32 y, MASK mask, EFloaterButton index)
{
	if( mButtonsEnabled[index] )
	{
		LLButton* my_butt = mButtons[index];
		S32 local_x = x - my_butt->getRect().mLeft;
		S32 local_y = y - my_butt->getRect().mBottom;
		if (
			my_butt->pointInView(local_x, local_y) &&
			my_butt->handleMouseDown(local_x, local_y, mask))
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL LLFloater::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if( mMinimized )
	{
		if(offerClickToButton(x, y, mask, BUTTON_CLOSE)) return TRUE;
		if(offerClickToButton(x, y, mask, BUTTON_RESTORE)) return TRUE;
		if(offerClickToButton(x, y, mask, BUTTON_TEAR_OFF)) return TRUE;
		return mDragHandle->handleMouseDown(x, y, mask);
	}
	else
	{
		bringToFront( x, y );
		return LLPanel::handleMouseDown( x, y, mask );
	}
}
BOOL LLFloater::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL was_minimized = mMinimized;
	bringToFront( x, y );
	return was_minimized || LLPanel::handleRightMouseDown( x, y, mask );
}
BOOL LLFloater::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	bringToFront( x, y );
	return LLPanel::handleMiddleMouseDown( x, y, mask );
}
BOOL LLFloater::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL was_minimized = mMinimized;
	setMinimized(FALSE);
	return was_minimized || LLPanel::handleDoubleClick(x, y, mask);
}
void LLFloater::bringToFront( S32 x, S32 y )
{
	if (getVisible() && pointInView(x, y))
	{
		LLMultiFloater* hostp = getHost();
		if (hostp)
		{
			hostp->showFloater(this);
		}
		else
		{
			LLFloaterView* parent = (LLFloaterView*) getParent();
			if (parent)
			{
				parent->bringToFront( this );
			}
		}
	}
}
void LLFloater::setVisibleAndFrontmost(BOOL take_focus)
{
	setVisible(TRUE);
	setFrontmost(take_focus);
}
void LLFloater::setFrontmost(BOOL take_focus)
{
	LLMultiFloater* hostp = getHost();
	if (hostp)
	{
		hostp->showFloater(this);
	}
	else
	{
		LLFloaterView * parent = dynamic_cast<LLFloaterView*>( getParent() );
		if (parent)
		{
			parent->bringToFront(this, take_focus);
		}
		updateTransparency(hasFocus() || getIsChrome() ? TT_ACTIVE : TT_INACTIVE);
	}
}
void LLFloater::setEditModeEnabled(BOOL enable)
{
	if (enable != sEditModeEnabled)
	{
		S32 count = 0;
		for(handle_map_iter_t iter = sFloaterMap.begin(); iter != sFloaterMap.end(); ++iter)
		{
			LLFloater* floater = iter->second;
			if (!floater->isDead())
			{
				iter->second->mButtonsEnabled[BUTTON_EDIT] = enable;
				iter->second->updateButtons();
			}
			count++;
		}
	}
	sEditModeEnabled = enable;
}
void LLFloater::onClickMinimize()
{
	setMinimized( !isMinimized() );
}
void LLFloater::onClickTearOff()
{
	LLMultiFloater* host_floater = getHost();
	if (host_floater)
	{
		LLRect new_rect;
		host_floater->removeFloater(this);
		gFloaterView->addChild(this);
		open();
		if (getRectControl().empty())
		{
			new_rect.setLeftTopAndSize(host_floater->getRect().mLeft + 5, host_floater->getRect().mTop - LLFLOATER_HEADER_SIZE - 5, getRect().getWidth(), getRect().getHeight());
			setRect(new_rect);
		}
		gFloaterView->adjustToFitScreen(this, FALSE);
		setFocus(TRUE);
	}
	else
	{
		LLMultiFloater* new_host = (LLMultiFloater*)mLastHostHandle.get();
		if (new_host)
		{
			setMinimized(FALSE);
			new_host->showFloater(this);
			new_host->open();
		}
	}
}
void LLFloater::onClickEdit()
{
	mEditing = mEditing ? FALSE : TRUE;
}
LLFloater* LLFloater::getClosableFloaterFromFocus()
{
	LLFloater* focused_floater = gFloaterView->getFocusedFloater();
	if (!focused_floater)
	{
		return NULL;
	}
	LLFloater* previous_floater = NULL;
	for(LLFloater* floater_to_close = focused_floater;
		NULL != floater_to_close;
		floater_to_close = gFloaterView->getParentFloater(floater_to_close))
	{
		if(floater_to_close == previous_floater)
		{
			break;
		}
		if(floater_to_close->isCloseable())
		{
			return floater_to_close;
		}
		previous_floater = floater_to_close;
	}
	return NULL;
}
void LLFloater::closeFocusedFloater()
{
	LLFloater* floater_to_close = LLFloater::getClosableFloaterFromFocus();
	if(floater_to_close)
	{
		floater_to_close->close();
	}
	if (gFocusMgr.getKeyboardFocus() == NULL)
	{
		gFloaterView->focusFrontFloater();
	}
}
LLNotificationPtr LLFloater::addContextualNotification(const std::string& name, const LLSD& substitutions)
{
	return LLNotifications::instance().add(LLNotification::Params(name).context(mNotificationContext).substitutions(substitutions));
}
void LLFloater::onClickClose()
{
	close();
}
void LLFloater::draw()
{
	const F32 alpha = getCurrentTransparency();
	bool drew_rounded_chrome = false;
	if( isBackgroundVisible() )
	{
		drawShadow(this);
		S32 left = LLPANEL_BORDER_WIDTH;
		S32 top = getRect().getHeight() - LLPANEL_BORDER_WIDTH;
		S32 right = getRect().getWidth() - LLPANEL_BORDER_WIDTH;
		S32 bottom = LLPANEL_BORDER_WIDTH;
		LLColor4 color;
		if (isBackgroundOpaque())
		{
			color = getBackgroundColor();
		}
		else
		{
			color = getTransparentColor();
		}
		{
			const S32 w = getRect().getWidth();
			const S32 h = getRect().getHeight();
			const bool focused = gFocusMgr.childHasKeyboardFocus(this);
			LLColor4 outlineColor = focused
				? LLUI::sColorsGroup->getColor("FloaterFocusBorderColor")
				: LLUI::sColorsGroup->getColor("FloaterUnfocusBorderColor");
			LLUIImagePtr chrome = LLUI::getUIImage("Rounded_Square");
			if (chrome.notNull())
			{
				chrome->drawSolid(0, 0, w, h, outlineColor % alpha);
				chrome->drawSolid(1, 1, w - 2, h - 2, color % alpha);
				drew_rounded_chrome = true;
			}
			else
			{
				gl_rect_2d(left, top, right, bottom, color % alpha);
			}
			if(!getIsChrome() && !getCurrentTitle().empty() && chrome.notNull())
			{
				static auto titlebar_focus_color = LLUI::sColorsGroup->getColor("TitleBarFocusColor");
				LLColor4 title_bg(14.f/255.f, 16.f/255.f, 38.f/255.f, color.mV[VALPHA]);
				{
					LLLocalClipRect header_clip(LLRect(0, h, w, h - LLFLOATER_HEADER_SIZE));
					chrome->drawSolid(1, 1, w - 2, h - 2, title_bg % alpha);
					if (focused)
					{
						chrome->drawSolid(1, 1, w - 2, h - 2, titlebar_focus_color % alpha);
					}
				}
				LLColor4 divider = outlineColor;
				divider.mV[VALPHA] *= 0.55f;
				gl_line_2d(1, h - LLFLOATER_HEADER_SIZE, w - 1, h - LLFLOATER_HEADER_SIZE, divider % alpha);
			}
			else if(!getIsChrome() && !getCurrentTitle().empty())
			{
				static auto titlebar_focus_color = LLUI::sColorsGroup->getColor("TitleBarFocusColor");
				LLColor4 title_bg(14.f/255.f, 16.f/255.f, 38.f/255.f, color.mV[VALPHA]);
				gl_rect_2d(left, top, right, h - LLFLOATER_HEADER_SIZE, title_bg % alpha);
				if (focused)
				{
					gl_rect_2d(left, top, right, h - LLFLOATER_HEADER_SIZE, titlebar_focus_color % alpha);
				}
			}
		}
	}
	LLPanel::updateDefaultBtn();
	if( getDefaultButton() )
	{
		if (hasFocus() && getDefaultButton()->getEnabled())
		{
			LLFocusableElement* focus_ctrl = gFocusMgr.getKeyboardFocus();
			BOOL focus_is_child_button = dynamic_cast<LLButton*>(focus_ctrl) != NULL && dynamic_cast<LLButton*>(focus_ctrl)->getParent() == this;
			getDefaultButton()->setBorderEnabled(!focus_is_child_button);
		}
		else
		{
			getDefaultButton()->setBorderEnabled(FALSE);
		}
	}
	{
		LLFloaterShapeClip shape_clip(
			drew_rounded_chrome ? getRect().getWidth() : 0,
			drew_rounded_chrome ? getRect().getHeight() : 0,
			drew_rounded_chrome ? LLFLOATER_CORNER_RADIUS : 0);
		if (isMinimized())
		{
			for (S32 i = 0; i < BUTTON_COUNT; i++)
			{
				drawChild(mButtons[i]);
			}
			drawChild(mDragHandle);
		}
		else
		{
			LLView::draw();
		}
	}
	if( isBackgroundVisible() && !drew_rounded_chrome )
	{
		LLUI::setLineWidth(1.5f);
		LLColor4 outlineColor = gFocusMgr.childHasKeyboardFocus(this) ? LLUI::sColorsGroup->getColor("FloaterFocusBorderColor") : LLUI::sColorsGroup->getColor("FloaterUnfocusBorderColor");
		gl_rect_2d_offset_local(0, getRect().getHeight() + 1, getRect().getWidth() + 1, 0, outlineColor, -LLPANEL_BORDER_WIDTH, FALSE);
		LLUI::setLineWidth(1.f);
	}
	if (mCanTearOff && !getHost())
	{
		LLFloater* old_host = mLastHostHandle.get();
		if (!old_host)
		{
			setCanTearOff(FALSE);
		}
	}
}
void	LLFloater::drawShadow(LLPanel* panel)
{
	S32 left = LLPANEL_BORDER_WIDTH;
	S32 top = panel->getRect().getHeight() - LLPANEL_BORDER_WIDTH;
	S32 right = panel->getRect().getWidth() - LLPANEL_BORDER_WIDTH;
	S32 bottom = LLPANEL_BORDER_WIDTH;
	static LLUICachedControl<S32> shadow_offset_S32 ("DropShadowFloater", 0);
	static LLColor4 shadow_color = LLUI::sColorsGroup->getColor("ColorDropShadow");
	F32 shadow_offset = (F32)shadow_offset_S32;
	if (!panel->isBackgroundOpaque())
	{
		shadow_offset *= 0.2f;
		shadow_color.mV[VALPHA] *= 0.5f;
	}
	gl_drop_shadow(left, top, right, bottom,
		shadow_color % getCurrentTransparency(),
		ll_round(shadow_offset));
}
void LLFloater::updateTransparency(LLView* view, ETypeTransparency transparency_type)
{
	if (view)
	{
		if (view->isCtrl())
		{
			static_cast<LLUICtrl*>(view)->setTransparencyType(transparency_type);
		}
		for (LLView* pChild : *view->getChildList())
		{
			if ((pChild->getChildCount()) || (pChild->isCtrl()))
				updateTransparency(pChild, transparency_type);
		}
	}
}
void LLFloater::updateTransparency(ETypeTransparency transparency_type)
{
	updateTransparency(this, transparency_type);
}
void	LLFloater::setCanMinimize(BOOL can_minimize)
{
	if (!can_minimize)
	{
		setMinimized(FALSE);
	}
	mButtonsEnabled[BUTTON_MINIMIZE] = can_minimize && !isMinimized();
	mButtonsEnabled[BUTTON_RESTORE]  = can_minimize &&  isMinimized();
	updateButtons();
}
void	LLFloater::setCanClose(BOOL can_close)
{
	mButtonsEnabled[BUTTON_CLOSE] = can_close;
	updateButtons();
}
void	LLFloater::setCanTearOff(BOOL can_tear_off)
{
	mCanTearOff = can_tear_off;
	mButtonsEnabled[BUTTON_TEAR_OFF] = mCanTearOff && !mHostHandle.isDead();
	updateButtons();
}
void	LLFloater::setCanResize(BOOL can_resize)
{
	if (mResizable && !can_resize)
	{
		for (S32 i = 0; i < 4; i++)
		{
			removeChild(mResizeBar[i]);
			delete mResizeBar[i];
			mResizeBar[i] = NULL;
			removeChild(mResizeHandle[i]);
			delete mResizeHandle[i];
			mResizeHandle[i] = NULL;
		}
	}
	else if (!mResizable && can_resize)
	{
		addResizeCtrls();
		enableResizeCtrls(can_resize);
	}
	mResizable = can_resize;
}
void LLFloater::setCanDrag(BOOL can_drag)
{
	if (!can_drag && mDragHandle->getEnabled())
	{
		mDragHandle->setEnabled(FALSE);
	}
	else if (can_drag && !mDragHandle->getEnabled())
	{
		mDragHandle->setEnabled(TRUE);
	}
}
void LLFloater::updateButtons()
{
	S32 button_count = 0;
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		if(!mButtons[i]) continue;
		mButtons[i]->setEnabled(mButtonsEnabled[i]);
		if (mButtonsEnabled[i]
			|| (i == BUTTON_CLOSE && mButtonScale != 1.f))
		{
			button_count++;
			LLRect btn_rect;
			if (mDragOnLeft)
			{
				btn_rect.setLeftTopAndSize(
					LLPANEL_BORDER_WIDTH,
					getRect().getHeight() - CLOSE_BOX_FROM_TOP - (LLFLOATER_CLOSE_BOX_SIZE + 1) * button_count,
					ll_round((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
					ll_round((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
			}
			else
			{
				btn_rect.setLeftTopAndSize(
					getRect().getWidth() - CLOSE_BOX_FROM_RIGHT - (LLFLOATER_CLOSE_BOX_SIZE + CLOSE_BOX_GAP) * button_count,
					getRect().getHeight() - CLOSE_BOX_FROM_TOP,
					ll_round((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
					ll_round((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
			}
			mButtons[i]->setRect(btn_rect);
			mButtons[i]->setVisible(TRUE);
			mButtons[i]->setTabStop(i == BUTTON_RESTORE);
		}
		else if (mButtons[i])
		{
			mButtons[i]->setVisible(FALSE);
		}
	}
	if (mDragHandle)
	{
		mDragHandle->setMaxTitleWidth(getRect().getWidth() - CLOSE_BOX_FROM_RIGHT
			- (button_count * (LLFLOATER_CLOSE_BOX_SIZE + CLOSE_BOX_GAP)) - 8);
		mDragHandle->reshape(mDragHandle->getRect().getWidth(), mDragHandle->getRect().getHeight(), TRUE);
	}
}
void LLFloater::buildButtons()
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		LLRect btn_rect;
		if (mDragOnLeft)
		{
			btn_rect.setLeftTopAndSize(
				LLPANEL_BORDER_WIDTH,
				getRect().getHeight() - CLOSE_BOX_FROM_TOP - (LLFLOATER_CLOSE_BOX_SIZE + 1) * (i + 1),
				ll_round(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
				ll_round(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
		}
		else
		{
			btn_rect.setLeftTopAndSize(
				getRect().getWidth() - CLOSE_BOX_FROM_RIGHT - (LLFLOATER_CLOSE_BOX_SIZE + CLOSE_BOX_GAP) * (i + 1),
				getRect().getHeight() - CLOSE_BOX_FROM_TOP,
				ll_round(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
				ll_round(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
		}
		LLButton* buttonp = new LLButton(
			sButtonNames[i],
			btn_rect,
			sButtonActiveImageNames[i],
			sButtonPressedImageNames[i],
			LLStringUtil::null,
			boost::bind(sButtonCallbacks[i],this),
			LLFontGL::getFontSansSerif());
		buttonp->setTabStop(FALSE);
		buttonp->setFollowsTop();
		buttonp->setFollowsRight();
		buttonp->setToolTip( sButtonToolTips[i] );
		buttonp->setImageColor(LLUI::sColorsGroup->getColor("FloaterButtonImageColor"));
		buttonp->setImageHoverSelected(LLUI::getUIImage(sButtonPressedImageNames[i]));
		buttonp->setImageHoverUnselected(LLUI::getUIImage(sButtonPressedImageNames[i]));
		buttonp->setScaleImage(TRUE);
		buttonp->setHoverGlowStrength(0.22f);
		buttonp->setSaveToXML(false);
		addChild(buttonp);
		mButtons[i] = buttonp;
	}
	updateButtons();
}
LLFloaterView::LLFloaterView( const std::string& name, const LLRect& rect )
:	LLUICtrl( name, rect, FALSE, NULL, FOLLOWS_ALL ),
	mFocusCycleMode(FALSE),
	mSnapOffsetBottom(0)
{
	setTabStop(FALSE);
	resetStartingFloaterPosition();
}
void LLFloaterView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	reshapeFloater(width, height, called_from_parent, ADJUST_VERTICAL_YES);
}
void LLFloaterView::reshapeFloater(S32 width, S32 height, BOOL called_from_parent, BOOL adjust_vertical)
{
	S32 old_width = getRect().getWidth();
	S32 old_height = getRect().getHeight();
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		LLFloater* floaterp = (LLFloater*)viewp;
		if (floaterp->isDependent())
		{
			continue;
		}
		U32 follow_flags = 0x0;
		if (floaterp->isMinimized())
		{
			follow_flags |= (FOLLOWS_LEFT | FOLLOWS_TOP);
		}
		else
		{
			LLRect r = floaterp->getRect();
			S32 left_offset = llabs(r.mLeft - 0);
			S32 right_offset = llabs(old_width - r.mRight);
			S32 top_offset = llabs(old_height - r.mTop);
			S32 bottom_offset = llabs(r.mBottom - 0);
			if (left_offset < right_offset)
			{
				follow_flags |= FOLLOWS_LEFT;
			}
			else
			{
				follow_flags |= FOLLOWS_RIGHT;
			}
			if (!adjust_vertical)
			{
				follow_flags |= FOLLOWS_TOP;
			}
			else if (top_offset < bottom_offset)
			{
				follow_flags |= FOLLOWS_TOP;
			}
			else
			{
				follow_flags |= FOLLOWS_BOTTOM;
			}
		}
		floaterp->setFollows(follow_flags);
		for(LLFloater::handle_set_iter_t dependent_it = floaterp->mDependents.begin();
			dependent_it != floaterp->mDependents.end(); ++dependent_it)
		{
			LLFloater* dependent_floaterp = dependent_it->get();
			if (dependent_floaterp)
			{
				dependent_floaterp->setFollows(follow_flags);
			}
		}
	}
	LLView::reshape(width, height, called_from_parent);
}
void LLFloaterView::restoreAll()
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		floaterp->setMinimized(FALSE);
	}
}
void LLFloaterView::getNewFloaterPosition(S32* left,S32* top)
{
	static BOOL first = TRUE;
	if( first )
	{
		resetStartingFloaterPosition();
		first = FALSE;
	}
	const S32 FLOATER_PAD = 16;
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);
	LLRect full_window(0, window_size.mY, window_size.mX, 0);
	LLRect floater_creation_rect(
		160,
		full_window.getHeight() - 2 * MENU_BAR_HEIGHT,
		full_window.getWidth() * 2 / 3,
		130 );
	floater_creation_rect.stretch( -FLOATER_PAD );
	*left = mNextLeft;
	*top = mNextTop;
	const S32 STEP = 25;
	S32 bottom = floater_creation_rect.mBottom + 2 * STEP;
	S32 right = floater_creation_rect.mRight - 4 * STEP;
	mNextTop -= STEP;
	mNextLeft += STEP;
	if( (mNextTop < bottom ) || (mNextLeft > right) )
	{
		mColumn++;
		mNextTop = floater_creation_rect.mTop;
		mNextLeft = STEP * mColumn;
		if( (mNextTop < bottom) || (mNextLeft > right) )
		{
			resetStartingFloaterPosition();
		}
	}
}
void LLFloaterView::resetStartingFloaterPosition()
{
	const S32 FLOATER_PAD = 16;
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);
	LLRect full_window(0, window_size.mY, window_size.mX, 0);
	LLRect floater_creation_rect(
		160,
		full_window.getHeight() - 2 * MENU_BAR_HEIGHT,
		full_window.getWidth() * 2 / 3,
		130 );
	floater_creation_rect.stretch( -FLOATER_PAD );
	mNextLeft = floater_creation_rect.mLeft;
	mNextTop = floater_creation_rect.mTop;
	mColumn = 0;
}
LLRect LLFloaterView::findNeighboringPosition( LLFloater* reference_floater, LLFloater* neighbor )
{
	LLRect base_rect = reference_floater->getRect();
	S32 width = neighbor->getRect().getWidth();
	S32 height = neighbor->getRect().getHeight();
	LLRect new_rect = neighbor->getRect();
	LLRect expanded_base_rect = base_rect;
	expanded_base_rect.stretch(10);
	for(LLFloater::handle_set_iter_t dependent_it = reference_floater->mDependents.begin();
		dependent_it != reference_floater->mDependents.end(); ++dependent_it)
	{
		LLFloater* sibling = dependent_it->get();
		if (sibling &&
			sibling != neighbor &&
			sibling->getVisible() &&
			expanded_base_rect.overlaps(sibling->getRect()))
		{
			base_rect.unionWith(sibling->getRect());
		}
	}
	S32 left_margin = llmax(0, base_rect.mLeft);
	S32 right_margin = llmax(0, getRect().getWidth() - base_rect.mRight);
	S32 top_margin = llmax(0, getRect().getHeight() - base_rect.mTop);
	S32 bottom_margin = llmax(0, base_rect.mBottom);
	for (S32 i = 0; i < 5; i++)
	{
		if (right_margin > width)
		{
			new_rect.translate(base_rect.mRight - neighbor->getRect().mLeft, base_rect.mTop - neighbor->getRect().mTop);
			return new_rect;
		}
		else if (left_margin > width)
		{
			new_rect.translate(base_rect.mLeft - neighbor->getRect().mRight, base_rect.mTop - neighbor->getRect().mTop);
			return new_rect;
		}
		else if (bottom_margin > height)
		{
			new_rect.translate(base_rect.mLeft - neighbor->getRect().mLeft, base_rect.mBottom - neighbor->getRect().mTop);
			return new_rect;
		}
		else if (top_margin > height)
		{
			new_rect.translate(base_rect.mLeft - neighbor->getRect().mLeft, base_rect.mTop - neighbor->getRect().mBottom);
			return new_rect;
		}
		left_margin += 20;
		right_margin += 20;
		top_margin += 20;
		bottom_margin += 20;
	}
	return new_rect;
}
void LLFloaterView::bringToFront(LLFloater* child, BOOL give_focus)
{
	static bool recursive;
	if (recursive) { return; }
	AIRecursive enter(recursive);
	if (child->getHost())
 	{
		return;
	}
	std::vector<LLView*> floaters_to_move;
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		LLFloater *floater = (LLFloater *)viewp;
		if (child->isDependent())
		{
			LLFloater::handle_set_iter_t found_dependent = floater->mDependents.find(child->getHandle());
			if (found_dependent != floater->mDependents.end())
			{
				for(LLFloater::handle_set_iter_t dependent_it = floater->mDependents.begin();
					dependent_it != floater->mDependents.end(); )
				{
					LLFloater* sibling = dependent_it->get();
					if (sibling)
					{
						floaters_to_move.push_back(sibling);
					}
					++dependent_it;
				}
				floaters_to_move.push_back(floater);
			}
		}
	}
	std::vector<LLView*>::iterator view_it;
	for(view_it = floaters_to_move.begin(); view_it != floaters_to_move.end(); ++view_it)
	{
		LLFloater* floaterp = (LLFloater*)(*view_it);
		sendChildToFront(floaterp);
		if (!floaterp->isDependent())
		{
			floaterp->setMinimized(FALSE);
		}
	}
	floaters_to_move.clear();
	for(LLFloater::handle_set_iter_t dependent_it = child->mDependents.begin();
		dependent_it != child->mDependents.end(); )
	{
		LLFloater* dependent = dependent_it->get();
		if (dependent)
		{
			sendChildToFront(dependent);
		}
		++dependent_it;
	}
	if( *getChildList()->begin() != child )
	{
		sendChildToFront(child);
	}
	child->setMinimized(FALSE);
	if (give_focus && !gFocusMgr.childHasKeyboardFocus(child))
	{
		child->setFocus(TRUE);
		if (!child->hasFocus())
		{
			gFocusMgr.setKeyboardFocus(NULL);
		}
	}
}
void LLFloaterView::highlightFocusedFloater()
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater *floater = (LLFloater *)(*child_it);
		if (floater->isDependent())
		{
			continue;
		}
		BOOL floater_or_dependent_has_focus = gFocusMgr.childHasKeyboardFocus(floater);
		for(LLFloater::handle_set_iter_t dependent_it = floater->mDependents.begin();
			dependent_it != floater->mDependents.end();
			++dependent_it)
		{
			LLFloater* dependent_floaterp = dependent_it->get();
			if (dependent_floaterp && gFocusMgr.childHasKeyboardFocus(dependent_floaterp))
			{
				floater_or_dependent_has_focus = TRUE;
			}
		}
		floater->setForeground(floater_or_dependent_has_focus);
		for(LLFloater::handle_set_iter_t dependent_it = floater->mDependents.begin();
			dependent_it != floater->mDependents.end(); )
		{
			LLFloater* dependent_floaterp = dependent_it->get();
			if (dependent_floaterp)
			{
				dependent_floaterp->setForeground(floater_or_dependent_has_focus);
			}
			++dependent_it;
		}
		floater->cleanupHandles();
	}
}
void LLFloaterView::unhighlightFocusedFloater()
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater *floater = (LLFloater *)(*child_it);
		floater->setForeground(FALSE);
	}
}
void LLFloaterView::focusFrontFloater()
{
	LLFloater* floaterp = getFrontmost();
	if (floaterp)
	{
		floaterp->setFocus(TRUE);
	}
}
void LLFloaterView::getMinimizePosition(S32 *left, S32 *bottom)
{
	S32 col = 0;
	LLRect snap_rect_local = getLocalSnapRect();
	for(S32 row = snap_rect_local.mBottom;
		row < snap_rect_local.getHeight() - LLFLOATER_HEADER_SIZE;
		row += LLFLOATER_HEADER_SIZE )
	{
		for(col = snap_rect_local.mLeft;
			col < snap_rect_local.getWidth() - MINIMIZED_WIDTH;
			col += MINIMIZED_WIDTH)
		{
			bool foundGap = TRUE;
			for(child_list_const_iter_t child_it = getChildList()->begin();
				child_it != getChildList()->end();
				++child_it)
			{
				LLFloater* floater = (LLFloater*)((LLView*)*child_it);
				if(floater->isMinimized())
				{
					LLRect r = floater->getRect();
					if((r.mBottom < (row + LLFLOATER_HEADER_SIZE))
					   && (r.mBottom > (row - LLFLOATER_HEADER_SIZE))
					   && (r.mLeft < (col + MINIMIZED_WIDTH))
					   && (r.mLeft > (col - MINIMIZED_WIDTH)))
					{
						foundGap = FALSE;
						break;
					}
				}
			}
			if(foundGap)
			{
				*left = col;
				*bottom = row;
				return;
			}
		}
	}
	*left = snap_rect_local.mLeft;
	*bottom = snap_rect_local.mBottom;
}
void LLFloaterView::destroyAllChildren()
{
	LLView::deleteAllChildren();
}
void LLFloaterView::closeAllChildren(bool app_quitting)
{
	child_list_t child_list = *(getChildList());
	for (child_list_const_iter_t it = child_list.begin(); it != child_list.end(); ++it)
	{
		LLView* viewp = *it;
		child_list_const_iter_t exists = std::find(getChildList()->begin(), getChildList()->end(), viewp);
		if (exists == getChildList()->end())
		{
			continue;
		}
		LLFloater* floaterp = (LLFloater*)viewp;
		if (floaterp->canClose() && !floaterp->isDead() &&
			(app_quitting || floaterp->getVisible()))
		{
			floaterp->close(app_quitting);
		}
	}
}
void LLFloaterView::minimizeAllChildren()
{
	child_list_t child_list = *(getChildList());
	for (child_list_const_iter_t it = child_list.begin(); it != child_list.end(); ++it)
	{
		LLView* viewp = *it;
		child_list_const_iter_t exists = std::find(getChildList()->begin(), getChildList()->end(), viewp);
		if (exists == getChildList()->end())
		{
			continue;
		}
		LLFloater* floaterp = (LLFloater*)viewp;
		if (!floaterp->isDead())
		{
			floaterp->setMinimized(TRUE);
		}
	}
}
BOOL LLFloaterView::allChildrenClosed()
{
	for (child_list_const_iter_t it = getChildList()->begin(); it != getChildList()->end(); ++it)
	{
		LLView* viewp = *it;
		LLFloater* floaterp = (LLFloater*)viewp;
		if (floaterp->getVisible() && !floaterp->isDead() && floaterp->isCloseable())
		{
			return false;
		}
	}
	return true;
}
void LLFloaterView::refresh()
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater* floaterp = dynamic_cast<LLFloater*>(*child_it);
		if (floaterp && floaterp->getVisible() )
		{
			adjustToFitScreen(floaterp, !floaterp->isMinimized());
		}
	}
}
void LLFloaterView::adjustToFitScreen(LLFloater* floater, BOOL allow_partial_outside)
{
	if (floater->getParent() != this)
	{
		return;
	}
	LLRect::tCoordType screen_width = getSnapRect().getWidth();
	LLRect::tCoordType screen_height = getSnapRect().getHeight();
	if( floater->isResizable() && !floater->isMinimized() )
	{
		LLRect view_rect = floater->getRect();
		S32 old_width = view_rect.getWidth();
		S32 old_height = view_rect.getHeight();
		S32 min_width;
		S32 min_height;
		floater->getResizeLimits( &min_width, &min_height );
		S32 new_width = llmax( min_width, old_width );
		S32 new_height = llmax( min_height, old_height);
		if((new_width > screen_width) || (new_height > screen_height))
		{
			new_width = llmin(new_width, screen_width);
			new_height = llmin(new_height, screen_height);
			new_width = llmax(new_width, min_width);
			new_height = llmax(new_height, min_height);
			LLRect new_rect;
			new_rect.setLeftTopAndSize(view_rect.mLeft,view_rect.mTop,new_width, new_height);
			floater->setShape(new_rect);
			if (floater->followsRight())
			{
				floater->translate(old_width - new_width, 0);
			}
			if (floater->followsTop())
			{
				floater->translate(0, old_height - new_height);
			}
		}
	}
	if (floater->translateIntoRect( getLocalSnapRect(), allow_partial_outside ))
	{
		floater->clearSnapTarget();
	}
}
void LLFloaterView::draw()
{
	refresh();
	LLFloater* focused_floater = getFocusedFloater();
	if (mFocusCycleMode && focused_floater)
	{
		child_list_const_iter_t child_it = getChildList()->begin();
		for (;child_it != getChildList()->end(); ++child_it)
		{
			if ((*child_it) != focused_floater)
			{
				drawChild(*child_it);
			}
		}
		drawChild(focused_floater, -TABBED_FLOATER_OFFSET, TABBED_FLOATER_OFFSET);
	}
	else
	{
		LLView::draw();
	}
}
LLRect LLFloaterView::getSnapRect() const
{
	LLRect snap_rect = getRect();
	snap_rect.mBottom += mSnapOffsetBottom;
	return snap_rect;
}
LLFloater *LLFloaterView::getFocusedFloater() const
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLUICtrl* ctrlp = (*child_it)->isCtrl() ? static_cast<LLUICtrl*>(*child_it) : NULL;
		if ( ctrlp && ctrlp->hasFocus() )
		{
			return static_cast<LLFloater *>(ctrlp);
		}
	}
	return NULL;
}
LLFloater *LLFloaterView::getFrontmost() const
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if ( viewp->getVisible() && !viewp->isDead())
		{
			return (LLFloater *)viewp;
		}
	}
	return NULL;
}
LLFloater *LLFloaterView::getBackmost() const
{
	LLFloater* back_most = NULL;
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if ( viewp->getVisible() )
		{
			back_most = (LLFloater *)viewp;
		}
	}
	return back_most;
}
void LLFloaterView::syncFloaterTabOrder()
{
	for ( child_list_const_reverse_iter_t child_it = getChildList()->rbegin(); child_it != getChildList()->rend(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		if (gFocusMgr.childHasKeyboardFocus(floaterp))
		{
			bringToFront(floaterp, FALSE);
			break;
		}
	}
	for ( child_list_const_reverse_iter_t child_it = getChildList()->rbegin(); child_it != getChildList()->rend(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		moveChildToFrontOfTabGroup(floaterp);
	}
}
LLFloater*	LLFloaterView::getParentFloater(LLView* viewp) const
{
	LLView* parentp = viewp->getParent();
	while(parentp && parentp != this)
	{
		viewp = parentp;
		parentp = parentp->getParent();
	}
	if (parentp == this)
	{
		return (LLFloater*)viewp;
	}
	return NULL;
}
S32 LLFloaterView::getZOrder(LLFloater* child)
{
	S32 rv = 0;
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if(viewp == child)
		{
			break;
		}
		++rv;
	}
	return rv;
}
void LLFloaterView::pushVisibleAll(BOOL visible, const skip_list_t& skip_list)
{
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		if (skip_list.find(view) == skip_list.end())
		{
			view->pushVisible(visible);
		}
	}
}
void LLFloaterView::popVisibleAll(const skip_list_t& skip_list)
{
	child_list_t child_list_copy = *getChildList();
	for (child_list_const_iter_t child_iter = child_list_copy.begin();
		 child_iter != child_list_copy.end(); ++child_iter)
	{
		LLView *view = *child_iter;
		if (skip_list.find(view) == skip_list.end())
		{
			view->popVisible();
		}
	}
}
LLXMLNodePtr LLFloater::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLPanel::getXML();
	node->setName(LL_FLOATER_TAG);
	node->createChild("title", TRUE)->setStringValue(getCurrentTitle());
	node->createChild("can_resize", TRUE)->setBoolValue(isResizable());
	node->createChild("can_minimize", TRUE)->setBoolValue(isMinimizeable());
	node->createChild("can_close", TRUE)->setBoolValue(isCloseable());
	node->createChild("can_drag_on_left", TRUE)->setBoolValue(isDragOnLeft());
	node->createChild("min_width", TRUE)->setIntValue(getMinWidth());
	node->createChild("min_height", TRUE)->setIntValue(getMinHeight());
	node->createChild("can_tear_off", TRUE)->setBoolValue(mCanTearOff);
	return node;
}
LLView* LLFloater::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string name("floater");
	node->getAttributeString("name", name);
	LLFloater *floaterp = new LLFloater(name);
	std::string filename;
	node->getAttributeString("filename", filename);
	if (filename.empty())
	{
		floaterp->getCommitCallbackRegistrar().pushScope();
		floaterp->getEnableCallbackRegistrar().pushScope();
		floaterp->initFloaterXML(node, parent, factory);
		floaterp->getCommitCallbackRegistrar().popScope();
		floaterp->getEnableCallbackRegistrar().popScope();
	}
	else
	{
		factory->buildFloater(floaterp, filename);
	}
	return floaterp;
}
LLTrace::BlockTimerStatHandle POST_BUILD("Floater Post Build");
void LLFloater::initFloaterXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory, BOOL open)
{
	std::string name(getName());
	std::string title(getCurrentTitle());
	std::string short_title(getShortTitle());
	std::string rect_control("");
	BOOL resizable = isResizable();
	S32 min_width = getMinWidth();
	S32 min_height = getMinHeight();
	BOOL drag_on_left = isDragOnLeft();
	BOOL minimizable = isMinimizeable();
	BOOL close_btn = isCloseable();
	LLRect rect;
	node->getAttributeString("name", name);
	node->getAttributeString("title", title);
	node->getAttributeString("short_title", short_title);
	node->getAttributeString("rect_control", rect_control);
	node->getAttributeBOOL("can_resize", resizable);
	node->getAttributeBOOL("can_minimize", minimizable);
	node->getAttributeBOOL("can_close", close_btn);
	node->getAttributeBOOL("can_drag_on_left", drag_on_left);
	node->getAttributeS32("min_width", min_width);
	node->getAttributeS32("min_height", min_height);
	if (! rect_control.empty())
	{
		setRectControl(rect_control);
	}
	createRect(node, rect, parent, LLRect());
	setRect(rect);
	setName(name);
	initFloater(title,
			resizable,
			min_width,
			min_height,
			drag_on_left,
			minimizable,
			close_btn);
	setTitle(title);
	applyTitle ();
	setShortTitle(short_title);
	std::string title_icon;
	node->getAttributeString("title_icon", title_icon);
	if (!title_icon.empty() && mDragHandle)
	{
		mDragHandle->setTitleIcon(title_icon);
	}
	BOOL can_tear_off;
	if (node->getAttributeBOOL("can_tear_off", can_tear_off))
	{
		setCanTearOff(can_tear_off);
	}
	initFromXML(node, parent);
	LLMultiFloater* last_host = LLFloater::getFloaterHost();
	if (node->hasName("multi_floater"))
	{
		LLFloater::setFloaterHost((LLMultiFloater*) this);
	}
	initChildrenXML(node, factory);
	if (node->hasName("multi_floater"))
	{
		LLFloater::setFloaterHost(last_host);
	}
	BOOL result;
	{
		LL_RECORD_BLOCK_TIME(POST_BUILD);
		result = postBuild();
	}
	if (!result)
	{
		LL_ERRS() << "Failed to construct floater " << name << LL_ENDL;
	}
	applyRectControl();
	if (open)
	{
		this->open();
	}
	moveResizeHandlesToFront();
}
void VisibilityPolicy<LLFloater>::show(LLFloater* instance, const LLSD& key)
{
	if (instance)
	{
		instance->open();
		if (instance->getHost())
		{
			instance->getHost()->open();
		}
	}
}
