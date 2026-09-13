/**
 * @file llnavigationbar.h
 * @brief Lightweight Firestorm-style navigation bar (buttons, location, search).
 */
#ifndef LL_LLNAVIGATIONBAR_H
#define LL_LLNAVIGATIONBAR_H
#include "llpanel.h"
#include "llsd.h"
#include "v3dmath.h"
#include <vector>
class LLButton;
class LLComboBox;
class LLLineEditor;
class LLUICtrl;
class LLView;
extern S32 NAV_BAR_HEIGHT;
class LLNavigationBar : public LLPanel
{
public:
	LLNavigationBar(const std::string& name, const LLRect& rect);
	virtual ~LLNavigationBar();
	void draw() override;
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE) override;
	BOOL handleHover(S32 x, S32 y, MASK mask) override;
	BOOL handleMouseDown(S32 x, S32 y, MASK mask) override;
	BOOL handleMouseUp(S32 x, S32 y, MASK mask) override;
	void refreshLocation();
	void refreshHomeButton();
	void refresh();
	void setVisibleForMouselook(bool visible);
	void updateFloaterViewTop();
	S32 getOccupiedHeight() const;
	void goToHistoryIndex(S32 index);
private:
	struct HistoryItem
	{
		std::string title;
		LLVector3d global_pos;
	};
	void updateNavButtons();
	void ensureCurrentInHistory();
	void rebuildLocationList(const std::string& filter);
	void addTypedHistory(const std::string& text);
	std::string getLocationText() const;
	std::string getPrettyLocation() const;
	std::string getCurrentSLURL() const;
	LLLineEditor* getLocationEditor() const;
	bool isLocationFieldFocused() const;
	void setLocationFieldText(const std::string& text);
	void showLocationName();
	void showLocationSLURL();
	void onBackClicked();
	void onForwardClicked();
	void onBackHeld(const LLSD& param);
	void onForwardHeld(const LLSD& param);
	void onHomeClicked();
	void onLandClicked();
	void onLightingClicked();
	void onAddLandmarkClicked();
	void onSearch();
	void onLocationSelection();
	void onLocationPrearrange(const LLSD& data);
	void onLocationFocusReceived();
	void onLocationFocusLost();
	void onTeleportFinished(const LLVector3d& pos);
	void onTeleportFailed();
	void showHistoryMenu(bool backward);
	void applyVisibility();
	void applyFlexibleLayout();
	void setSearchWidth(S32 width);
	S32 clampSearchWidth(S32 width) const;
	bool isSearchVisible() const;
	bool isOverSplitter(S32 x, S32 y) const;
	static void onHistoryMenuItem(void* data);
	LLButton*		mBtnBack;
	LLButton*		mBtnForward;
	LLButton*		mBtnHome;
	LLButton*		mBtnLand;
	LLButton*		mBtnLighting;
	LLButton*		mBtnAddLandmark;
	LLComboBox*		mLocationCombo;
	LLUICtrl*		mSearchEditor;
	LLUICtrl*		mSearchBtn;
	LLView*			mSearchBevel;
	std::vector<HistoryItem> mHistory;
	std::vector<std::string> mTypedHistory;
	S32				mCurrent;
	S32				mPendingIndex;
	bool			mNavigating;
	bool			mIgnoreNavClick;
	bool			mMouselookHidden;
	bool			mResizingFields;
	S32				mSearchWidth;
	S32				mDragStartX;
	S32				mDragStartSearchWidth;
	boost::signals2::connection mTeleportFinishedSlot;
	boost::signals2::connection mTeleportFailedSlot;
	boost::signals2::connection mParcelChangedSlot;
	boost::signals2::connection mRegionChangedSlot;
	boost::signals2::connection mShowNavSlot;
};
extern LLNavigationBar* gNavigationBar;
#endif
