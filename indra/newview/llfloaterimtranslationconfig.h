/**
 * @file llfloaterimtranslationconfig.h
 * @brief Per-IM / per-group translation language configuration
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#ifndef LL_LLFLOATERIMTRANSLATIONCONFIG_H
#define LL_LLFLOATERIMTRANSLATIONCONFIG_H
#include "llfloater.h"
#include "lluuid.h"
class LLFloaterIMPanel;
class LLFloaterIMTranslationConfig : public LLFloater
{
public:
	LLFloaterIMTranslationConfig(LLFloaterIMPanel* panel);
	virtual ~LLFloaterIMTranslationConfig();
	BOOL postBuild();
	void onClose(bool app_quitting);
	static void show(LLFloaterIMPanel* panel);
private:
	void onOK();
	void onCancel();
	void populateLangCombo(const std::string& combo_name, const std::string& selected);
	LLUUID mConfigKey;
	LLHandle<LLFloater> mPanelHandle;
	static LLFloaterIMTranslationConfig* sInstance;
};
#endif
