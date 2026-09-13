/** 
* @file llfloatertos.cpp
* @brief Terms of Service Agreement dialog
*
* $LicenseInfo:firstyear=2003&license=viewergpl$
* 
* Copyright (c) 2003-2009, Linden Research, Inc.; 2010, McCabe Maxsted; 2019, Liru Faers
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
* online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
* 
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
* 
* ALL SOURCE CODE IS PROVIDED "AS IS." THE AUTHOR MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/
#include "llviewerprecompiledheaders.h"
#include "llfloatertos.h"
#include "llagent.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llhttpclient.h"
#include "llhttpstatuscodes.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llvfile.h"
#include "message.h"
class AIHTTPTimeoutPolicy;
extern AIHTTPTimeoutPolicy iamHere_timeout;
LLFloaterTOS* LLFloaterTOS::sInstance = nullptr;
LLFloaterTOS* LLFloaterTOS::show(ETOSType type, const std::string & message)
{
	if (sInstance) sInstance->close();
	return sInstance = new LLFloaterTOS(type, message);
}
LLFloaterTOS::LLFloaterTOS(ETOSType type, const std::string& message)
:	LLModalDialog(std::string(" "), 100, 100),
	mType(type),
	mLoadCompleteCount(0)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
		mType == TOS_CRITICAL_MESSAGE ? "floater_critical.xml"
			: mType == TOS_TOS ? "floater_tos.xml"
			: "floater_voice_license.xml");
	if (mType == TOS_CRITICAL_MESSAGE)
	{
		LLTextEditor *editor = getChild<LLTextEditor>("tos_text");
		editor->setHandleEditKeysDirectly(TRUE);
		editor->setEnabled(FALSE);
		editor->setWordWrap(TRUE);
		editor->setFocus(TRUE);
		editor->setValue(message);
	}
}
class LLIamHere : public LLHTTPClient::ResponderWithResult
{
	LLIamHere(LLFloaterTOS* parent) : mParent(parent->getDerivedHandle<LLFloaterTOS>()) {}
	LLHandle<LLFloaterTOS> mParent;
public:
	static boost::intrusive_ptr<LLIamHere> build(LLFloaterTOS* parent)
	{
		return boost::intrusive_ptr<LLIamHere>(new LLIamHere(parent));
	}
	void httpSuccess() override
	{
		if (!mParent.isDead())
			mParent.get()->setSiteIsAlive(true);
	}
	void httpFailure() override
	{
		if (!mParent.isDead())
		{
			mParent.get()->setSiteIsAlive(mStatus == HTTP_FOUND);
		}
	}
	AIHTTPTimeoutPolicy const& getHTTPTimeoutPolicy() const override { return iamHere_timeout; }
	bool pass_redirect_status() const override { return true; }
	char const* getName() const override { return "LLIamHere"; }
};
BOOL LLFloaterTOS::postBuild()
{
	childSetAction("Continue", onContinue, this);
	childSetAction("Cancel", onCancel, this);
	childSetCommitCallback("agree_chk", updateAgree, this);
	if (mType == TOS_CRITICAL_MESSAGE) return TRUE;
	LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
	tos_agreement->setEnabled(false);
	bool voice = mType == TOS_VOICE;
	getChild<LLTextEditor>(voice ? "license_text" : "tos_text")->setVisible(FALSE);
	if (LLMediaCtrl* web_browser = getChild<LLMediaCtrl>(voice ? "license_html" : "tos_html"))
	{
		web_browser->addObserver(this);
		std::string url = getString( "real_url" );
		if (!voice || url.substr(0,4) == "http") {
			LLHTTPClient::get(url, LLIamHere::build(this));
		} else {
			setSiteIsAlive(false);
		}
	}
	return TRUE;
}
void LLFloaterTOS::setSiteIsAlive(bool alive)
{
	if (mType != TOS_CRITICAL_MESSAGE)
	{
		bool voice = mType == TOS_VOICE;
		LLMediaCtrl* web_browser = getChild<LLMediaCtrl>(voice ? "license_html" : "tos_html");
		if (alive)
		{
			if (web_browser)
			{
				web_browser->navigateTo(getString("real_url"));
			}
		}
		else
		{
			if (voice) web_browser->navigateToLocalPage("license", getString("fallback_html"));
			getChild<LLCheckBoxCtrl>("agree_chk")->setEnabled(true);
		}
	}
}
LLFloaterTOS::~LLFloaterTOS()
{
	sInstance = nullptr;
}
void LLFloaterTOS::updateAgree(LLUICtrl*, void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	bool agree = self->childGetValue("agree_chk").asBoolean();
	self->childSetEnabled("Continue", agree);
}
void LLFloaterTOS::onContinue(void* userdata)
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	bool voice = self->mType == TOS_VOICE;
	LL_INFOS() << (voice ? "User agreed to the Vivox personal license" : "User agrees with TOS.") << LL_ENDL;
	if (voice)
	{
		gSavedSettings.setBOOL("EnableVoiceChat", TRUE);
		gSavedSettings.setBOOL("VivoxLicenseAccepted", TRUE);
		gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);
	}
	else if (self->mType == TOS_TOS)
	{
		gAcceptTOS = TRUE;
	}
	else
	{
		gAcceptCriticalMessage = TRUE;
	}
	auto state = LLStartUp::getStartupState();
#if !LL_RELEASE_FOR_DOWNLOAD
	if (!voice && state == STATE_LOGIN_WAIT)
	{
		LLStartUp::setStartupState(STATE_LOGIN_SHOW);
	}
	else
#endif
	if (!voice || state == STATE_LOGIN_VOICE_LICENSE)
	{
		LLStartUp::setStartupState(STATE_LOGIN_AUTH_INIT);
	}
	self->close();
}
void LLFloaterTOS::onCancel(void* userdata)
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	if (self->mType == TOS_VOICE)
	{
		LL_INFOS() << "User disagreed with the Vivox personal license" << LL_ENDL;
		gSavedSettings.setBOOL("EnableVoiceChat", FALSE);
		gSavedSettings.setBOOL("VivoxLicenseAccepted", FALSE);
		if (LLStartUp::getStartupState() == STATE_LOGIN_VOICE_LICENSE)
		{
			LLStartUp::setStartupState(STATE_LOGIN_AUTH_INIT);
		}
	}
	else
	{
		LL_INFOS() << "User disagrees with TOS." << LL_ENDL;
		LLNotificationsUtil::add("MustAgreeToLogIn", LLSD(), LLSD(), login_alert_done);
		LLStartUp::setStartupState(STATE_LOGIN_SHOW);
	}
	self->close();
}
void LLFloaterTOS::handleMediaEvent(LLPluginClassMedia* , EMediaEvent event)
{
	if (event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		if (++mLoadCompleteCount == 2)
		{
			LL_INFOS() << "NAVIGATE COMPLETE" << LL_ENDL;
			getChild<LLCheckBoxCtrl>("agree_chk")->setEnabled(true);
		}
	}
}
