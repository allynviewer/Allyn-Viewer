/**
 * @file llfloatertoolbarprefs.h
 * @brief Floater for choosing which toolbar buttons are visible.
 */
#ifndef LL_LLFLOATERTOOLBARPREFS_H
#define LL_LLFLOATERTOOLBARPREFS_H
#include "llfloater.h"
#include <boost/signals2/connection.hpp>
#include <vector>
class LLCheckBoxCtrl;
class LLFilterEditor;
class LLPanel;
class LLScrollContainer;
class LLFloaterToolbarPrefs : public LLFloater, public LLFloaterSingleton<LLFloaterToolbarPrefs>
{
public:
	LLFloaterToolbarPrefs(const LLSD& seed);
	virtual ~LLFloaterToolbarPrefs();
	BOOL postBuild();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
private:
	void onFilter(const LLSD& value);
	void onVisibilityChanged(LLControlVariable* control, const LLSD& value);
	void applyFilter(const std::string& filter);
	void layoutContent();
	S32 checkboxColumnWidth(LLCheckBoxCtrl* ctrl) const;
	S32 checkboxItemHeight(LLCheckBoxCtrl* ctrl) const;
	bool checkboxAllowed(LLCheckBoxCtrl* ctrl) const;
	LLFilterEditor* mFilterEditor;
	LLPanel* mButtonsPanel;
	LLScrollContainer* mScroll;
	std::vector<LLCheckBoxCtrl*> mCheckboxes;
	std::vector<boost::signals2::connection> mVisibilityConnections;
	std::string mFilter;
	bool mInLayout;
};
#endif
