/**
 * @file llnotifyiconbar.h
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
#ifndef LL_LLNOTIFYICONBAR_H
#define LL_LLNOTIFYICONBAR_H
#include "llpanel.h"
#include "llfloatingbarstyle.h"
class LLButton;
extern S32 NOTIFY_ICON_BAR_HEIGHT;
extern S32 NOTIFY_ICON_BAR_WIDTH;
class LLNotifyIconBar : public LLPanel
{
public:
	LLNotifyIconBar(const std::string& name, const LLRect& rect);
	virtual ~LLNotifyIconBar();
	void draw() override;
	void setVisibleForMouselook(bool visible);
	void updatePosition();
private:
	void refreshBadges();
	void onWellClicked();
	LLButton* mWellBtn;
	bool mMouselookHidden;
	S32 mLastNotifyCount;
	LLFloatingBarStyle mStyle;
	boost::signals2::connection mMarginSlot;
};
extern LLNotifyIconBar* gNotifyIconBar;
#endif
