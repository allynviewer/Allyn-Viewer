/**
 * @file llnotifyiconbar.h
 * @brief Compact notification icon below the navigation bar.
 */
#ifndef LL_LLNOTIFYICONBAR_H
#define LL_LLNOTIFYICONBAR_H
#include "llpanel.h"
#include "llfloatingbarstyle.h"
class LLButton;
extern S32 NOTIFY_ICON_BAR_HEIGHT;
extern S32 NOTIFY_ICON_BAR_WIDTH;
class LLNotifyIconBar : public LLPanel
{
public:
	LLNotifyIconBar(const std::string& name, const LLRect& rect);
	virtual ~LLNotifyIconBar();
	void draw() override;
	void setVisibleForMouselook(bool visible);
	void updatePosition();
private:
	void refreshBadges();
	void onWellClicked();
	LLButton* mWellBtn;
	bool mMouselookHidden;
	S32 mLastNotifyCount;
	LLFloatingBarStyle mStyle;
	boost::signals2::connection mMarginSlot;
};
extern LLNotifyIconBar* gNotifyIconBar;
#endif
