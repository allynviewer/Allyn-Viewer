/**
 * @file llfloatertoolbarprefs.h
 * @brief Floater for choosing which toolbar buttons are visible.
 */
#ifndef LL_LLFLOATERTOOLBARPREFS_H
#define LL_LLFLOATERTOOLBARPREFS_H
#include "llfloater.h"
#include <vector>
class LLCheckBoxCtrl;
class LLFloaterToolbarPrefs : public LLFloater, public LLFloaterSingleton<LLFloaterToolbarPrefs>
{
public:
	LLFloaterToolbarPrefs(const LLSD& seed);
	BOOL postBuild();
private:
	void onFilter(const LLSD& value);
	void applyFilter(const std::string& filter);
	struct CheckboxInfo
	{
		LLCheckBoxCtrl* ctrl;
		LLRect original_rect;
	};
	std::vector<CheckboxInfo> mCheckboxes;
};
#endif
