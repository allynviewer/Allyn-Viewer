/**
 * @file llfloatingbarstyle.cpp
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
#include "llfloatingbarstyle.h"
#include "llbutton.h"
#include "llfloater.h"
#include "llhudview.h"
#include "llnavigationbar.h"
#include "llnotify.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
const S32 LLFloatingBarStyle::TOP_BAR_HEIGHT;
const S32 LLFloatingBarStyle::NAV_BAR_PAD;
const S32 LLFloatingBarStyle::BOTTOM_BAR_HEIGHT;
const S32 LLFloatingBarStyle::BAR_GAP;
const S32 LLFloatingBarStyle::CAP_PAD;
const S32 LLFloatingBarStyle::ROUND_BUTTON_SIZE;
const S32 LLFloatingBarStyle::ICON_BUTTON_SIZE;
LLFloatingBarStyle::LLFloatingBarStyle()
:	mFill(LLUI::getUIImage("Floating_Pill")),
	mBorder(LLUI::getUIImage("Floating_Pill_Border"))
{
}
void LLFloatingBarStyle::drawPill(const LLRect& rect, const LLColor4& color) const
{
	if (rect.getWidth() <= 0 || rect.getHeight() <= 0)
	{
		return;
	}
	static const LLCachedControl<LLColor4> border_color(*LLUI::sColorsGroup, "FloatingBarBorderColor", LLColor4(1.f, 1.f, 1.f, 0.11f));
	LLColor4 fill = color;
	fill.mV[VALPHA] *= opacity();
	if (mFill.notNull())
	{
		mFill->draw(rect, fill);
	}
	if (mBorder.notNull())
	{
		mBorder->draw(rect, border_color);
	}
}
void LLFloatingBarStyle::drawDisc(const LLRect& rect, const LLColor4& color) const
{
	if (rect.getWidth() <= 0 || rect.getHeight() <= 0)
	{
		return;
	}
	if (mFill.notNull())
	{
		mFill->draw(rect, color);
	}
}
S32 LLFloatingBarStyle::margin()
{
	static const LLCachedControl<S32> bar_margin(gSavedSettings, "FloatingBarMargin", 8);
	return llclamp((S32)bar_margin, 0, 32);
}
F32 LLFloatingBarStyle::opacity()
{
	static const LLCachedControl<F32> bar_opacity(gSavedSettings, "FloatingBarOpacity", 0.92f);
	return llclamp((F32)bar_opacity, 0.2f, 1.f);
}
S32 LLFloatingBarStyle::iconSpacing()
{
	static const LLCachedControl<S32> icon_spacing(gSavedSettings, "FloatingBarIconSpacing", 4);
	return llclamp((S32)icon_spacing, 0, 16);
}
LLFloatingBarStyle::EAlignment LLFloatingBarStyle::bottomAlignment()
{
	static const LLCachedControl<S32> alignment(gSavedSettings, "FloatingBarBottomAlignment", 1);
	return (EAlignment)llclamp((S32)alignment, (S32)ALIGN_LEFT, (S32)ALIGN_RIGHT);
}
LLColor4 LLFloatingBarStyle::barColor()
{
	static const LLCachedControl<LLColor4> bar_color(*LLUI::sColorsGroup, "MenuBarBgColor", LLColor4(0.125f, 0.125f, 0.14f, 1.f));
	return bar_color;
}
LLColor4 LLFloatingBarStyle::accentColor()
{
	static const LLCachedControl<LLColor4> accent_color(*LLUI::sColorsGroup, "FloatingBarAccentColor", LLColor4(0.545f, 0.36f, 0.965f, 1.f));
	return accent_color;
}
S32 LLFloatingBarStyle::topBarHeight()
{
	if (gNavigationBar && gNavigationBar->isCompact())
	{
		return COMPACT_TOP_BAR_HEIGHT;
	}
	return TOP_BAR_HEIGHT;
}
S32 LLFloatingBarStyle::topBarInset()
{
	return margin() + topBarHeight() + BAR_GAP;
}
S32 LLFloatingBarStyle::topReserved()
{
	return topBarInset() + (gNavigationBar ? gNavigationBar->getOccupiedHeight() : 0);
}
void LLFloatingBarStyle::applyTopLayout()
{
	if (!gViewerWindow || !gViewerWindow->getRootView())
	{
		return;
	}
	const LLRect root_rect = gViewerWindow->getRootView()->getRect();
	const S32 root_height = root_rect.getHeight();
	const S32 content_top = root_height - topReserved();
	if (gFloaterView)
	{
		LLRect r = gFloaterView->getRect();
		if (r.mLeft != 0 || r.mBottom != 0 || r.mRight != root_rect.getWidth() || r.mTop != root_height)
		{
			r.set(0, root_height, root_rect.getWidth(), 0);
			gFloaterView->setRect(r);
		}
		gFloaterView->setSnapOffsetBottom(0);
	}
	if (gHUDView)
	{
		LLRect r = gHUDView->getRect();
		if (r.mTop != content_top)
		{
			r.mTop = content_top;
			gHUDView->reshape(r.getWidth(), r.getHeight(), TRUE);
			gHUDView->setRect(r);
		}
	}
	if (gNotifyBoxView)
	{
		LLRect r = gNotifyBoxView->getRect();
		const S32 notify_top = root_height - topBarInset();
		if (r.mTop != notify_top)
		{
			r.mTop = notify_top;
			gNotifyBoxView->setShape(r);
		}
		gNotifyBoxView->layoutStack();
	}
}
void LLFloatingBarStyle::styleIconButton(LLButton* button)
{
	if (!button)
	{
		return;
	}
	LLUIImagePtr idle = LLUI::getUIImage("Floating_Btn_Idle");
	LLUIImagePtr active = LLUI::getUIImage("Floating_Btn_Active");
	LLUIImagePtr pressed = LLUI::getUIImage("Floating_Btn_Pressed");
	button->setImageUnselected(idle);
	button->setImageSelected(active);
	button->setImageHoverUnselected(LLUI::getUIImage("Floating_Btn_Hover"));
	button->setImageHoverSelected(LLUI::getUIImage("Floating_Btn_Active_Hover"));
	button->setImageDisabled(idle);
	button->setImageDisabledSelected(active);
	button->setImagePressed(pressed);
	button->setImagePressedSelected(pressed);
	button->setScaleImage(TRUE);
	button->setImageOverlaySelectedColor(accentColor());
}
void LLFloatingBarStyle::styleRoundButton(LLButton* button)
{
	if (!button)
	{
		return;
	}
	LLUIImagePtr idle = LLUI::getUIImage("Floating_Btn_Idle");
	LLUIImagePtr disc = LLUI::getUIImage("Floating_Pill");
	button->setImageUnselected(idle);
	button->setImageSelected(idle);
	button->setImageHoverUnselected(disc);
	button->setImageHoverSelected(disc);
	button->setImageDisabled(idle);
	button->setImageDisabledSelected(idle);
	button->setImagePressed(disc);
	button->setImagePressedSelected(disc);
	button->setImageColor(LLColor4(1.f, 1.f, 1.f, 0.14f));
	button->setScaleImage(TRUE);
}
void LLFloatingBarStyle::labelToToolTip(LLButton* button)
{
	if (!button)
	{
		return;
	}
	button->setShowLabelInToolTip(TRUE);
}
boost::signals2::connection LLFloatingBarStyle::connectSetting(const std::string& name, const boost::function<void()>& callback)
{
	LLControlVariable* control = gSavedSettings.getControl(name);
	if (!control)
	{
		return boost::signals2::connection();
	}
	return control->getSignal()->connect(boost::bind(callback));
}
