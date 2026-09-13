/**
 * @file llprefstranslation.h
 * @brief Preferences panel for AI chat translation
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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
