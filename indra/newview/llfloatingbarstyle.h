/**
 * @file llfloatingbarstyle.h
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
#ifndef LL_LLFLOATINGBARSTYLE_H
#define LL_LLFLOATINGBARSTYLE_H
#include "llrect.h"
#include "lluiimage.h"
#include "v4color.h"
#include <boost/function.hpp>
#include <boost/signals2.hpp>
class LLButton;
class LLFloatingBarStyle
{
public:
	enum EAlignment
	{
		ALIGN_LEFT = 0,
		ALIGN_CENTER = 1,
		ALIGN_RIGHT = 2
	};
	static const S32 TOP_BAR_HEIGHT = 28;
	static const S32 COMPACT_TOP_BAR_HEIGHT = 22;
	static const S32 NAV_BAR_PAD = 3;
	static const S32 BOTTOM_BAR_HEIGHT = 40;
	static const S32 BAR_GAP = 4;
	static const S32 CAP_PAD = 12;
	static const S32 ROUND_BUTTON_SIZE = 28;
	static const S32 ICON_BUTTON_SIZE = 32;
	LLFloatingBarStyle();
	void drawPill(const LLRect& rect, const LLColor4& color) const;
	void drawDisc(const LLRect& rect, const LLColor4& color) const;
	static S32 margin();
	static F32 opacity();
	static S32 iconSpacing();
	static EAlignment bottomAlignment();
	static LLColor4 barColor();
	static LLColor4 accentColor();
	static S32 topBarHeight();
	static S32 topBarInset();
	static S32 topReserved();
	static void applyTopLayout();
	static void styleIconButton(LLButton* button);
	static void styleRoundButton(LLButton* button);
	static void labelToToolTip(LLButton* button);
	static boost::signals2::connection connectSetting(const std::string& name, const boost::function<void()>& callback);
private:
	LLUIImagePtr mFill;
	LLUIImagePtr mBorder;
};
#endif
