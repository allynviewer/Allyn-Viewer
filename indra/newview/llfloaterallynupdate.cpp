/**
 * @file llfloaterallynupdate.cpp
 * @brief Floater offering to download and install a newer Allyn Viewer
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llfloaterallynupdate.h"

#include "llbutton.h"
#include "llcontrol.h"
#include "llprogressbar.h"
#include "llstring.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"

void LLFloaterAllynUpdate::offer(const LLSD& info)
{
	allyn_update_log("showing update floater label=" + info["label"].asString()
		+ " installer=" + info["installerUrl"].asString());
	LLFloaterAllynUpdate* floater = showInstance(info);
	if (floater)
	{
		floater->setUpdateInfo(info);
		floater->center();
	}
}

LLFloaterAllynUpdate::LLFloaterAllynUpdate(const LLSD& seed)
:	LLFloater(std::string("allyn update floater"))
,	mProgressBar(NULL)
,	mMessageText(NULL)
,	mPercentText(NULL)
,	mStatsText(NULL)
,	mStatusText(NULL)
,	mLastLoggedPercent(-1)
,	mHandledState(ALLYN_UPDATE_IDLE)
{
	setUpdateInfo(seed);
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_allyn_update.xml");
}

LLFloaterAllynUpdate::~LLFloaterAllynUpdate()
{
}

void LLFloaterAllynUpdate::setUpdateInfo(const LLSD& info)
{
	mLabel = info["label"].asString();
	mCurrent = info["current"].asString();
	mInstallerUrl = info["installerUrl"].asString();
	if (mMessageText)
	{
		refreshMessage();
	}
}

BOOL LLFloaterAllynUpdate::postBuild()
{
	mProgressBar = getChild<LLProgressBar>("download_progress");
	mMessageText = getChild<LLTextBox>("message_text");
	mPercentText = getChild<LLTextBox>("percent_text");
	mStatsText = getChild<LLTextBox>("stats_text");
	mStatusText = getChild<LLTextBox>("status_text");

	childSetAction("update_now_btn", boost::bind(&LLFloaterAllynUpdate::onUpdateNow, this));
	childSetAction("update_later_btn", boost::bind(&LLFloaterAllynUpdate::onUpdateLater, this));
	childSetAction("close_btn", boost::bind(&LLFloaterAllynUpdate::onCloseBtn, this));

	refreshMessage();
	if (mProgressBar)
	{
		LLRect bar_rect = mProgressBar->getRect();
		if (bar_rect.getHeight() != 14)
		{
			bar_rect.mTop = bar_rect.mBottom + 14;
			mProgressBar->setRect(bar_rect);
		}
		mProgressBar->setPercent(0.f);
	}
	if (mPercentText)
	{
		mPercentText->setText(LLStringExplicit("0%"));
	}
	if (mStatsText)
	{
		mStatsText->setText(LLStringExplicit(""));
		mStatsText->setVisible(FALSE);
	}
	if (mStatusText)
	{
		mStatusText->setVisible(TRUE);
	}
	setStatusFromTemplate("status_ready", LLStringUtil::format_map_t());
	center();
	return TRUE;
}

void LLFloaterAllynUpdate::onOpen()
{
	center();
}

void LLFloaterAllynUpdate::refreshMessage()
{
	if (!mMessageText)
	{
		return;
	}
	LLStringUtil::format_map_t args = LLTrans::getDefaultArgs();
	args["[RECOMMENDED_VER]"] = mLabel.empty() ? std::string("?") : mLabel;
	args["[CURRENT_VER]"] = mCurrent.empty() ? std::string("?") : mCurrent;
	mMessageText->setText(LLStringExplicit(getString("update_message", args)));
}

void LLFloaterAllynUpdate::setStatusFromTemplate(const std::string& string_name, const LLStringUtil::format_map_t& extra)
{
	if (!mStatusText)
	{
		return;
	}
	LLStringUtil::format_map_t args = LLTrans::getDefaultArgs();
	args.insert(extra.begin(), extra.end());
	mStatusText->setText(LLStringExplicit(getString(string_name, args)));
}

void LLFloaterAllynUpdate::refreshTransferStats()
{
	if (!mStatsText)
	{
		return;
	}

	const S64 downloaded = allyn_update_download_bytes();
	const S64 total = allyn_update_download_total();
	const S64 speed = allyn_update_download_speed_bps();
	LLStringUtil::format_map_t extra;
	extra["[DOWNLOADED]"] = allyn_update_format_bytes(downloaded);
	extra["[TOTAL]"] = total > 0 ? allyn_update_format_bytes(total) : std::string("--");
	extra["[SPEED]"] = allyn_update_format_speed(speed);

	const std::string key = (total > 0) ? "status_stats" : "status_stats_unknown";
	LLStringUtil::format_map_t args = LLTrans::getDefaultArgs();
	args.insert(extra.begin(), extra.end());
	mStatsText->setText(LLStringExplicit(getString(key, args)));
}

void LLFloaterAllynUpdate::setButtonsEnabled(bool enabled)
{
	childSetEnabled("update_now_btn", enabled);
}

void LLFloaterAllynUpdate::draw()
{
	updateProgressUi();
	LLFloater::draw();
}

void LLFloaterAllynUpdate::updateProgressUi()
{
	const AllynUpdateDownloadState state = allyn_update_download_state();
	const int percent = allyn_update_download_percent();

	if (mProgressBar)
	{
		mProgressBar->setPercent(static_cast<F32>(percent));
	}
	if (mPercentText)
	{
		mPercentText->setText(LLStringExplicit(llformat("%d%%", percent)));
	}

	if (state == ALLYN_UPDATE_DOWNLOADING)
	{
		setButtonsEnabled(false);
		if (mStatusText)
		{
			mStatusText->setVisible(FALSE);
		}
		if (mStatsText)
		{
			mStatsText->setVisible(TRUE);
		}
		refreshTransferStats();
		if (percent / 10 != mLastLoggedPercent / 10)
		{
			mLastLoggedPercent = percent;
			LL_INFOS("AllynUpdate") << "UI download " << percent << "% "
				<< allyn_update_format_bytes(allyn_update_download_bytes()) << "/"
				<< allyn_update_format_bytes(allyn_update_download_total()) << " "
				<< allyn_update_format_speed(allyn_update_download_speed_bps()) << LL_ENDL;
		}
		return;
	}

	if (mHandledState == state)
	{
		return;
	}

	if (state == ALLYN_UPDATE_DONE)
	{
		mHandledState = state;
		if (mStatsText)
		{
			mStatsText->setVisible(FALSE);
		}
		if (mStatusText)
		{
			mStatusText->setVisible(TRUE);
		}
		setStatusFromTemplate("status_launching", LLStringUtil::format_map_t());
		allyn_update_log("UI received download complete");
		if (!allyn_update_launch_installer_and_quit())
		{
			setButtonsEnabled(true);
			LLStringUtil::format_map_t extra;
			extra["[ERROR]"] = std::string("Could not start installer");
			setStatusFromTemplate("status_failed", extra);
		}
		return;
	}

	if (state == ALLYN_UPDATE_SIMULATED)
	{
		mHandledState = state;
		setButtonsEnabled(true);
		if (mStatsText)
		{
			mStatsText->setVisible(FALSE);
		}
		if (mStatusText)
		{
			mStatusText->setVisible(TRUE);
		}
		setStatusFromTemplate("status_simulate", LLStringUtil::format_map_t());
		allyn_update_log("UI received simulated download complete");
		return;
	}

	if (state == ALLYN_UPDATE_FAILED)
	{
		mHandledState = state;
		setButtonsEnabled(true);
		if (mStatsText)
		{
			mStatsText->setVisible(FALSE);
		}
		if (mStatusText)
		{
			mStatusText->setVisible(TRUE);
		}
		LLStringUtil::format_map_t extra;
		extra["[ERROR]"] = allyn_update_download_error();
		setStatusFromTemplate("status_failed", extra);
		allyn_update_log("UI received download failure: " + allyn_update_download_error());
		return;
	}

	if (state == ALLYN_UPDATE_IDLE)
	{
		mHandledState = state;
		setButtonsEnabled(true);
	}
}

void LLFloaterAllynUpdate::onUpdateNow()
{
	allyn_update_log("user clicked Update Now");
	if (mInstallerUrl.empty() && !allyn_update_is_simulate())
	{
		LLStringUtil::format_map_t extra;
		extra["[ERROR]"] = std::string("Missing installer URL");
		setStatusFromTemplate("status_failed", extra);
		allyn_update_log("FAIL missing installer url");
		return;
	}
	mHandledState = ALLYN_UPDATE_IDLE;
	mLastLoggedPercent = -1;
	setButtonsEnabled(false);
	allyn_update_start_download(mInstallerUrl);
}

void LLFloaterAllynUpdate::onUpdateLater()
{
	allyn_update_log("user clicked Update Later");
	allyn_update_cancel_download();
	close();
}

void LLFloaterAllynUpdate::onCloseBtn()
{
	allyn_update_log("user clicked Close");
	allyn_update_cancel_download();
	close();
}

void LLFloaterAllynUpdate::onClose(bool app_quitting)
{
	if (!app_quitting)
	{
		allyn_update_cancel_download();
	}
	destroy();
}
