/**
 * @file llfloaterallynupdate.h
 * @brief Floater offering to download and install a newer Allyn Viewer
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
#ifndef LL_LLFLOATERALLYNUPDATE_H
#define LL_LLFLOATERALLYNUPDATE_H

#include "allynupdate.h"
#include "llfloater.h"
#include "llstring.h"

class LLProgressBar;
class LLTextBox;

class LLFloaterAllynUpdate : public LLFloater, public LLFloaterSingleton<LLFloaterAllynUpdate>
{
	friend class LLUISingleton<LLFloaterAllynUpdate, VisibilityPolicy<LLFloater> >;
public:
	static void offer(const LLSD& info);
	BOOL postBuild() override;
	void onOpen() override;
	void draw() override;
	void onClose(bool app_quitting) override;
	void setUpdateInfo(const LLSD& info);

private:
	LLFloaterAllynUpdate(const LLSD& seed);
	~LLFloaterAllynUpdate();

	void refreshMessage();
	void setStatusFromTemplate(const std::string& string_name, const LLStringUtil::format_map_t& extra);
	void refreshTransferStats();
	void updateProgressUi();
	void setButtonsEnabled(bool enabled);
	void onUpdateNow();
	void onUpdateLater();
	void onCloseBtn();

	std::string mLabel;
	std::string mCurrent;
	std::string mInstallerUrl;
	LLProgressBar* mProgressBar;
	LLTextBox* mMessageText;
	LLTextBox* mPercentText;
	LLTextBox* mStatsText;
	LLTextBox* mStatusText;
	int mLastLoggedPercent;
	AllynUpdateDownloadState mHandledState;
};

#endif
