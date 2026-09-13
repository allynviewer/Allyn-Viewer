/**
 * @file llnotifyiconbar.h
 * @brief Compact notification icon below the navigation bar.
 */
#ifndef LL_LLNOTIFYICONBAR_H
#define LL_LLNOTIFYICONBAR_H
#include "llpanel.h"
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
	S32 getOccupiedHeight() const;
private:
	void refreshBadges();
	void onWellClicked();
	LLButton* mWellBtn;
	bool mMouselookHidden;
	S32 mLastNotifyCount;
};
extern LLNotifyIconBar* gNotifyIconBar;
#endif
