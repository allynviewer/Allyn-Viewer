/** 
 * @file llfloaterperms.cpp
 * @brief Asset creation permission preferences.
 * @author Jonathan Yap
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * Modifications Copyright (C) 2026 - Allyn Viewer
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
#include "llviewerprecompiledheaders.h"
#include "lfsimfeaturehandler.h"
#include "llagent.h"
#include "llcheckboxctrl.h"
#include "llfloaterperms.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llpermissions.h"
#include "llsdserialize.h"
#include "hippogridmanager.h"
#include "llcallbacklist.h"
extern class AIHTTPTimeoutPolicy floaterPermsResponder_timeout;
U32 LLFloaterPerms::getGroupPerms(std::string prefix)
{
	return gSavedSettings.getBOOL(prefix+"ShareWithGroup") ? PERM_COPY | PERM_MOVE | PERM_MODIFY : PERM_NONE;
}
U32 LLFloaterPerms::getEveryonePerms(std::string prefix)
{
	U32 flags = PERM_NONE;
	if (prefix != "Bulk" && LFSimFeatureHandler::instance().simSupportsExport() && prefix.empty() && gSavedPerAccountSettings.getBOOL(prefix+"EveryoneExport"))
		flags |= PERM_EXPORT;
	if (gSavedSettings.getBOOL(prefix+"EveryoneCopy"))
		flags |= PERM_COPY;
	return flags;
}
U32 LLFloaterPerms::getNextOwnerPerms(std::string prefix)
{
	U32 flags = PERM_MOVE;
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerCopy") )
	{
		flags |= PERM_COPY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerModify") )
	{
		flags |= PERM_MODIFY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerTransfer") )
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}
U32 LLFloaterPerms::getNextOwnerPermsInverted(std::string prefix)
{
	U32 flags = PERM_MOVE;
	if (!gSavedSettings.getBOOL(prefix+"NextOwnerCopy"))
	{
		flags |= PERM_COPY;
	}
	if (!gSavedSettings.getBOOL(prefix+"NextOwnerModify"))
	{
		flags |= PERM_MODIFY;
	}
	if (!gSavedSettings.getBOOL(prefix+"NextOwnerTransfer"))
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}
namespace
{
	void handle_checkboxes(LLView* view, const std::string& ctrl_name, const LLSD& value, const std::string& type)
	{
		if (ctrl_name == type+"everyone_export")
		{
			view->getChildView(type+"next_owner_copy")->setEnabled(!value);
			view->getChildView(type+"next_owner_modify")->setEnabled(!value);
			view->getChildView(type+"next_owner_transfer")->setEnabled(!value);
		}
		else
		{
			if (ctrl_name == type+"next_owner_copy")
			{
				if (!value)
					gSavedSettings.setBOOL(type+"NextOwnerTransfer", true);
				view->getChildView(type+"next_owner_transfer")->setEnabled(value);
			}
			if (!value)
				view->getChildView(type+"everyone_export")->setEnabled(false);
			else
				view->getChildView(type+"everyone_export")->setEnabled(LFSimFeatureHandler::instance().simSupportsExport() && (LLFloaterPerms::getNextOwnerPerms(type) & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED);
		}
	}
}
static bool mCapSent = false;
static bool mInitialPermsSend = false;
static U32 sPermsRequestGeneration = 0;
static const S32 PERMS_MAX_ATTEMPTS = 5;
static const F32 PERMS_RETRY_SECONDS = 5.f;
LLFloaterPermsDefault::LLFloaterPermsDefault(const LLSD& seed)
	: LLFloater()
{
	mCommitCallbackRegistrar.add("PermsDefault.OK", boost::bind(&LLFloaterPermsDefault::onClickOK, this));
	mCommitCallbackRegistrar.add("PermsDefault.Cancel", boost::bind(&LLFloaterPermsDefault::onClickCancel, this));
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_perm_prefs.xml");
}
const std::string LLFloaterPermsDefault::sCategoryNames[CAT_LAST] =
{
	"Objects",
	"Uploads",
	"Scripts",
	"Notecards",
	"Gestures",
	"Wearables"
};
void LLFloaterPermsDefault::initCheckboxes(bool export_support, const std::string& type)
{
	const U32 next_owner_perms = LLFloaterPerms::getNextOwnerPerms(type);
	getChildView(type + "everyone_export")->setEnabled(export_support && (next_owner_perms & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED);
	if (!(next_owner_perms & PERM_COPY))
	{
		getChildView(type + "next_owner_transfer")->setEnabled(false);
	}
	else if (export_support)
	{
		bool export_off = !gSavedPerAccountSettings.getBOOL(type+"EveryoneExport");
		getChildView(type + "next_owner_copy")->setEnabled(export_off);
		getChildView(type + "next_owner_modify")->setEnabled(export_off);
		getChildView(type + "next_owner_transfer")->setEnabled(export_off);
	}
	else
		gSavedPerAccountSettings.setBOOL(type+"EveryoneExport", false);
}
BOOL LLFloaterPermsDefault::postBuild()
{
	bool export_support = LFSimFeatureHandler::instance().simSupportsExport();
	bool is_sl = gHippoGridManager->getCurrentGrid()->isSecondLife();
	for (S32 i = 0; i < CAT_LAST; ++i)
	{
		const std::string& type(sCategoryNames[i]);
		initCheckboxes(export_support, type);
		commit_callback_t handle_checks(boost::bind(handle_checkboxes, this, boost::bind(&LLView::getName, _1), _2, type));
		getChild<LLUICtrl>(type + "next_owner_copy")->setCommitCallback(handle_checks);
		getChild<LLUICtrl>(type + "next_owner_modify")->setCommitCallback(handle_checks);
		getChild<LLUICtrl>(type + "next_owner_transfer")->setCommitCallback(handle_checks);
		if (is_sl)
			getChildView(type + "everyone_export")->setVisible(false);
		else
			getChild<LLUICtrl>(type + "everyone_export")->setCommitCallback(handle_checks);
	}
	if (is_sl)
	{
		LLView* view(getChildView("ExportationLabel"));
		S32 shift(view->getRect().getWidth());
		LLRect rect(getRect());
		rect.mRight -= shift;
		setRect(rect);
		view->setVisible(false);
		shift /= -2;
		view = getChildView("ok");
		rect = view->getRect();
		rect.translate(shift, 0);
		view->setRect(rect);
		view = getChildView("cancel");
		rect = view->getRect();
		rect.translate(shift, 0);
		view->setRect(rect);
	}
	refresh();
	return TRUE;
}
void LLFloaterPermsDefault::onClickOK()
{
	ok();
	close();
}
void LLFloaterPermsDefault::onClickCancel()
{
	cancel();
	close();
}
class LLFloaterPermsResponder final : public LLHTTPClient::ResponderWithResult
{
public:
	static void send(std::string url, LLSD report, S32 attempt, U32 generation)
	{
		if (generation != sPermsRequestGeneration || LLApp::isExiting())
			return;
		std::string current = gAgent.getRegionCapability("AgentPreferences");
		if (!current.empty())
			url = current;
		if (url.empty())
		{
			if (attempt < PERMS_MAX_ATTEMPTS)
			{
				doAfterInterval([report, attempt, generation]()
				{
					LLFloaterPermsResponder::send(std::string(), report, attempt + 1, generation);
				}, PERMS_RETRY_SECONDS);
				return;
			}
			mInitialPermsSend = false;
			LL_WARNS("ObjectPermissionsFloater") << "Unable to send default permissions. Giving up for now." << LL_ENDL;
			return;
		}
		LLHTTPClient::post(url, report, new LLFloaterPermsResponder(url, report, attempt, generation));
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy() const override { return floaterPermsResponder_timeout; }
	char const* getName() const override { return "LLFloaterPermsResponder"; }
private:
	static std::string sPreviousReason;
	LLFloaterPermsResponder(const std::string& url, const LLSD& report, S32 attempt, U32 generation)
	:	mUrl(url),
		mReport(report),
		mAttempt(attempt),
		mGeneration(generation)
	{
	}
	void httpFailure() override
	{
		if (mGeneration != sPermsRequestGeneration || LLApp::isExiting())
			return;
		const std::string& reason = getReason();
		LL_INFOS("ObjectPermissionsFloater") << getURL() << " [" << getStatus() << "]: " << reason << LL_ENDL;
		if (mCapSent && reason != sPreviousReason)
		{
			sPreviousReason = reason;
			LLSD args;
			args["REASON"] = reason;
			LLNotificationsUtil::add("DefaultObjectPermissions", args);
		}
		if (mAttempt < PERMS_MAX_ATTEMPTS)
		{
			doAfterInterval([url = mUrl, report = mReport, attempt = mAttempt, generation = mGeneration]()
			{
				LLFloaterPermsResponder::send(url, report, attempt + 1, generation);
			}, PERMS_RETRY_SECONDS);
			return;
		}
		mInitialPermsSend = false;
		LL_WARNS("ObjectPermissionsFloater") << "Unable to send default permissions. Giving up for now." << LL_ENDL;
	}
	void httpSuccess() override
	{
		if (mGeneration != sPermsRequestGeneration)
			return;
		sPreviousReason.clear();
		mCapSent = true;
		mInitialPermsSend = false;
		LL_INFOS("ObjectPermissionsFloater") << "Default permissions successfully sent to simulator" << LL_ENDL;
	}
	std::string mUrl;
	LLSD mReport;
	S32 mAttempt;
	U32 mGeneration;
};
std::string LLFloaterPermsResponder::sPreviousReason;
void LLFloaterPermsDefault::sendInitialPerms()
{
	if (!mCapSent && !mInitialPermsSend)
		updateCap();
}
void LLFloaterPermsDefault::updateCap()
{
	if (!gAgent.getRegion())
	{
		LL_WARNS("Avatar") << "Region not set, cannot request capability update" << LL_ENDL;
		return;
	}
	std::string object_url = gAgent.getRegionCapability("AgentPreferences");
	if (object_url.empty())
	{
		LL_DEBUGS("ObjectPermissionsFloater") << "AgentPreferences cap not available." << LL_ENDL;
		return;
	}
	LLSD report = LLSD::emptyMap();
	report["default_object_perm_masks"]["Group"] =
		(LLSD::Integer)LLFloaterPerms::getGroupPerms(sCategoryNames[CAT_OBJECTS]);
	report["default_object_perm_masks"]["Everyone"] =
		(LLSD::Integer)LLFloaterPerms::getEveryonePerms(sCategoryNames[CAT_OBJECTS]);
	report["default_object_perm_masks"]["NextOwner"] =
		(LLSD::Integer)LLFloaterPerms::getNextOwnerPerms(sCategoryNames[CAT_OBJECTS]);
	{
		std::ostringstream sent_perms_log;
		LLSDSerialize::toPrettyXML(report, sent_perms_log);
		LL_DEBUGS("ObjectPermissionsFloater") << "Sending default permissions to '"
											  << object_url << "'\n"
											  << sent_perms_log.str() << LL_ENDL;
	}
	mInitialPermsSend = true;
	LLFloaterPermsResponder::send(object_url, report, 1, ++sPermsRequestGeneration);
}
void LLFloaterPermsDefault::ok()
{
	refresh();
	updateCap();
}
void LLFloaterPermsDefault::cancel()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerCopy",     mNextOwnerCopy[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerModify",   mNextOwnerModify[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"NextOwnerTransfer", mNextOwnerTransfer[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"ShareWithGroup",    mShareWithGroup[iter]);
		gSavedSettings.setBOOL(sCategoryNames[iter]+"EveryoneCopy",      mEveryoneCopy[iter]);
		gSavedPerAccountSettings.setBOOL(sCategoryNames[iter]+"EveryoneExport", mEveryoneExport[iter]);
	}
}
void LLFloaterPermsDefault::refresh()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		mShareWithGroup[iter]    = gSavedSettings.getBOOL(sCategoryNames[iter]+"ShareWithGroup");
		mEveryoneCopy[iter]      = gSavedSettings.getBOOL(sCategoryNames[iter]+"EveryoneCopy");
		mNextOwnerCopy[iter]     = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerCopy");
		mNextOwnerModify[iter]   = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerModify");
		mNextOwnerTransfer[iter] = gSavedSettings.getBOOL(sCategoryNames[iter]+"NextOwnerTransfer");
		mEveryoneExport[iter]    = gSavedPerAccountSettings.getBOOL(sCategoryNames[iter]+"EveryoneExport");
	}
}
void LLFloaterPermsDefault::onClose(bool app_quitting)
{
	cancel();
	LLFloater::onClose(app_quitting);
}
