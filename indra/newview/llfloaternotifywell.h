/**
 * @file llfloaternotifywell.h
 * @brief Overflow list for stacked notify boxes (Allyn chrome, Firestorm behavior).
 */
#ifndef LL_LLFLOATERNOTIFYWELL_H
#define LL_LLFLOATERNOTIFYWELL_H
#include "llfloater.h"
#include "lluuid.h"
#include <string>
#include <utility>
#include <vector>
class LLScrollListCtrl;
class LLFloaterNotifyWell : public LLFloater, public LLFloaterSingleton<LLFloaterNotifyWell>
{
	friend class LLUISingleton<LLFloaterNotifyWell, VisibilityPolicy<LLFloater> >;
public:
	BOOL postBuild() override;
	void refreshItems(const std::vector<std::pair<LLUUID, std::string> >& items);
private:
	LLFloaterNotifyWell(const LLSD& seed);
	void onOpenSelected();
	void onCloseAll();
	void onListMouseUp(S32 x, S32 y);
	void onListDoubleClick();
	bool wasRecentCloseClick() const;
	LLScrollListCtrl* mList;
	F64 mLastCloseTime;
};
#endif
