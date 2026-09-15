/**
 * @file llfloaterallynupdate.h
 * @brief Floater offering to download and install a newer Allyn Viewer
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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
