/**
 * @file llfloaternotranslate.h
 * @brief Floater listing residents excluded from chat/IM translation
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */
#ifndef LL_LLFLOATERNOTRANSLATE_H
#define LL_LLFLOATERNOTRANSLATE_H
#include "llfloater.h"
class LLNameListCtrl;
class LLFloaterNoTranslate : public LLFloater, public LLFloaterSingleton<LLFloaterNoTranslate>
{
	friend class LLUISingleton<LLFloaterNoTranslate, VisibilityPolicy<LLFloater> >;
public:
	BOOL postBuild() override;
	void onOpen() override;
	void onClose(bool app_quitting) override;
	void refreshList();
	static void refreshIfOpen();
private:
	LLFloaterNoTranslate(const LLSD& seed);
	~LLFloaterNoTranslate();
	void updateButtons();
	void onReactivate();
	LLNameListCtrl* mList;
};
#endif
