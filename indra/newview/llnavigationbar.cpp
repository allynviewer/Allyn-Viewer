/**
 * @file llnavigationbar.cpp
 * @brief Lightweight Firestorm-style navigation bar.
 */
#include "llviewerprecompiledheaders.h"
#include "llnavigationbar.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentui.h"
#include "llbutton.h"
#include "llcontrol.h"
#include "llcombobox.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llfloater.h"
#include "llformat.h"
#include "llfloaterland.h"
#include "llfloatersearch.h"
#include "llfloaterwindlight.h"
#include "llmenucommands.h"
#include "llfocusmgr.h"
#include "llhudview.h"
#include "lllandmarkactions.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llnotify.h"
#include "llnotifyiconbar.h"
#include "llslurl.h"
#include "llstring.h"
#include "lltrans.h"
#include "llurldispatcher.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "rlvactions.h"
#include "rlvcommon.h"
#include "rlvhandler.h"
#include "lfsimfeaturehandler.h"
#include <algorithm>
LLNavigationBar* gNavigationBar = NULL;
S32 NAV_BAR_HEIGHT = 29;
extern S32 MENU_BAR_HEIGHT;
static const S32 TYPED_HISTORY_MAX = 20;
static const S32 LANDMARK_LIST_MAX = 30;
static const S32 NAV_LOCATION_LEFT = 240;
static const S32 NAV_FIELD_BOTTOM = 1;
static const S32 NAV_FIELD_HEIGHT = 27;
static const S32 NAV_ADD_WIDTH = 22;
static const S32 NAV_SEARCH_RIGHT_INSET = 21;
static const S32 NAV_GAP_COMBO_ADD = 6;
static const S32 NAV_GAP_ADD_SEARCH = 3;
static const S32 NAV_SPLITTER_PAD = 4;
static const S32 NAV_MIN_SEARCH_WIDTH = 80;
static const S32 NAV_MIN_LOCATION_WIDTH = 120;
static const S32 NAV_DEFAULT_SEARCH_WIDTH = 226;
static const S32 NAV_ADD_RIGHT_MARGIN = 6;
static void placeNavCtrl(LLView* view, S32 left, S32 right)
{
	if (!view)
	{
		return;
	}
	LLRect r(left, NAV_FIELD_BOTTOM + NAV_FIELD_HEIGHT, right, NAV_FIELD_BOTTOM);
	view->setRect(r);
	view->reshape(r.getWidth(), r.getHeight(), TRUE);
}
LLNavigationBar::LLNavigationBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, LLRect(), FALSE),
	mBtnBack(NULL),
	mBtnForward(NULL),
	mBtnHome(NULL),
	mBtnLand(NULL),
	mBtnLighting(NULL),
	mBtnAddLandmark(NULL),
	mLocationCombo(NULL),
	mSearchEditor(NULL),
	mSearchBtn(NULL),
	mSearchBevel(NULL),
	mCurrent(-1),
	mPendingIndex(-1),
	mNavigating(false),
	mIgnoreNavClick(false),
	mMouselookHidden(false),
	mResizingFields(false),
	mSearchWidth(NAV_DEFAULT_SEARCH_WIDTH),
	mDragStartX(0),
	mDragStartSearchWidth(NAV_DEFAULT_SEARCH_WIDTH)
{
	setMouseOpaque(TRUE);
	setIsChrome(TRUE);
	setFocusRoot(TRUE);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_navigation_bar.xml");
	reshape(rect.getWidth(), llmax(rect.getHeight(), NAV_BAR_HEIGHT), TRUE);
	translate(0, rect.mBottom - getRect().mBottom);
	setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_TOP);
	mBtnBack = getChild<LLButton>("back_btn");
	mBtnForward = getChild<LLButton>("forward_btn");
	mBtnHome = getChild<LLButton>("home_btn");
	mBtnLand = getChild<LLButton>("About_Land");
	mBtnLighting = getChild<LLButton>("PersonalLighting");
	mBtnAddLandmark = getChild<LLButton>("add_landmark_btn");
	mLocationCombo = getChild<LLComboBox>("location_combo");
	mSearchEditor = getChild<LLUICtrl>("search_editor");
	mSearchBtn = getChild<LLUICtrl>("search_btn");
	mSearchBevel = getChildView("navbar_search_bevel_bg", TRUE, FALSE);
	mBtnBack->setClickedCallback(boost::bind(&LLNavigationBar::onBackClicked, this));
	mBtnForward->setClickedCallback(boost::bind(&LLNavigationBar::onForwardClicked, this));
	mBtnBack->setHeldDownCallback(boost::bind(&LLNavigationBar::onBackHeld, this, _2));
	mBtnForward->setHeldDownCallback(boost::bind(&LLNavigationBar::onForwardHeld, this, _2));
	mBtnHome->setClickedCallback(boost::bind(&LLNavigationBar::onHomeClicked, this));
	mBtnLand->setClickedCallback(boost::bind(&LLNavigationBar::onLandClicked, this));
	mBtnLighting->setClickedCallback(boost::bind(&LLNavigationBar::onLightingClicked, this));
	mBtnAddLandmark->setClickedCallback(boost::bind(&LLNavigationBar::onAddLandmarkClicked, this));
	mSearchEditor->setCommitCallback(boost::bind(&LLNavigationBar::onSearch, this));
	mSearchBtn->setCommitCallback(boost::bind(&LLNavigationBar::onSearch, this));
	mLocationCombo->setSuppressAutoComplete(true);
	mLocationCombo->setSuppressTentative(true);
	mLocationCombo->setCommitCallback(boost::bind(&LLNavigationBar::onLocationSelection, this));
	mLocationCombo->setPrearrangeCallback(boost::bind(&LLNavigationBar::onLocationPrearrange, this, _2));
	if (LLLineEditor* search = getChild<LLLineEditor>("search_editor", TRUE, FALSE))
	{
		search->setVAlign(LLFontGL::VCENTER);
		const LLColor4 white(1.f, 1.f, 1.f, 1.f);
		search->setFgColor(white);
		search->setReadOnlyFgColor(white);
		search->setTentativeFgColor(white);
		search->setCursorColor(white);
	}
	if (LLLineEditor* location = mLocationCombo->getChild<LLLineEditor>("combo_text_entry", TRUE, FALSE))
	{
		location->setVAlign(LLFontGL::VCENTER);
		const LLColor4 white(1.f, 1.f, 1.f, 1.f);
		const LLColor4 field(16.f / 255.f, 18.f / 255.f, 42.f / 255.f, 1.f);
		location->setFgColor(white);
		location->setReadOnlyFgColor(white);
		location->setTentativeFgColor(white);
		location->setCursorColor(white);
		location->setWriteableBgColor(field);
		location->setFocusBgColor(field);
		location->setSelectAllonFocusReceived(TRUE);
		location->setTentative(FALSE);
		location->setFocusReceivedCallback(boost::bind(&LLNavigationBar::onLocationFocusReceived, this));
		location->setFocusLostCallback(boost::bind(&LLNavigationBar::onLocationFocusLost, this));
	}
	mTeleportFinishedSlot = LLViewerParcelMgr::getInstance()->setTeleportFinishedCallback(
		boost::bind(&LLNavigationBar::onTeleportFinished, this, _1));
	mTeleportFailedSlot = LLViewerParcelMgr::getInstance()->setTeleportFailedCallback(
		boost::bind(&LLNavigationBar::onTeleportFailed, this));
	mParcelChangedSlot = LLViewerParcelMgr::getInstance()->addAgentParcelChangedCallback(
		boost::bind(&LLNavigationBar::refreshLocation, this));
	mRegionChangedSlot = gAgent.addRegionChangedCallback(
		boost::bind(&LLNavigationBar::refreshLocation, this));
	updateNavButtons();
	refreshHomeButton();
	mSearchWidth = gSavedSettings.getS32("NavigationBarSearchWidth");
	if (mSearchWidth < NAV_MIN_SEARCH_WIDTH)
	{
		mSearchWidth = NAV_DEFAULT_SEARCH_WIDTH;
	}
	applyFlexibleLayout();
	refreshLocation();
	if (LLControlVariable* ctrl = gSavedSettings.getControl("ShowNavigationBar"))
	{
		mShowNavSlot = ctrl->getSignal()->connect(
			boost::bind(&LLNavigationBar::applyVisibility, this));
	}
}
LLNavigationBar::~LLNavigationBar()
{
	mTeleportFinishedSlot.disconnect();
	mTeleportFailedSlot.disconnect();
	mParcelChangedSlot.disconnect();
	mRegionChangedSlot.disconnect();
	mShowNavSlot.disconnect();
}
void LLNavigationBar::draw()
{
	applyVisibility();
	LLPanel::draw();
}
void LLNavigationBar::refresh()
{
	applyVisibility();
}
void LLNavigationBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	applyFlexibleLayout();
}
BOOL LLNavigationBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (isOverSplitter(x, y))
	{
		gFocusMgr.setMouseCapture(this);
		mResizingFields = true;
		mDragStartX = x;
		mDragStartSearchWidth = mSearchWidth;
		return TRUE;
	}
	return LLPanel::handleMouseDown(x, y, mask);
}
BOOL LLNavigationBar::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
		if (mResizingFields)
		{
			mResizingFields = false;
			gSavedSettings.setS32("NavigationBarSearchWidth", mSearchWidth);
		}
		return TRUE;
	}
	return LLPanel::handleMouseUp(x, y, mask);
}
BOOL LLNavigationBar::handleHover(S32 x, S32 y, MASK mask)
{
	if (mResizingFields && hasMouseCapture())
	{
		setSearchWidth(mDragStartSearchWidth - (x - mDragStartX));
		getWindow()->setCursor(UI_CURSOR_SIZEWE);
		return TRUE;
	}
	if (isOverSplitter(x, y))
	{
		getWindow()->setCursor(UI_CURSOR_SIZEWE);
		return TRUE;
	}
	return LLPanel::handleHover(x, y, mask);
}
bool LLNavigationBar::isSearchVisible() const
{
	static const LLCachedControl<bool> show_search(gSavedSettings, "ShowSearchBar", true);
	return show_search;
}
S32 LLNavigationBar::clampSearchWidth(S32 width) const
{
	const S32 bar_w = getRect().getWidth();
	S32 leftover = bar_w - NAV_LOCATION_LEFT - NAV_ADD_WIDTH
		- NAV_GAP_COMBO_ADD - NAV_GAP_ADD_SEARCH - NAV_SEARCH_RIGHT_INSET;
	S32 max_search = leftover - NAV_MIN_LOCATION_WIDTH;
	if (max_search < NAV_MIN_SEARCH_WIDTH)
	{
		max_search = llmax(40, leftover - 60);
	}
	return llclamp(width, NAV_MIN_SEARCH_WIDTH, llmax(NAV_MIN_SEARCH_WIDTH, max_search));
}
void LLNavigationBar::setSearchWidth(S32 width)
{
	mSearchWidth = clampSearchWidth(width);
	applyFlexibleLayout();
}
void LLNavigationBar::applyFlexibleLayout()
{
	if (!mLocationCombo || !mBtnAddLandmark)
	{
		return;
	}
	const std::string saved_location = getLocationText();
	const S32 bar_w = getRect().getWidth();
	const bool show_search = isSearchVisible();
	S32 add_left;
	S32 add_right;
	S32 combo_right;
	if (show_search && mSearchEditor)
	{
		const S32 search_w = clampSearchWidth(mSearchWidth);
		const S32 search_right = bar_w - NAV_SEARCH_RIGHT_INSET;
		const S32 search_left = search_right - search_w;
		add_right = search_left - NAV_GAP_ADD_SEARCH;
		add_left = add_right - NAV_ADD_WIDTH;
		combo_right = add_left - NAV_GAP_COMBO_ADD;
		placeNavCtrl(mSearchEditor, search_left, search_right);
	}
	else
	{
		add_right = bar_w - NAV_ADD_RIGHT_MARGIN;
		add_left = add_right - NAV_ADD_WIDTH;
		combo_right = add_left - NAV_GAP_COMBO_ADD;
	}
	if (combo_right < NAV_LOCATION_LEFT + NAV_MIN_LOCATION_WIDTH)
	{
		combo_right = NAV_LOCATION_LEFT + NAV_MIN_LOCATION_WIDTH;
	}
	placeNavCtrl(mLocationCombo, NAV_LOCATION_LEFT, combo_right);
	placeNavCtrl(mBtnAddLandmark, add_left, add_right);
	if (isLocationFieldFocused())
	{
		if (LLLineEditor* editor = getLocationEditor())
		{
			if (!editor->isDirty())
			{
				showLocationSLURL();
			}
		}
	}
	else if (!saved_location.empty())
	{
		setLocationFieldText(saved_location);
	}
}
bool LLNavigationBar::isOverSplitter(S32 x, S32 y) const
{
	if (!isSearchVisible() || !mLocationCombo || !mSearchEditor)
	{
		return false;
	}
	const LLRect& combo = mLocationCombo->getRect();
	const LLRect& search = mSearchEditor->getRect();
	const S32 top = llmax(combo.mTop, search.mTop);
	const S32 bottom = llmin(combo.mBottom, search.mBottom);
	if (y < bottom || y > top)
	{
		return false;
	}
	const S32 hit_left = combo.mRight - NAV_SPLITTER_PAD;
	const S32 hit_right = search.mLeft + NAV_SPLITTER_PAD;
	if (x < hit_left || x > hit_right)
	{
		return false;
	}
	if (mBtnAddLandmark && mBtnAddLandmark->getVisible()
		&& mBtnAddLandmark->getRect().pointInRect(x, y))
	{
		return false;
	}
	return true;
}
void LLNavigationBar::applyVisibility()
{
	static const LLCachedControl<bool> show_nav(gSavedSettings, "ShowNavigationBar", true);
	bool show = show_nav && !mMouselookHidden && !gAgentCamera.cameraMouselook();
	if (getVisible() != (BOOL)show)
	{
		setVisible(show);
		if (show)
		{
			applyFlexibleLayout();
		}
		updateFloaterViewTop();
	}
	static const LLCachedControl<bool> show_search(gSavedSettings, "ShowSearchBar", true);
	const bool search_vis = show_search;
	bool search_changed = false;
	if (mSearchEditor)
	{
		search_changed = mSearchEditor->getVisible() != (BOOL)search_vis;
		mSearchEditor->setVisible(search_vis);
	}
	if (mSearchBtn)
	{
		mSearchBtn->setVisible(search_vis);
	}
	if (mSearchBevel)
	{
		mSearchBevel->setVisible(search_vis);
	}
	if (search_changed)
	{
		applyFlexibleLayout();
	}
}
S32 LLNavigationBar::getOccupiedHeight() const
{
	return getVisible() ? getRect().getHeight() : 0;
}
void LLNavigationBar::updateFloaterViewTop()
{
	LLView* parent = NULL;
	if (gFloaterView)
	{
		parent = gFloaterView->getParent();
	}
	if (!parent && gViewerWindow)
	{
		parent = gViewerWindow->getRootView();
	}
	if (!parent)
	{
		return;
	}
	S32 occupied = getOccupiedHeight();
	S32 menu_h = (gMenuBarView) ? gMenuBarView->getRect().getHeight() : MENU_BAR_HEIGHT;
	S32 desired_top = parent->getRect().getHeight() - menu_h - occupied;
	if (gFloaterView)
	{
		LLRect r = gFloaterView->getRect();
		if (r.mTop != desired_top)
		{
			r.mTop = desired_top;
			gFloaterView->reshapeFloater(r.getWidth(), r.getHeight(), TRUE, ADJUST_VERTICAL_NO);
			gFloaterView->setShape(r);
		}
	}
	if (gHUDView)
	{
		LLRect hud = gHUDView->getRect();
		if (hud.mTop != desired_top)
		{
			hud.mTop = desired_top;
			gHUDView->reshape(hud.getWidth(), hud.getHeight(), TRUE);
			gHUDView->setRect(hud);
		}
	}
	if (gNotifyIconBar)
	{
		gNotifyIconBar->updatePosition();
	}
	if (gNotifyBoxView)
	{
		gNotifyBoxView->layoutStack();
	}
}
void LLNavigationBar::setVisibleForMouselook(bool visible)
{
	mMouselookHidden = !visible;
	applyVisibility();
	updateFloaterViewTop();
}
void LLNavigationBar::refreshHomeButton()
{
	if (!mBtnHome)
	{
		return;
	}
	bool enable = !(gRlvHandler.hasBehaviour(RLV_BHVR_TPLM) && gRlvHandler.hasBehaviour(RLV_BHVR_TPLOC));
	mBtnHome->setEnabled(enable);
}
void LLNavigationBar::refreshLocation()
{
	if (!mLocationCombo)
	{
		return;
	}
	if (isLocationFieldFocused())
	{
		if (LLLineEditor* editor = getLocationEditor())
		{
			if (!editor->isDirty())
			{
				showLocationSLURL();
			}
		}
	}
	else
	{
		showLocationName();
	}
	if (mBtnAddLandmark)
	{
		mBtnAddLandmark->setEnabled(LLLandmarkActions::canCreateLandmarkHere());
	}
	ensureCurrentInHistory();
}
std::string LLNavigationBar::getPrettyLocation() const
{
	std::string location_name;
	LLViewerRegion* region = gAgent.getRegion();
	if (RlvActions::hasBehaviour(RLV_BHVR_SHOWLOC))
	{
		if (region)
		{
			return llformat("%s (%s) - %s",
				RlvStrings::getString(RLV_STRING_HIDDEN_REGION).c_str(),
				region->getSimAccessString().c_str(),
				RlvStrings::getString(RLV_STRING_HIDDEN).c_str());
		}
		return RlvStrings::getString(RLV_STRING_HIDDEN);
	}
	if (!LLAgentUI::buildLocationString(location_name, LLAgentUI::LOCATION_FORMAT_FULL))
	{
		return region ? region->getName() : std::string("(Unknown)");
	}
	const std::string& grid(LFSimFeatureHandler::instance().gridName());
	if (!grid.empty())
	{
		location_name += ", " + grid;
	}
	return location_name;
}
std::string LLNavigationBar::getCurrentSLURL() const
{
	if (RlvActions::hasBehaviour(RLV_BHVR_SHOWLOC))
	{
		return getPrettyLocation();
	}
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
	{
		return std::string();
	}
	std::string slurl = LLSLURL(region->getName(), gAgent.getPositionAgent()).getSLURLString();
	if (LLStringUtil::startsWith(slurl, "http://"))
	{
		slurl.replace(0, 4, "https");
	}
	return slurl;
}
LLLineEditor* LLNavigationBar::getLocationEditor() const
{
	return mLocationCombo ? mLocationCombo->getChild<LLLineEditor>("combo_text_entry", TRUE, FALSE) : NULL;
}
bool LLNavigationBar::isLocationFieldFocused() const
{
	LLLineEditor* editor = getLocationEditor();
	return editor && editor->hasFocus();
}
void LLNavigationBar::setLocationFieldText(const std::string& text)
{
	if (!mLocationCombo)
	{
		return;
	}
	mLocationCombo->setTextEntry(text);
	if (LLLineEditor* editor = getLocationEditor())
	{
		editor->setTentative(FALSE);
		editor->resetDirty();
	}
}
void LLNavigationBar::showLocationName()
{
	setLocationFieldText(getPrettyLocation());
}
void LLNavigationBar::showLocationSLURL()
{
	const std::string slurl = getCurrentSLURL();
	if (slurl.empty())
	{
		showLocationName();
		return;
	}
	setLocationFieldText(slurl);
}
void LLNavigationBar::onLocationFocusReceived()
{
	showLocationSLURL();
	if (LLLineEditor* editor = getLocationEditor())
	{
		editor->selectAll();
	}
}
void LLNavigationBar::onLocationFocusLost()
{
	showLocationName();
}
void LLNavigationBar::ensureCurrentInHistory()
{
	if (!mHistory.empty() || !gAgent.getRegion())
	{
		return;
	}
	HistoryItem item;
	item.global_pos = gAgent.getPositionGlobal();
	if (!LLAgentUI::buildLocationString(item.title, LLAgentUI::LOCATION_FORMAT_LANDMARK))
	{
		item.title = gAgent.getRegion()->getName();
	}
	mHistory.push_back(item);
	mCurrent = 0;
	updateNavButtons();
}
void LLNavigationBar::updateNavButtons()
{
	if (mBtnBack)
	{
		mBtnBack->setEnabled(mCurrent > 0);
	}
	if (mBtnForward)
	{
		mBtnForward->setEnabled(mCurrent >= 0 && mCurrent + 1 < (S32)mHistory.size());
	}
}
void LLNavigationBar::onBackClicked()
{
	if (mIgnoreNavClick)
	{
		mIgnoreNavClick = false;
		return;
	}
	if (mCurrent <= 0)
	{
		return;
	}
	goToHistoryIndex(mCurrent - 1);
}
void LLNavigationBar::onForwardClicked()
{
	if (mIgnoreNavClick)
	{
		mIgnoreNavClick = false;
		return;
	}
	if (mCurrent < 0 || mCurrent + 1 >= (S32)mHistory.size())
	{
		return;
	}
	goToHistoryIndex(mCurrent + 1);
}
void LLNavigationBar::onBackHeld(const LLSD& param)
{
	if (param["count"].asInteger() > 0)
	{
		return;
	}
	mIgnoreNavClick = true;
	showHistoryMenu(true);
}
void LLNavigationBar::onForwardHeld(const LLSD& param)
{
	if (param["count"].asInteger() > 0)
	{
		return;
	}
	mIgnoreNavClick = true;
	showHistoryMenu(false);
}
void LLNavigationBar::goToHistoryIndex(S32 index)
{
	if (index < 0 || index >= (S32)mHistory.size())
	{
		return;
	}
	mPendingIndex = index;
	mNavigating = true;
	gAgent.teleportViaLocation(mHistory[index].global_pos);
}
void LLNavigationBar::showHistoryMenu(bool backward)
{
	if (mHistory.empty())
	{
		return;
	}
	LLMenuGL* menu = new LLMenuGL("navbar_history");
	menu->setCanTearOff(FALSE);
	if (backward)
	{
		for (S32 i = mCurrent - 1; i >= 0; --i)
		{
			menu->addChild(new LLMenuItemCallGL(
				llformat("%d. %s", i, mHistory[i].title.c_str()),
				onHistoryMenuItem, NULL, (void*)(intptr_t)i));
		}
	}
	else
	{
		for (S32 i = mCurrent + 1; i < (S32)mHistory.size(); ++i)
		{
			menu->addChild(new LLMenuItemCallGL(
				llformat("%d. %s", i, mHistory[i].title.c_str()),
				onHistoryMenuItem, NULL, (void*)(intptr_t)i));
		}
	}
	if (menu->getItemCount() == 0)
	{
		delete menu;
		return;
	}
	LLButton* btn = backward ? mBtnBack : mBtnForward;
	menu->updateParent(LLMenuGL::sMenuContainer);
	gFocusMgr.setMouseCapture(NULL);
	LLMenuGL::showPopup(btn, menu, 0, 0);
}
void LLNavigationBar::onHistoryMenuItem(void* data)
{
	if (!gNavigationBar)
	{
		return;
	}
	gNavigationBar->goToHistoryIndex((S32)(intptr_t)data);
}
void LLNavigationBar::onHomeClicked()
{
	gAgent.teleportHome();
}
void LLNavigationBar::onLandClicked()
{
	show_floater("about land");
}
void LLNavigationBar::onLightingClicked()
{
	show_floater("Windlight");
}
void LLNavigationBar::onAddLandmarkClicked()
{
	LLLandmarkActions::createLandmarkHere();
}
void LLNavigationBar::onSearch()
{
	std::string query;
	if (mSearchEditor)
	{
		query = mSearchEditor->getValue().asString();
	}
	if (query.empty())
	{
		show_floater("search");
		return;
	}
	LLFloaterSearch::SearchQuery search;
	search.query = query;
	LLFloaterSearch::showInstance(search);
}
void LLNavigationBar::addTypedHistory(const std::string& text)
{
	if (text.empty())
	{
		return;
	}
	mTypedHistory.erase(std::remove(mTypedHistory.begin(), mTypedHistory.end(), text), mTypedHistory.end());
	mTypedHistory.insert(mTypedHistory.begin(), text);
	if ((S32)mTypedHistory.size() > TYPED_HISTORY_MAX)
	{
		mTypedHistory.resize(TYPED_HISTORY_MAX);
	}
}
void LLNavigationBar::rebuildLocationList(const std::string& filter)
{
	if (!mLocationCombo)
	{
		return;
	}
	const std::string current_text = getLocationText();
	mLocationCombo->removeall();
	bool added_section = false;
	if (!mHistory.empty())
	{
		if (LLScrollListItem* hdr = mLocationCombo->add(LLTrans::getString("NavBarHistory")))
		{
			hdr->setEnabled(FALSE);
		}
		for (S32 i = (S32)mHistory.size() - 1; i >= 0; --i)
		{
			if (!filter.empty() && mHistory[i].title.find(filter) == std::string::npos)
			{
				continue;
			}
			LLSD value;
			value["type"] = "history";
			value["index"] = i;
			mLocationCombo->add(mHistory[i].title, value);
			added_section = true;
		}
	}
	if (!mTypedHistory.empty())
	{
		if (added_section)
		{
			mLocationCombo->addSeparator();
		}
		if (LLScrollListItem* hdr = mLocationCombo->add(LLTrans::getString("NavBarTyped")))
		{
			hdr->setEnabled(FALSE);
		}
		for (const std::string& typed : mTypedHistory)
		{
			if (!filter.empty() && typed.find(filter) == std::string::npos)
			{
				continue;
			}
			LLSD value;
			value["type"] = "typed";
			value["text"] = typed;
			mLocationCombo->add(typed, value);
			added_section = true;
		}
	}
	if (!filter.empty())
	{
		std::string name_filter = filter;
		LLInventoryModel::item_array_t landmarks = LLLandmarkActions::fetchLandmarksByName(name_filter, TRUE);
		if (!landmarks.empty())
		{
			if (added_section)
			{
				mLocationCombo->addSeparator();
			}
			if (LLScrollListItem* hdr = mLocationCombo->add(LLTrans::getString("NavBarLandmarks")))
			{
				hdr->setEnabled(FALSE);
			}
			S32 count = 0;
			for (LLInventoryModel::item_array_t::iterator it = landmarks.begin();
				 it != landmarks.end() && count < LANDMARK_LIST_MAX; ++it, ++count)
			{
				LLViewerInventoryItem* item = *it;
				if (!item)
				{
					continue;
				}
				LLSD value;
				value["type"] = "landmark";
				value["asset"] = item->getAssetUUID();
				mLocationCombo->add(item->getName(), value);
			}
		}
	}
	if (!current_text.empty())
	{
		mLocationCombo->setTextEntry(current_text);
	}
	else
	{
		refreshLocation();
	}
}
void LLNavigationBar::onLocationPrearrange(const LLSD& data)
{
	std::string filter = data.asString();
	const std::string current = getLocationText();
	if (!filter.empty() && filter == current)
	{
		filter.clear();
	}
	rebuildLocationList(filter);
}
std::string LLNavigationBar::getLocationText() const
{
	if (!mLocationCombo)
	{
		return std::string();
	}
	std::string text = mLocationCombo->getTextEntry();
	if (text.empty())
	{
		text = mLocationCombo->getSimple();
	}
	return text;
}
void LLNavigationBar::onLocationSelection()
{
	if (!mLocationCombo)
	{
		return;
	}
	LLSD selected = mLocationCombo->getSelectedValue();
	if (mLocationCombo->getCurrentIndex() >= 0 && selected.isMap() && selected.has("type"))
	{
		const std::string type = selected["type"].asString();
		if (type == "landmark")
		{
			gAgent.teleportViaLandmark(selected["asset"].asUUID());
			return;
		}
		if (type == "history")
		{
			goToHistoryIndex(selected["index"].asInteger());
			return;
		}
		if (type == "typed")
		{
			std::string typed = selected["text"].asString();
			LLStringUtil::trim(typed);
			if (typed.empty())
			{
				return;
			}
			addTypedHistory(typed);
			LLSLURL slurl(typed);
			if (slurl.getType() == LLSLURL::LOCATION || slurl.getType() == LLSLURL::HOME_LOCATION)
			{
				LLURLDispatcher::dispatch(slurl.getSLURLString(), "clicked", NULL, true);
			}
			else
			{
				LLURLDispatcher::dispatch(typed, "clicked", NULL, true);
			}
			return;
		}
	}
	std::string typed = getLocationText();
	LLStringUtil::trim(typed);
	if (typed.empty() || typed == getPrettyLocation() || typed == getCurrentSLURL())
	{
		return;
	}
	addTypedHistory(typed);
	LLSLURL slurl(typed);
	if (slurl.getType() == LLSLURL::LOCATION || slurl.getType() == LLSLURL::HOME_LOCATION)
	{
		LLURLDispatcher::dispatch(slurl.getSLURLString(), "clicked", NULL, true);
		return;
	}
	LLURLDispatcher::dispatch(typed, "clicked", NULL, true);
}
void LLNavigationBar::onTeleportFinished(const LLVector3d& pos)
{
	if (mNavigating)
	{
		mCurrent = mPendingIndex;
		mNavigating = false;
		mPendingIndex = -1;
	}
	else
	{
		HistoryItem item;
		item.global_pos = pos.isExactlyZero() ? gAgent.getPositionGlobal() : pos;
		if (!LLAgentUI::buildLocationString(item.title, LLAgentUI::LOCATION_FORMAT_LANDMARK))
		{
			LLViewerRegion* region = gAgent.getRegion();
			item.title = region ? region->getName() : std::string("Teleport");
		}
		if (mCurrent >= 0 && mCurrent + 1 < (S32)mHistory.size())
		{
			mHistory.erase(mHistory.begin() + mCurrent + 1, mHistory.end());
		}
		mHistory.push_back(item);
		mCurrent = (S32)mHistory.size() - 1;
	}
	updateNavButtons();
	refreshLocation();
}
void LLNavigationBar::onTeleportFailed()
{
	mNavigating = false;
	mPendingIndex = -1;
}
