/**
 * @file llprefstranslation.h
 * @brief Preferences panel for AI chat translation
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
#ifndef LL_LLPREFSTRANSLATION_H
#define LL_LLPREFSTRANSLATION_H
#include "llpanel.h"
#include "v4color.h"
class LLScrollListCtrl;
class LLTextBox;
class LLPrefsTranslation : public LLPanel
{
public:
	LLPrefsTranslation();
	~LLPrefsTranslation();
	void apply();
	void cancel();
	void refresh();
	void refreshValues();
	void onNoticeAccepted();
	void onNoticeDeclined();
private:
	void onProviderCommit();
	void onEnableCheck();
	void onTestConnection();
	void onConnectionResult(bool success, const std::string& message);
	void setStatus(S32 state, const std::string& message);
	void markStatusStale();
	void syncAllFieldsToSettings();
	void syncProviderFieldsToSettings();
	void syncGroupSelectionToSettings();
	void refreshGroupList();
	void setEnableCheck(bool enabled);
	void pushSettingsToWidgets();
	LLTextBox* mStatusText;
	LLScrollListCtrl* mGroupList;
	bool mIgnoreEnableCommit;
	bool mIgnoreProviderCommit;
	bool mEnabled;
	std::string mScope;
	std::string mSourceLang;
	std::string mTargetLang;
	bool mIncoming;
	bool mOutgoing;
	std::string mIncomingDisplay;
	std::string mOutgoingDisplay;
	LLColor4 mIncomingColor;
	LLColor4 mOutgoingColor;
	std::string mProvider;
	std::string mApiKey;
	std::string mBaseUrl;
	std::string mModel;
	std::string mApiStyle;
	bool mGroupsEnabled;
	std::string mGroupMode;
	std::string mGroupIDs;
};
#endif
