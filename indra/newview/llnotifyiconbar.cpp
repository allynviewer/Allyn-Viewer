/**
 * @file llnotifyiconbar.cpp
 * @brief Compact notification icon below the navigation bar.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (C) 2026 - Allyn Viewer
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
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llnotifyiconbar.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llfloaternotifywell.h"
#include "llfontgl.h"
#include "llformat.h"
#include "llmenugl.h"
#include "llnavigationbar.h"
#include "llnotify.h"
#include "llstatusbar.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llviewermenu.h"
#include "llviewerwindow.h"
LLNotifyIconBar* gNotifyIconBar = NULL;
S32 NOTIFY_ICON_BAR_HEIGHT = LLFloatingBarStyle::ROUND_BUTTON_SIZE;
S32 NOTIFY_ICON_BAR_WIDTH = LLFloatingBarStyle::ROUND_BUTTON_SIZE;
namespace
{
const S32 NOTIFY_PAD_LEFT = 8;
const S32 NOTIFY_PAD_RIGHT = 12;
const S32 NOTIFY_ICON = 16;
const S32 NOTIFY_ICON_GAP = 6;
std::string notifyCountText(S32 count)
{
	return llformat("%d", llmax(0, count));
}
S32 notifyBarWidth(S32 count)
{
	const S32 size = LLFloatingBarStyle::ROUND_BUTTON_SIZE;
	if (count <= 0)
	{
		return size;
	}
	const LLFontGL* font = LLFontGL::getFontSansSerifBold();
	S32 text_w = font ? font->getWidth(notifyCountText(count)) : size;
	text_w = llmax(text_w, 8);
	return llmax(size, NOTIFY_PAD_LEFT + NOTIFY_ICON + NOTIFY_ICON_GAP + text_w + NOTIFY_PAD_RIGHT);
}
void setNotifyButtonChrome(LLButton* button, bool active)
{
	if (!button)
	{
		return;
	}
	button->setLabel(LLStringExplicit(""));
	button->setLeftHPad(0);
	button->setRightHPad(0);
	if (active)
	{
		LLUIImagePtr pill = LLUI::getUIImage("Floating_Pill");
		button->setImageUnselected(pill);
		button->setImageSelected(pill);
		button->setImageHoverUnselected(pill);
		button->setImageHoverSelected(pill);
		button->setImageDisabled(pill);
		button->setImageDisabledSelected(pill);
		button->setImagePressed(pill);
		button->setImagePressedSelected(pill);
		button->setImageColor(LLColor4(1.f, 1.f, 1.f, 0.f));
		button->setImageOverlay(LLStringUtil::null);
	}
	else
	{
		LLFloatingBarStyle::styleRoundButton(button);
		button->setImageOverlay("floating_ui/bell.png", LLFontGL::HCENTER, LLColor4::white);
	}
}
void drawNotifyCount(const LLRect& rect, S32 count)
{
	if (count <= 0)
	{
		return;
	}
	const LLFontGL* font = LLFontGL::getFontSansSerifBold();
	if (!font)
	{
		return;
	}
	const S32 height = rect.getHeight();
	LLUIImagePtr bell = LLUI::getUIImage("floating_ui/bell.png");
	if (bell.notNull())
	{
		bell->draw(NOTIFY_PAD_LEFT, (height - NOTIFY_ICON) / 2, NOTIFY_ICON, NOTIFY_ICON, LLColor4::white);
	}
	font->renderUTF8(notifyCountText(count), 0,
		(F32)(NOTIFY_PAD_LEFT + NOTIFY_ICON + NOTIFY_ICON_GAP),
		(F32)(height / 2),
		LLColor4::white,
		LLFontGL::LEFT,
		LLFontGL::VCENTER,
		LLFontGL::NORMAL,
		LLFontGL::DROP_SHADOW_SOFT,
		S32_MAX,
		S32_MAX);
}
}
LLNotifyIconBar::LLNotifyIconBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, LLRect(), FALSE),
	mWellBtn(NULL),
	mMouselookHidden(false),
	mLastNotifyCount(-1)
{
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);
	setBackgroundVisible(FALSE);
	setBackgroundOpaque(FALSE);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_notify_icon_bar.xml");
	reshape(LLFloatingBarStyle::ROUND_BUTTON_SIZE, LLFloatingBarStyle::ROUND_BUTTON_SIZE, TRUE);
	setFollows(FOLLOWS_RIGHT | FOLLOWS_TOP);
	mWellBtn = getChild<LLButton>("notify_well_btn", TRUE, FALSE);
	if (mWellBtn)
	{
		mWellBtn->setClickedCallback(boost::bind(&LLNotifyIconBar::onWellClicked, this));
		mWellBtn->setFont(LLFontGL::getFontSansSerifBold());
		mWellBtn->setHAlign(LLFontGL::HCENTER);
		LLFloatingBarStyle::styleRoundButton(mWellBtn);
		mWellBtn->setLeftHPad(0);
		mWellBtn->setRightHPad(0);
		mWellBtn->setLabel(LLStringExplicit(""));
		mWellBtn->setUnselectedLabelColor(LLColor4::white);
		mWellBtn->setSelectedLabelColor(LLColor4::white);
		mWellBtn->setDropShadowedText(TRUE);
	}
	setRect(rect);
	mMarginSlot = LLFloatingBarStyle::connectSetting("FloatingBarMargin", boost::bind(&LLNotifyIconBar::updatePosition, this));
}
LLNotifyIconBar::~LLNotifyIconBar()
{
	mMarginSlot.disconnect();
	if (gNotifyIconBar == this)
	{
		gNotifyIconBar = NULL;
	}
}
void LLNotifyIconBar::draw()
{
	refreshBadges();
	const bool active = mLastNotifyCount > 0;
	mStyle.drawPill(getLocalRect(), active ? LLFloatingBarStyle::accentColor() : LLFloatingBarStyle::barColor());
	if (active)
	{
		LLUIImagePtr border = LLUI::getUIImage("Floating_Pill_Border");
		if (border.notNull())
		{
			const F32 wave = 0.5f + 0.5f * sinf((F32)LLFrameTimer::getElapsedSeconds() * 6.f);
			LLColor4 orange(1.f, 0.42f, 0.f, 0.2f + 0.8f * wave);
			border->draw(getLocalRect(), orange);
		}
	}
	LLPanel::draw();
	if (active)
	{
		drawNotifyCount(getLocalRect(), mLastNotifyCount);
	}
}
void LLNotifyIconBar::setVisibleForMouselook(bool visible)
{
	mMouselookHidden = !visible;
	bool show = visible && !gAgentCamera.cameraMouselook();
	if (getVisible() != (BOOL)show)
	{
		setVisible(show);
		if (gNotifyBoxView)
		{
			gNotifyBoxView->layoutStack();
		}
	}
}
void LLNotifyIconBar::updatePosition()
{
	static bool sInPosition = false;
	if (sInPosition)
	{
		return;
	}
	LLView* parent = getParent();
	if (!parent)
	{
		return;
	}
	sInPosition = true;
	const S32 margin = LLFloatingBarStyle::margin();
	const S32 height = LLFloatingBarStyle::ROUND_BUTTON_SIZE;
	const S32 width = notifyBarWidth(llmax(0, mLastNotifyCount));
	const S32 top = parent->getRect().getHeight() - margin;
	const S32 right = parent->getRect().getWidth() - margin;
	const S32 previous_width = NOTIFY_ICON_BAR_WIDTH;
	NOTIFY_ICON_BAR_WIDTH = width;
	NOTIFY_ICON_BAR_HEIGHT = height;
	const LLRect next(right - width, top, right, top - height);
	const bool shape_changed = getRect() != next;
	if (shape_changed)
	{
		setShape(next);
	}
	if (gStatusBar && (shape_changed || previous_width != width))
	{
		gStatusBar->updateBarRect();
	}
	sInPosition = false;
}
void LLNotifyIconBar::refreshBadges()
{
	S32 notify_count = gNotifyBoxView ? gNotifyBoxView->getOverflowCount() : 0;
	if (!mWellBtn)
	{
		return;
	}
	const bool count_changed = notify_count != mLastNotifyCount;
	if (count_changed)
	{
		mLastNotifyCount = notify_count;
		setNotifyButtonChrome(mWellBtn, notify_count > 0);
	}
	if (count_changed || getRect().getWidth() != notifyBarWidth(notify_count))
	{
		updatePosition();
	}
}
void LLNotifyIconBar::onWellClicked()
{
	LLFloaterNotifyWell::toggleInstance();
	if (LLFloaterNotifyWell::instanceVisible() && gNotifyBoxView)
	{
		gNotifyBoxView->layoutStack();
	}
}
