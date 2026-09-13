/**
 * @file llprefstranslation.cpp
 * @brief Preferences panel for AI chat translation
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llprefstranslation.h"
#include "llagent.h"
#include "llbutton.h"
#include "llchataitranslate.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmodaldialog.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltrans.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
namespace
{
class LLFloaterAITranslateNotice : public LLModalDialog
{
public:
	static void show(LLPrefsTranslation* parent);
	static void closeIfOpen();
	LLFloaterAITranslateNotice(LLPrefsTranslation* parent);
	~LLFloaterAITranslateNotice();
	BOOL handleKeyHere(KEY key, MASK mask) override;
	void draw() override;
	void onClose(bool app_quitting) override;
private:
	void onAccept();
	void onCancel();
	void updateAcceptEnabled();
	static void onScrollEnd(void* userdata);
	static LLFloaterAITranslateNotice* sInstance;
	LLHandle<LLPanel> mParentHandle;
	LLButton* mAcceptBtn;
	bool mAccepted;
	bool mReachedEnd;
	S32 mDrawCount;
};
LLFloaterAITranslateNotice* LLFloaterAITranslateNotice::sInstance = NULL;
void LLFloaterAITranslateNotice::show(LLPrefsTranslation* parent)
{
	if (!parent)
		return;
	if (sInstance)
	{
		sInstance->setFocus(TRUE);
		return;
	}
	sInstance = new LLFloaterAITranslateNotice(parent);
	sInstance->startModal();
}
void LLFloaterAITranslateNotice::closeIfOpen()
{
	if (sInstance)
		sInstance->close();
}
LLFloaterAITranslateNotice::LLFloaterAITranslateNotice(LLPrefsTranslation* parent)
	: LLModalDialog(LLStringUtil::null, 640, 560)
	, mParentHandle(parent->getHandle())
	, mAcceptBtn(NULL)
	, mAccepted(false)
	, mReachedEnd(false)
	, mDrawCount(0)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_ai_translate_notice.xml");
	setCanClose(TRUE);
	setCanMinimize(FALSE);
	setCanResize(TRUE);
	mAcceptBtn = findChild<LLButton>("accept_btn");
	if (mAcceptBtn)
	{
		mAcceptBtn->setEnabled(FALSE);
		mAcceptBtn->setCommitCallback(boost::bind(&LLFloaterAITranslateNotice::onAccept, this));
	}
	if (LLTextEditor* editor = findChild<LLTextEditor>("notice_text"))
	{
		static_cast<LLView*>(editor)->setEnabled(TRUE);
		editor->setEnabled(FALSE);
		editor->setHandleEditKeysDirectly(TRUE);
		editor->setWordWrap(TRUE);
		if (hasString("notice_body"))
			editor->setValue(getString("notice_body"));
		editor->setOnScrollEndCallback(&LLFloaterAITranslateNotice::onScrollEnd, this);
	}
	else
	{
		LL_WARNS("AITranslate") << "floater_ai_translate_notice.xml: notice_text not found" << LL_ENDL;
	}
	if (LLUICtrl* cancel = findChild<LLUICtrl>("cancel_btn"))
		cancel->setCommitCallback(boost::bind(&LLFloaterAITranslateNotice::onCancel, this));
}
LLFloaterAITranslateNotice::~LLFloaterAITranslateNotice()
{
	if (sInstance == this)
		sInstance = NULL;
}
BOOL LLFloaterAITranslateNotice::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		onCancel();
		return TRUE;
	}
	return LLModalDialog::handleKeyHere(key, mask);
}
void LLFloaterAITranslateNotice::draw()
{
	LLModalDialog::draw();
	updateAcceptEnabled();
}
void LLFloaterAITranslateNotice::onScrollEnd(void* userdata)
{
	if (LLFloaterAITranslateNotice* self = static_cast<LLFloaterAITranslateNotice*>(userdata))
		self->updateAcceptEnabled();
}
void LLFloaterAITranslateNotice::updateAcceptEnabled()
{
	if (mReachedEnd)
		return;
	++mDrawCount;
	if (mDrawCount < 2)
		return;
	LLTextEditor* editor = findChild<LLTextEditor>("notice_text");
	if (!editor)
		return;
	if (editor->getText().size() < 200)
		return;
	if (!editor->isScrolledToBottom())
		return;
	mReachedEnd = true;
	if (mAcceptBtn)
		mAcceptBtn->setEnabled(TRUE);
}
void LLFloaterAITranslateNotice::onAccept()
{
	if (!mReachedEnd)
		return;
	mAccepted = true;
	if (LLPrefsTranslation* parent = dynamic_cast<LLPrefsTranslation*>(mParentHandle.get()))
		parent->onNoticeAccepted();
	close();
}
void LLFloaterAITranslateNotice::onCancel()
{
	close();
}
void LLFloaterAITranslateNotice::onClose(bool app_quitting)
{
	if (!mAccepted)
	{
		if (LLPrefsTranslation* parent = dynamic_cast<LLPrefsTranslation*>(mParentHandle.get()))
			parent->onNoticeDeclined();
	}
	if (sInstance == this)
		sInstance = NULL;
	LLModalDialog::onClose(app_quitting);
}
}
LLPrefsTranslation::LLPrefsTranslation()
	: mStatusText(NULL)
	, mGroupList(NULL)
	, mIgnoreEnableCommit(false)
	, mIgnoreProviderCommit(false)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_translation.xml");
	mStatusText = getChild<LLTextBox>("status_text");
	mGroupList = findChild<LLScrollListCtrl>("group_list");
	getChild<LLUICtrl>("provider_combo")->setCommitCallback(boost::bind(&LLPrefsTranslation::onProviderCommit, this));
	getChild<LLUICtrl>("test_btn")->setCommitCallback(boost::bind(&LLPrefsTranslation::onTestConnection, this));
	if (LLUICtrl* btn = findChild<LLUICtrl>("group_refresh_btn"))
		btn->setCommitCallback(boost::bind(&LLPrefsTranslation::refreshGroupList, this));
	getChild<LLUICtrl>("api_key")->setCommitCallback(boost::bind(&LLPrefsTranslation::markStatusStale, this));
	getChild<LLUICtrl>("base_url")->setCommitCallback(boost::bind(&LLPrefsTranslation::markStatusStale, this));
	getChild<LLUICtrl>("model")->setCommitCallback(boost::bind(&LLPrefsTranslation::markStatusStale, this));
	if (LLUICtrl* enable = findChild<LLUICtrl>("enable_check"))
		enable->setCommitCallback(boost::bind(&LLPrefsTranslation::onEnableCheck, this));
	else
		LL_WARNS("AITranslate") << "enable_check not found in translation prefs" << LL_ENDL;
	refreshValues();
	refresh();
	refreshGroupList();
	setStatus(0, LLTrans::getString("AITranslateTestNotTested"));
}
LLPrefsTranslation::~LLPrefsTranslation()
{
	LLFloaterAITranslateNotice::closeIfOpen();
}
void LLPrefsTranslation::refreshValues()
{
	mEnabled = gSavedSettings.getBOOL("AITranslateEnabled");
	mScope = gSavedSettings.getString("AITranslateScope");
	mSourceLang = gSavedSettings.getString("AITranslateSourceLang");
	mTargetLang = gSavedSettings.getString("AITranslateTargetLang");
	mIncoming = gSavedSettings.getBOOL("AITranslateIncoming");
	mOutgoing = gSavedSettings.getBOOL("AITranslateOutgoing");
	mIncomingDisplay = gSavedSettings.getString("AITranslateIncomingDisplay");
	mOutgoingDisplay = gSavedSettings.getString("AITranslateOutgoingDisplay");
	mIncomingColor = gSavedSettings.getColor4("AITranslateIncomingColor");
	mOutgoingColor = gSavedSettings.getColor4("AITranslateOutgoingColor");
	mProvider = gSavedSettings.getString("AITranslateProvider");
	mApiKey = gSavedSettings.getString("AITranslateApiKey");
	mBaseUrl = gSavedSettings.getString("AITranslateBaseUrl");
	mModel = gSavedSettings.getString("AITranslateModel");
	mApiStyle = gSavedSettings.getString("AITranslateApiStyle");
	mGroupsEnabled = gSavedSettings.getBOOL("AITranslateGroupsEnabled");
	mGroupMode = gSavedSettings.getString("AITranslateGroupMode");
	mGroupIDs = gSavedSettings.getString("AITranslateGroupIDs");
}
void LLPrefsTranslation::refresh()
{
	pushSettingsToWidgets();
}
void LLPrefsTranslation::setEnableCheck(bool enabled)
{
	LLCheckBoxCtrl* check = findChild<LLCheckBoxCtrl>("enable_check");
	if (!check)
		return;
	mIgnoreEnableCommit = true;
	check->setValue(enabled);
	mIgnoreEnableCommit = false;
}
void LLPrefsTranslation::pushSettingsToWidgets()
{
	setEnableCheck(gSavedSettings.getBOOL("AITranslateEnabled"));
	mIgnoreProviderCommit = true;
	if (LLUICtrl* c = findChild<LLUICtrl>("scope_combo"))
		c->setValue(gSavedSettings.getString("AITranslateScope"));
	if (LLUICtrl* c = findChild<LLUICtrl>("incoming_check"))
		c->setValue(gSavedSettings.getBOOL("AITranslateIncoming"));
	if (LLUICtrl* c = findChild<LLUICtrl>("outgoing_check"))
		c->setValue(gSavedSettings.getBOOL("AITranslateOutgoing"));
	if (LLUICtrl* c = findChild<LLUICtrl>("source_lang"))
		c->setValue(gSavedSettings.getString("AITranslateSourceLang"));
	if (LLUICtrl* c = findChild<LLUICtrl>("target_lang"))
		c->setValue(gSavedSettings.getString("AITranslateTargetLang"));
	if (LLUICtrl* c = findChild<LLUICtrl>("incoming_display"))
		c->setValue(gSavedSettings.getString("AITranslateIncomingDisplay"));
	if (LLUICtrl* c = findChild<LLUICtrl>("outgoing_display"))
		c->setValue(gSavedSettings.getString("AITranslateOutgoingDisplay"));
	if (LLUICtrl* c = findChild<LLUICtrl>("incoming_color"))
		c->setValue(gSavedSettings.getColor4("AITranslateIncomingColor").getValue());
	if (LLUICtrl* c = findChild<LLUICtrl>("outgoing_color"))
		c->setValue(gSavedSettings.getColor4("AITranslateOutgoingColor").getValue());
	if (LLUICtrl* c = findChild<LLUICtrl>("provider_combo"))
		c->setValue(gSavedSettings.getString("AITranslateProvider"));
	if (LLUICtrl* c = findChild<LLUICtrl>("api_key"))
		c->setValue(gSavedSettings.getString("AITranslateApiKey"));
	if (LLUICtrl* c = findChild<LLUICtrl>("base_url"))
		c->setValue(gSavedSettings.getString("AITranslateBaseUrl"));
	if (LLUICtrl* c = findChild<LLUICtrl>("model"))
		c->setValue(gSavedSettings.getString("AITranslateModel"));
	if (LLUICtrl* c = findChild<LLUICtrl>("groups_enable"))
		c->setValue(gSavedSettings.getBOOL("AITranslateGroupsEnabled"));
	if (LLUICtrl* c = findChild<LLUICtrl>("group_mode"))
		c->setValue(gSavedSettings.getString("AITranslateGroupMode"));
	mIgnoreProviderCommit = false;
}
void LLPrefsTranslation::onEnableCheck()
{
	if (mIgnoreEnableCommit)
		return;
	LLCheckBoxCtrl* check = findChild<LLCheckBoxCtrl>("enable_check");
	if (!check)
		return;
	if (!check->getValue().asBoolean())
	{
		gSavedSettings.setBOOL("AITranslateEnabled", FALSE);
		return;
	}
	setEnableCheck(false);
	LLFloaterAITranslateNotice::show(this);
}
void LLPrefsTranslation::onNoticeAccepted()
{
	gSavedSettings.setBOOL("AITranslateEnabled", TRUE);
	setEnableCheck(true);
}
void LLPrefsTranslation::onNoticeDeclined()
{
	gSavedSettings.setBOOL("AITranslateEnabled", FALSE);
	setEnableCheck(false);
}
void LLPrefsTranslation::refreshGroupList()
{
	if (!mGroupList)
	{
		LL_WARNS("AITranslate") << "group_list scroll_list not found in translation prefs" << LL_ENDL;
		return;
	}
	mGroupList->deleteAllItems();
	const std::vector<LLUUID> selected = LLChatAITranslate::parseGroupIdList(
		gSavedSettings.getString("AITranslateGroupIDs"));
	for (const LLGroupData& g : gAgent.mGroups)
	{
		LLSD row;
		row["id"] = g.mID;
		row["columns"][0]["column"] = "name";
		row["columns"][0]["value"] = g.mName;
		row["columns"][0]["type"] = "text";
		LLScrollListItem* item = mGroupList->addElement(row, ADD_BOTTOM);
		if (!item)
			continue;
		for (const LLUUID& sid : selected)
		{
			if (sid == g.mID)
			{
				item->setSelected(TRUE);
				break;
			}
		}
	}
	mGroupList->updateLayout();
}
void LLPrefsTranslation::syncGroupSelectionToSettings()
{
	if (!mGroupList)
		return;
	std::vector<LLUUID> ids = mGroupList->getSelectedIDs();
	gSavedSettings.setString("AITranslateGroupIDs", LLChatAITranslate::joinGroupIdList(ids));
}
void LLPrefsTranslation::syncAllFieldsToSettings()
{
	if (LLUICtrl* c = findChild<LLUICtrl>("enable_check"))
		gSavedSettings.setBOOL("AITranslateEnabled", c->getValue().asBoolean());
	if (LLUICtrl* c = findChild<LLUICtrl>("scope_combo"))
	{
		std::string scope = c->getValue().asString();
		if (!scope.empty())
			gSavedSettings.setString("AITranslateScope", scope);
	}
	if (LLUICtrl* c = findChild<LLUICtrl>("incoming_check"))
		gSavedSettings.setBOOL("AITranslateIncoming", c->getValue().asBoolean());
	if (LLUICtrl* c = findChild<LLUICtrl>("outgoing_check"))
		gSavedSettings.setBOOL("AITranslateOutgoing", c->getValue().asBoolean());
	if (LLUICtrl* c = findChild<LLUICtrl>("source_lang"))
	{
		std::string lang = c->getValue().asString();
		LLStringUtil::trim(lang);
		if (!lang.empty())
			gSavedSettings.setString("AITranslateSourceLang", lang);
	}
	if (LLUICtrl* c = findChild<LLUICtrl>("target_lang"))
	{
		std::string lang = c->getValue().asString();
		LLStringUtil::trim(lang);
		if (!lang.empty())
			gSavedSettings.setString("AITranslateTargetLang", lang);
	}
	if (LLUICtrl* c = findChild<LLUICtrl>("incoming_display"))
	{
		std::string display = c->getValue().asString();
		if (!display.empty())
			gSavedSettings.setString("AITranslateIncomingDisplay", display);
	}
	if (LLUICtrl* c = findChild<LLUICtrl>("outgoing_display"))
	{
		std::string display = c->getValue().asString();
		if (!display.empty())
			gSavedSettings.setString("AITranslateOutgoingDisplay", display);
	}
	if (LLUICtrl* c = findChild<LLUICtrl>("incoming_color"))
		gSavedSettings.setColor4("AITranslateIncomingColor", LLColor4(c->getValue()));
	if (LLUICtrl* c = findChild<LLUICtrl>("outgoing_color"))
		gSavedSettings.setColor4("AITranslateOutgoingColor", LLColor4(c->getValue()));
	if (LLUICtrl* c = findChild<LLUICtrl>("groups_enable"))
		gSavedSettings.setBOOL("AITranslateGroupsEnabled", c->getValue().asBoolean());
	if (LLUICtrl* c = findChild<LLUICtrl>("group_mode"))
	{
		std::string mode = c->getValue().asString();
		if (!mode.empty())
			gSavedSettings.setString("AITranslateGroupMode", mode);
	}
	syncProviderFieldsToSettings();
	syncGroupSelectionToSettings();
}
void LLPrefsTranslation::apply()
{
	syncAllFieldsToSettings();
	refreshValues();
	LLChatAITranslate::instance().resyncSessionConfigsFromGlobalPrefs();
}
void LLPrefsTranslation::cancel()
{
	gSavedSettings.setBOOL("AITranslateEnabled", mEnabled);
	gSavedSettings.setString("AITranslateScope", mScope);
	gSavedSettings.setString("AITranslateSourceLang", mSourceLang);
	gSavedSettings.setString("AITranslateTargetLang", mTargetLang);
	gSavedSettings.setBOOL("AITranslateIncoming", mIncoming);
	gSavedSettings.setBOOL("AITranslateOutgoing", mOutgoing);
	gSavedSettings.setString("AITranslateIncomingDisplay", mIncomingDisplay);
	gSavedSettings.setString("AITranslateOutgoingDisplay", mOutgoingDisplay);
	gSavedSettings.setColor4("AITranslateIncomingColor", mIncomingColor);
	gSavedSettings.setColor4("AITranslateOutgoingColor", mOutgoingColor);
	gSavedSettings.setString("AITranslateProvider", mProvider);
	gSavedSettings.setString("AITranslateApiKey", mApiKey);
	gSavedSettings.setString("AITranslateBaseUrl", mBaseUrl);
	gSavedSettings.setString("AITranslateModel", mModel);
	gSavedSettings.setString("AITranslateApiStyle", mApiStyle);
	gSavedSettings.setBOOL("AITranslateGroupsEnabled", mGroupsEnabled);
	gSavedSettings.setString("AITranslateGroupMode", mGroupMode);
	gSavedSettings.setString("AITranslateGroupIDs", mGroupIDs);
	refresh();
	refreshGroupList();
}
void LLPrefsTranslation::onProviderCommit()
{
	if (mIgnoreProviderCommit)
		return;
	std::string provider = getChild<LLComboBox>("provider_combo")->getValue().asString();
	LLChatAITranslate::applyProviderPreset(provider);
	getChild<LLUICtrl>("base_url")->setValue(gSavedSettings.getString("AITranslateBaseUrl"));
	getChild<LLUICtrl>("model")->setValue(gSavedSettings.getString("AITranslateModel"));
	markStatusStale();
}
void LLPrefsTranslation::markStatusStale()
{
	setStatus(0, LLTrans::getString("AITranslateTestStale"));
}
void LLPrefsTranslation::setStatus(S32 state, const std::string& message)
{
	if (!mStatusText)
		return;
	std::string safe = message;
	if (safe.size() > 420)
		safe = safe.substr(0, 420) + "...";
	mStatusText->setWrappedText(safe);
	if (state == 1)
		mStatusText->setColor(LLColor4(0.3f, 0.9f, 0.3f, 1.f));
	else if (state == 2)
		mStatusText->setColor(LLColor4(1.f, 0.3f, 0.3f, 1.f));
	else
		mStatusText->setColor(LLColor4(0.7f, 0.7f, 0.7f, 1.f));
}
void LLPrefsTranslation::syncProviderFieldsToSettings()
{
	LLLineEditor* key_ed = findChild<LLLineEditor>("api_key");
	LLLineEditor* url_ed = findChild<LLLineEditor>("base_url");
	LLLineEditor* model_ed = findChild<LLLineEditor>("model");
	LLComboBox* provider_cb = findChild<LLComboBox>("provider_combo");
	std::string key = key_ed ? key_ed->getText() : std::string();
	std::string url = url_ed ? url_ed->getText() : std::string();
	std::string model = model_ed ? model_ed->getText() : std::string();
	LLStringUtil::trim(key);
	LLStringUtil::trim(url);
	LLStringUtil::trim(model);
	gSavedSettings.setString("AITranslateApiKey", key);
	gSavedSettings.setString("AITranslateBaseUrl", url);
	gSavedSettings.setString("AITranslateModel", model);
	if (provider_cb)
	{
		std::string provider = provider_cb->getValue().asString();
		if (!provider.empty())
		{
			gSavedSettings.setString("AITranslateProvider", provider);
			if (provider == "anthropic")
				gSavedSettings.setString("AITranslateApiStyle", "anthropic_messages");
			else
				gSavedSettings.setString("AITranslateApiStyle", "openai_chat");
		}
	}
}
void LLPrefsTranslation::onTestConnection()
{
	syncProviderFieldsToSettings();
	const std::string key = gSavedSettings.getString("AITranslateApiKey");
	if (key.empty())
	{
		setStatus(2, LLTrans::getString("AITranslateTestEmptyKey"));
		return;
	}
	const std::string url = gSavedSettings.getString("AITranslateBaseUrl");
	const std::string model = gSavedSettings.getString("AITranslateModel");
	if (url.empty())
	{
		setStatus(2, LLTrans::getString("AITranslateTestEmptyUrl"));
		return;
	}
	if (model.empty())
	{
		setStatus(2, LLTrans::getString("AITranslateTestEmptyModel"));
		return;
	}
	setStatus(0, LLTrans::getString("AITranslateTestTesting"));
	LLHandle<LLPanel> handle = getHandle();
	LLChatAITranslate::instance().testConnection(
		[handle](bool success, const std::string& message) {
			LLPrefsTranslation* self = dynamic_cast<LLPrefsTranslation*>(handle.get());
			if (self)
				self->onConnectionResult(success, message);
		});
}
void LLPrefsTranslation::onConnectionResult(bool success, const std::string& message)
{
	if (success)
		setStatus(1, message);
	else
		setStatus(2, message.empty() ? LLTrans::getString("AITranslateTestUnknown") : message);
}
