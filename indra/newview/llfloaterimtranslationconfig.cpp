/**
 * @file llfloaterimtranslationconfig.cpp
 * @brief Per-IM / per-group translation language configuration
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"
#include "llfloaterimtranslationconfig.h"
#include "llbutton.h"
#include "llchataitranslate.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llimpanel.h"
#include "lluictrlfactory.h"
LLFloaterIMTranslationConfig* LLFloaterIMTranslationConfig::sInstance = NULL;
namespace
{
const char* kLangCodes[] = {
	"pt", "en", "es", "fr", "de", "it", "ru", "ja", "ko", "zh",
	"ar", "nl", "pl", "tr", "hi", "th", "vi", "uk"
};
const char* kLangLabels[] = {
	"Portuguese (pt)", "English (en)", "Spanish (es)", "French (fr)", "German (de)",
	"Italian (it)", "Russian (ru)", "Japanese (ja)", "Korean (ko)", "Chinese (zh)",
	"Arabic (ar)", "Dutch (nl)", "Polish (pl)", "Turkish (tr)", "Hindi (hi)",
	"Thai (th)", "Vietnamese (vi)", "Ukrainian (uk)"
};
}
LLFloaterIMTranslationConfig::LLFloaterIMTranslationConfig(LLFloaterIMPanel* panel)
: LLFloater(std::string("im_translation_config"))
, mConfigKey(LLUUID::null)
{
	if (panel)
	{
		mConfigKey = LLChatAITranslate::instance().sessionConfigKey(panel);
		mPanelHandle = panel->getHandle();
	}
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_im_translation_config.xml");
}
LLFloaterIMTranslationConfig::~LLFloaterIMTranslationConfig()
{
	if (sInstance == this)
		sInstance = NULL;
}
void LLFloaterIMTranslationConfig::show(LLFloaterIMPanel* panel)
{
	if (!panel)
		return;
	if (sInstance)
	{
		LLFloaterIMTranslationConfig* old = sInstance;
		sInstance = NULL;
		old->close();
	}
	sInstance = new LLFloaterIMTranslationConfig(panel);
	sInstance->center();
	sInstance->open();
}
BOOL LLFloaterIMTranslationConfig::postBuild()
{
	bool enabled = false;
	bool incoming = true;
	bool outgoing = true;
	std::string source, target;
	if (LLFloaterIMPanel* panel = dynamic_cast<LLFloaterIMPanel*>(mPanelHandle.get()))
	{
		LLChatAITranslate::instance().ensureSessionConfig(panel, enabled, source, target, incoming, outgoing);
	}
	else if (!LLChatAITranslate::instance().getSessionTranslation(mConfigKey, enabled, source, target, incoming, outgoing))
	{
		source = LLChatAITranslate::instance().sourceLang();
		target = LLChatAITranslate::instance().targetLang();
		incoming = LLChatAITranslate::instance().translateIncoming();
		outgoing = LLChatAITranslate::instance().translateOutgoing();
	}
	populateLangCombo("source_lang", source);
	populateLangCombo("target_lang", target);
	if (LLCheckBoxCtrl* c = findChild<LLCheckBoxCtrl>("incoming_check"))
		c->set(incoming);
	if (LLCheckBoxCtrl* c = findChild<LLCheckBoxCtrl>("outgoing_check"))
		c->set(outgoing);
	getChild<LLButton>("ok_btn")->setCommitCallback(boost::bind(&LLFloaterIMTranslationConfig::onOK, this));
	getChild<LLButton>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterIMTranslationConfig::onCancel, this));
	return TRUE;
}
void LLFloaterIMTranslationConfig::populateLangCombo(const std::string& combo_name, const std::string& selected)
{
	LLComboBox* combo = getChild<LLComboBox>(combo_name);
	if (!combo)
		return;
	combo->removeall();
	const size_t n = sizeof(kLangCodes) / sizeof(kLangCodes[0]);
	bool found = false;
	for (size_t i = 0; i < n; ++i)
	{
		combo->add(kLangLabels[i], LLSD(kLangCodes[i]));
		if (selected == kLangCodes[i])
			found = true;
	}
	if (!selected.empty() && !found)
		combo->add(selected + " (" + selected + ")", LLSD(selected));
	combo->setSelectedByValue(LLSD(selected), TRUE);
	if (combo->getCurrentIndex() < 0 && !selected.empty())
		combo->setTextEntry(selected);
}
void LLFloaterIMTranslationConfig::onOK()
{
	LLComboBox* src = getChild<LLComboBox>("source_lang");
	LLComboBox* dst = getChild<LLComboBox>("target_lang");
	if (!src || !dst || mConfigKey.isNull())
	{
		close();
		return;
	}
	std::string source = src->getSelectedValue().asString();
	if (source.empty())
		source = src->getSimple();
	std::string target = dst->getSelectedValue().asString();
	if (target.empty())
		target = dst->getSimple();
	bool enabled = true;
	bool incoming = true;
	bool outgoing = true;
	std::string old_source, old_target;
	if (LLFloaterIMPanel* panel = dynamic_cast<LLFloaterIMPanel*>(mPanelHandle.get()))
	{
		LLChatAITranslate::instance().ensureSessionConfig(panel, enabled, old_source, old_target, incoming, outgoing);
		if (LLCheckBoxCtrl* check = panel->findChild<LLCheckBoxCtrl>("im_translate_check"))
			enabled = check->get();
	}
	else if (!LLChatAITranslate::instance().getSessionTranslation(mConfigKey, enabled, old_source, old_target, incoming, outgoing))
	{
		enabled = true;
		incoming = LLChatAITranslate::instance().translateIncoming();
		outgoing = LLChatAITranslate::instance().translateOutgoing();
	}
	if (LLCheckBoxCtrl* c = findChild<LLCheckBoxCtrl>("incoming_check"))
		incoming = c->get();
	if (LLCheckBoxCtrl* c = findChild<LLCheckBoxCtrl>("outgoing_check"))
		outgoing = c->get();
	LLChatAITranslate::instance().setSessionTranslation(mConfigKey, enabled, source, target, incoming, outgoing);
	if (LLFloaterIMPanel* panel = dynamic_cast<LLFloaterIMPanel*>(mPanelHandle.get()))
	{
		if (LLCheckBoxCtrl* check = panel->findChild<LLCheckBoxCtrl>("im_translate_check"))
			check->set(enabled);
	}
	close();
}
void LLFloaterIMTranslationConfig::onCancel()
{
	close();
}
void LLFloaterIMTranslationConfig::onClose(bool app_quitting)
{
	if (sInstance == this)
		sInstance = NULL;
	LLFloater::onClose(app_quitting);
}
