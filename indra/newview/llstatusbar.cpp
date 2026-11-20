/** 
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
*
* $LicenseInfo:firstyear=2002&license=viewergpl$
* 
* Copyright (c) 2002-2009, Linden Research, Inc.
* Modifications Copyright (C) 2026 - Allyn Viewer
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at
* http://secondlifegrid.net/programs/open_source/licensing/flossexception
* 
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
* 
* ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/
#include "llviewerprecompiledheaders.h"
#include "llstatusbar.h"
#include "llagent.h"
#include "llbutton.h"
#include "llcommandhandler.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterchat.h"
#include "llfloaterinventory.h"
#include "llfloaterland.h"
#include "llmenucommands.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llfloaterstats.h"
#include "llhudicon.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llmenuoptionpathfindingrebakenavmesh.h"
#include "llnotify.h"
#include "llnotifyiconbar.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmeshstatus.h"
#include "llimview.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llui.h"
#include "llnavigationbar.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatarself.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llstatgraph.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerthrottle.h"
#include "lluictrlfactory.h"
#include "llagentui.h"
#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "llappviewer.h"
#include "imageids.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"
#include <iomanip>
#include "hippogridmanager.h"
#include "lfsimfeaturehandler.h"
#include "rlvactions.h"
#include "rlvcommon.h"
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 0;
extern S32 MENU_BAR_HEIGHT;
const S32 MENU_PARCEL_SPACING = 1;
const S32 SIM_STAT_WIDTH = 8;
const F32 SIM_WARN_FRACTION = 0.75f;
const F32 SIM_FULL_FRACTION = 0.98f;
const LLColor4 SIM_OK_COLOR(0.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_WARN_COLOR(1.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_FULL_COLOR(1.f, 0.f, 0.f, 1.f);
const F32 ICON_TIMER_EXPIRY		= 3.f;
const F32 ICON_FLASH_FREQUENCY	= 2.f;
const S32 TEXT_HEIGHT = 18;
const S32 STATUS_HPAD = LLFloatingBarStyle::CAP_PAD;
const S32 STATUS_VPAD = 5;
const S32 STATUS_GROUP_GAP = 10;
const S32 STATUS_BUY_GAP = 4;
const U32 STATUS_ICON_SCRIPT = 1 << 0;
const U32 STATUS_ICON_HEALTH = 1 << 1;
const U32 STATUS_ICON_NO_FLY = 1 << 2;
const U32 STATUS_ICON_NO_BUILD = 1 << 3;
const U32 STATUS_ICON_NO_SCRIPTS = 1 << 4;
const U32 STATUS_ICON_NO_PUSH = 1 << 5;
const U32 STATUS_ICON_NO_VOICE = 1 << 6;
const U32 STATUS_ICON_SEE_AV = 1 << 7;
const U32 STATUS_ICON_PF_DIRTY = 1 << 8;
const U32 STATUS_ICON_PF_DISABLED = 1 << 9;
const U32 STATUS_ICON_BUY_LAND = 1 << 10;
static void onClickParcelInfo();
static bool rebakeRegionCallback(const LLSD& n, const LLSD& r);
static void pf_dirty_click() { LLNotificationsUtil::add("PathfindingDirty", LLSD(), LLSD(), rebakeRegionCallback); }
static void onClickScripts();
std::vector<std::string> LLStatusBar::sDays;
std::vector<std::string> LLStatusBar::sMonths;
const U32 LLStatusBar::MAX_DATE_STRING_LENGTH = 2000;
class LLDispatchUPCBalance : public LLDispatchHandler
{
public:
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings)
	{
		S32 upc = atoi(strings[0].c_str());
		gStatusBar->setUPC(upc);
		return true;
	}
};
static LLDispatchUPCBalance sDispatchUPCBalance;
static void toggle_time_value()
{
	LLControlVariable* control = gSavedSettings.getControl("LiruLocalTime");
	control->set(!control->get());
}
LLStatusBar::LLStatusBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, LLRect(), FALSE),
mTextFPS(NULL),
mBalance(0),
mLastFPS(-1),
mSmoothFPS(-1.f),
mFPSRefresh(0.f),
mFPSWidth(72),
mUPC(0),
mHealth(100),
mSquareMetersCredit(0),
mSquareMetersCommitted(0),
mRegionCrossingSlot(),
mNavMeshSlot(),
mIsNavMeshDirty(false),
mMouselookHidden(false),
mLayoutDirty(true),
mCompactHost(NULL),
mParcelHost(NULL),
mCompactParcelW(-1),
mCompactAccountW(-1),
mUpdatingRect(false),
mClockLocal(false),
mClockTime(0),
mIconMask(0),
mMenuRight(-1),
mLayoutWidth(-1),
mLeftEnd(0),
mBalanceKnown(false),
mBalanceAutoRequested(false),
mBalanceAutoAt(0.0)
{
	mUPCSupported = gHippoGridManager->getConnectedGrid()->getUPCSupported();
	if (mUPCSupported)
		gGenericDispatcher.addHandler("upcbalance", &sDispatchUPCBalance);
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);
	sDays.reserve(7);
	sMonths.reserve(12);
	mBalanceTimer = new LLFrameTimer();
	mHealthTimer = new LLFrameTimer();
	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_status_bar.xml");
	setFocusRoot(FALSE);
	setupDate();
	mTextParcelName = getChild<LLTextBox>("ParcelNameText" );
	mTextBalance = getChild<LLTextBox>("BalanceText" );
	mBalanceLoadingText = mTextBalance->getText();
	LLStringUtil::trim(mBalanceLoadingText);
	mTextFPS = getChild<LLTextBox>("FPSText");
	mTextFPS->setText(LLStringExplicit("FPS: 0"));
	if (const LLFontGL* font = mTextFPS->getFont())
	{
		mFPSWidth = font->getWidth(std::string("FPS: 000")) + 8;
	}
	mTextUPC = getChild<LLTextBox>("UPCText" );
	mTextHealth = getChild<LLTextBox>("HealthText" );
	mTextTime = getChild<LLTextBox>("TimeText" );
	mTextTime->setClickedCallback(boost::bind(toggle_time_value));
	if (!mUPCSupported)
		mTextUPC->setVisible(false);
	mScriptOut = getChild<LLUICtrl>("scriptout");
	mScriptOut->setCommitCallback(boost::bind(LLFloaterScriptDebug::show, LLUUID::null));
	mHealthV = getChild<LLUICtrl>("health");
	mHealthV->setCommitCallback(boost::bind(LLNotificationsUtil::add, "NotSafe"));
	mNoFly = getChild<LLUICtrl>("no_fly");
	mNoFly->setCommitCallback(boost::bind(LLNotificationsUtil::add, "NoFly"));
	mBuyLand = getChild<LLUICtrl>("buyland");
	mBuyLand->setCommitCallback(boost::bind(&LLViewerParcelMgr::startBuyLand, boost::bind(LLViewerParcelMgr::getInstance), 0));
	mBuyCurrency = getChild<LLUICtrl>("buycurrency");
	mBuyCurrency->setCommitCallback(boost::bind(LLFloaterBuyCurrency::buyCurrency));
	mNoBuild = getChild<LLUICtrl>("no_build");
	mNoBuild->setCommitCallback(boost::bind(LLNotificationsUtil::add, "NoBuild"));
	mPFDirty = getChild<LLUICtrl>("pf_dirty");
	mPFDirty->setCommitCallback(boost::bind(pf_dirty_click));
	mPFDisabled = getChild<LLUICtrl>("pf_disabled");
	mPFDisabled->setCommitCallback(boost::bind(LLNotificationsUtil::add, "DynamicPathfindingDisabled"));
	mStatusSeeAV = getChild<LLUICtrl>("status_SeeAV");
	mStatusSeeAV->setCommitCallback(boost::bind(LLNotificationsUtil::add, "SeeAvatars"));
	mNoScripts = getChild<LLUICtrl>("no_scripts");
	mNoScripts->setCommitCallback(boost::bind(onClickScripts));
	mRestrictPush = getChild<LLUICtrl>("restrictpush");
	mRestrictPush->setCommitCallback(boost::bind(LLNotificationsUtil::add, "PushRestricted"));
	mStatusNoVoice = getChild<LLUICtrl>("status_no_voice");
	mStatusNoVoice->setCommitCallback(boost::bind(LLNotificationsUtil::add, "NoVoice"));
	mSearchEditor = getChild<LLUICtrl>("search_editor");
	mSearchBtn = getChild<LLUICtrl>("search_btn");
	mSearchBevel = getChildView("menubar_search_bevel_bg");
	mSearchEditor->setVisible(false);
	mSearchBtn->setVisible(false);
	mSearchBevel->setVisible(false);
	mTextParcelName->setVisible(false);
	mTextParcelName->setClickedCallback(boost::bind(onClickParcelInfo));
	mTextBalance->setClickedCallback(boost::bind(LLStatusBar::sendMoneyBalanceRequest, true));
	mBuyCurrency->setLabelArg("[CURRENCY]", gHippoGridManager->getConnectedGrid()->getCurrencySymbol());
	mRegionCrossingSlot = gAgent.addRegionChangedCallback(boost::bind(&LLStatusBar::createNavMeshStatusListenerForCurrentRegion, this));
	createNavMeshStatusListenerForCurrentRegion();
	S32 x = getRect().getWidth() - 2;
	S32 y = 0;
	LLRect r;
	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	mSGBandwidth = new LLStatGraph("BandwidthGraph", r);
	mSGBandwidth->setFollows(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mSGBandwidth->setStat(&LLViewerStats::getInstance()->mKBitStat);
	mSGBandwidth->setLabel(getString("bandwidth_tooltip") + " ");
	mSGBandwidth->setUnits("Kbps");
	mSGBandwidth->setPrecision(0);
	mSGBandwidth->setMouseOpaque(FALSE);
	addChild(mSGBandwidth);
	x -= SIM_STAT_WIDTH + 2;
	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	mSGPacketLoss = new LLStatGraph("PacketLossPercent", r);
	mSGPacketLoss->setFollows(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mSGPacketLoss->setStat(&LLViewerStats::getInstance()->mPacketsLostPercentStat);
	mSGPacketLoss->setLabel(getString("packet_loss_tooltip") + " ");
	mSGPacketLoss->setUnits("%");
	mSGPacketLoss->setMin(0.f);
	mSGPacketLoss->setMax(5.f);
	mSGPacketLoss->setThreshold(0, 0.5f);
	mSGPacketLoss->setThreshold(1, 1.f);
	mSGPacketLoss->setThreshold(2, 3.f);
	mSGPacketLoss->setPrecision(1);
	mSGPacketLoss->setMouseOpaque(FALSE);
	mSGPacketLoss->mPerSec = FALSE;
	addChild(mSGPacketLoss);
	mStatBtn = getChild<LLTextBox>("stat_btn");
	mStatBtn->setClickedCallback(boost::bind(LLFloaterStats::toggleInstance, LLSD()));
	setMouseOpaque(TRUE);
	setBackgroundVisible(FALSE);
	mMarginSlot = LLFloatingBarStyle::connectSetting("FloatingBarMargin", boost::bind(&LLStatusBar::onMarginChanged, this));
}
LLStatusBar::~LLStatusBar()
{
	delete mBalanceTimer;
	mBalanceTimer = NULL;
	delete mHealthTimer;
	mHealthTimer = NULL;
	setParcelHost(NULL);
	setCompactHost(NULL);
	mRegionCrossingSlot.disconnect();
	mNavMeshSlot.disconnect();
	mMarginSlot.disconnect();
	if (gStatusBar == this)
	{
		gStatusBar = NULL;
	}
}
void LLStatusBar::draw()
{
	refresh();
	LLRect pill = getLocalRect();
	bool draw_pill = true;
	if (mMouselookHidden)
	{
		draw_pill = gMenuBarView && gMenuBarView->getVisible();
		pill.mRight = llmin(pill.mRight, mLeftEnd + STATUS_HPAD);
	}
	if (draw_pill)
	{
		mStyle.drawPill(pill, getBackgroundColor());
	}
	LLPanel::draw();
}
static bool sMenuScaleReady = false;
static bool sMenuScaleCompact = false;
static void scaleMenuBar(bool compact)
{
	if (!gMenuBarView)
	{
		return;
	}
	if (sMenuScaleReady && sMenuScaleCompact == compact)
	{
		return;
	}
	sMenuScaleReady = true;
	sMenuScaleCompact = compact;
	const LLFontGL* font = LLFontGL::getFontSansSerif();
	const LLView::child_list_t* children = gMenuBarView->getChildList();
	for (LLView::child_list_const_iter_t it = children->begin(); it != children->end(); ++it)
	{
		if (LLMenuItemGL* item = dynamic_cast<LLMenuItemGL*>(*it))
		{
			item->setFont(font);
		}
	}
	S32 height = MENU_BAR_HEIGHT;
	if (compact)
	{
		height = llmax(14, ll_round(font->getLineHeight()) + 2);
	}
	const LLRect rect = gMenuBarView->getRect();
	if (rect.getHeight() != height)
	{
		gMenuBarView->reshape(rect.getWidth(), height);
	}
	gMenuBarView->needsArrange();
}
void LLStatusBar::updateBarRect()
{
	LLView* parent = getParent();
	if (!parent)
	{
		return;
	}
	const S32 margin = LLFloatingBarStyle::margin();
	const S32 right_slot = NOTIFY_ICON_BAR_WIDTH + LLFloatingBarStyle::BAR_GAP;
	LLRect r;
	r.mLeft = margin;
	r.mRight = llmax(r.mLeft + 2 * STATUS_HPAD, parent->getRect().getWidth() - margin - right_slot);
	if (gNavigationBar && gNavigationBar->isCompact() && gMenuBarView)
	{
		const S32 menu_w = llmax(80, gMenuBarView->getRightmostMenuEdge());
		r.mRight = llmin(r.mRight, r.mLeft + menu_w + STATUS_HPAD * 2);
	}
	const S32 bar_h = LLFloatingBarStyle::topBarHeight();
	r.mTop = parent->getRect().getHeight() - margin;
	r.mBottom = r.mTop - bar_h;
	mUpdatingRect = true;
	setShape(r);
	mUpdatingRect = false;
	if (gMenuBarView)
	{
		fitMenuBar();
		const S32 menu_h = gMenuBarView->getRect().getHeight();
		S32 menu_y = r.mBottom + (bar_h - menu_h) / 2;
		if (menu_y < r.mBottom)
		{
			menu_y = r.mBottom;
		}
		gMenuBarView->setOrigin(r.mLeft + STATUS_HPAD, menu_y);
		gMenuBarView->setFloatingStyle(llmax(2, (bar_h - menu_h) / 2), LLUI::getUIImage("Rounded_Square"));
	}
	mLayoutDirty = true;
}
void LLStatusBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	if (mUpdatingRect || !called_from_parent || !gNavigationBar || !gNavigationBar->isCompact())
	{
		LLPanel::reshape(width, height, called_from_parent);
		return;
	}
	updateBarRect();
}
void LLStatusBar::onMarginChanged()
{
	updateBarRect();
	LLFloatingBarStyle::applyTopLayout();
}
void LLStatusBar::updateClock()
{
	static const LLCachedControl<bool> show_local_time("LiruLocalTime");
	const bool local_time = show_local_time;
	time_t utc_time = local_time ? time(NULL) : time_corrected();
	if (utc_time == mClockTime && local_time == mClockLocal)
	{
		return;
	}
	mClockTime = utc_time;
	mClockLocal = local_time;
	struct tm* internal_time = local_time ? std::localtime(&utc_time) : utc_to_pacific_time(utc_time, gPacificDaylightTime);
	static const LLCachedControl<std::string> short_time_fmt(gSavedSettings, "ShortTimeFormat");
	std::string t;
	timeStructToFormattedString(internal_time, short_time_fmt, t);
	if (local_time)
	{
		static const std::string local(" " + getString("Local"));
		t += local;
	}
	else if (gPacificDaylightTime)
	{
		t += " PDT";
	}
	else
	{
		t += " PST";
	}
	if (t != mClockText)
	{
		mClockText = t;
		mTextTime->setText(t);
		mLayoutDirty = true;
	}
	static const LLCachedControl<std::string> long_date_fmt(gSavedSettings, "LongDateFormat");
	std::string date;
	timeStructToFormattedString(internal_time, long_date_fmt, date);
	if (date != mClockDate)
	{
		mClockDate = date;
		mTextTime->setToolTip(date);
	}
}
void LLStatusBar::refresh()
{
	if(gDisconnected)
	return;
	if (!mBalanceKnown && !mBalanceAutoRequested && mBalanceAutoAt > 0.0
		&& LLFrameTimer::getTotalSeconds() >= mBalanceAutoAt && mTextBalance)
	{
		std::string label = mTextBalance->getText();
		LLStringUtil::trim(label);
		if (label == mBalanceLoadingText)
		{
			mBalanceAutoRequested = true;
			sendMoneyBalanceRequest(true);
		}
	}
	F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1000.f;
	if (gViewerThrottle.isUnlimited())
	{
		F32 used = LLViewerStats::getInstance()->mKBitStat.getCurrentPerSec();
		bwtotal = llmax(used, 10000.f);
	}
	mSGBandwidth->setMin(0.f);
	mSGBandwidth->setMax(bwtotal*1.25f);
	mSGBandwidth->setThreshold(0, bwtotal*0.75f);
	mSGBandwidth->setThreshold(1, bwtotal);
	mSGBandwidth->setThreshold(2, bwtotal);
	updateClock();
	updateFPS();
	fitMenuBar();
	const S32 menu_right = gMenuBarView->getRightmostMenuEdge();
	if (menu_right != gMenuBarView->getRect().getWidth())
	{
		gMenuBarView->reshape(menu_right, gMenuBarView->getRect().getHeight());
	}
	if (gNavigationBar && gNavigationBar->isCompact() && menu_right != mMenuRight)
	{
		updateBarRect();
	}
	LLViewerRegion *region = gAgent.getRegion();
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	U32 icon_mask = 0;
	if (LLHUDIcon::iconsNearby())
	{
		icon_mask |= STATUS_ICON_SCRIPT;
	}
	if ((region && region->getAllowDamage()) ||
		(parcel && parcel->getAllowDamage()) )
	{
		icon_mask |= STATUS_ICON_HEALTH;
	}
	if ((region && region->getBlockFly()) ||
		(parcel && !parcel->getAllowFly()) )
	{
		icon_mask |= STATUS_ICON_NO_FLY;
	}
	if (parcel && !parcel->getAllowModify())
	{
		icon_mask |= STATUS_ICON_NO_BUILD;
	}
	if ((region
		&& (region->getRegionFlag(REGION_FLAGS_SKIP_SCRIPTS)
		|| region->getRegionFlag(REGION_FLAGS_ESTATE_SKIP_SCRIPTS)))
		|| (parcel && !parcel->getAllowOtherScripts()))
	{
		icon_mask |= STATUS_ICON_NO_SCRIPTS;
	}
	if ((region && region->getRestrictPushObject()) || (parcel && parcel->getRestrictPushObject()))
	{
		icon_mask |= STATUS_ICON_NO_PUSH;
	}
	if (!(parcel && parcel->getParcelFlagAllowVoice()))
	{
		icon_mask |= STATUS_ICON_NO_VOICE;
	}
	if (parcel && !parcel->getSeeAVs())
	{
		icon_mask |= STATUS_ICON_SEE_AV;
	}
	if (region)
	{
		if (!region->dynamicPathfindingEnabled())
		{
			icon_mask |= STATUS_ICON_PF_DISABLED;
		}
		else if (mIsNavMeshDirty)
		{
			icon_mask |= STATUS_ICON_PF_DIRTY;
		}
	}
	if (parcel && !parcel->isPublic() && LLViewerParcelMgr::getInstance()->canAgentBuyParcel(parcel, false))
	{
		icon_mask |= STATUS_ICON_BUY_LAND;
	}
	const bool mask_changed = icon_mask != mIconMask;
	const bool relayout = mask_changed
		|| menu_right != mMenuRight
		|| getRect().getWidth() != mLayoutWidth
		|| mLayoutDirty;
	S32 parcel_w = mCompactParcelW;
	S32 account_w = mCompactAccountW;
	if (mParcelHost)
	{
		parcel_w = layoutCompactParcelIcons(0, 0, 0, false);
	}
	if (mCompactHost)
	{
		account_w = layoutCompactAccount(0, 0, 0, false);
	}
	if (relayout)
	{
		layoutBar(icon_mask, menu_right);
	}
	if (mParcelHost && gNavigationBar
		&& (mask_changed || parcel_w != mCompactParcelW || (mCompactHost && account_w != mCompactAccountW)))
	{
		mCompactParcelW = parcel_w;
		mCompactAccountW = account_w;
		gNavigationBar->layoutCompactNow();
	}
	mHealthV->setVisible((icon_mask & STATUS_ICON_HEALTH)
		&& (mHealthTimer->hasExpired() || S32(mHealthTimer->getElapsedSeconds() * ICON_FLASH_FREQUENCY) & 1));
}
static void place_hosted(LLView* view, S32 x, S32 bottom, S32 row_h, S32 width, S32 height)
{
	const S32 y = bottom + llmax(0, (row_h - height) / 2);
	LLRect r;
	r.setOriginAndSize(x, y, width, height);
	view->setRect(r);
	view->setVisible(TRUE);
}
void LLStatusBar::moveHosted(LLPanel*& slot, LLPanel* host, LLView* const* widgets, S32 count)
{
	if (host == slot)
	{
		return;
	}
	if (slot)
	{
		for (S32 i = 0; i < count; ++i)
		{
			LLView* widget = widgets[i];
			if (!widget)
			{
				continue;
			}
			slot->removeChild(widget);
			addChild(widget);
			widget->setFollows(FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
		}
	}
	slot = host;
	if (slot)
	{
		for (S32 i = 0; i < count; ++i)
		{
			LLView* widget = widgets[i];
			if (!widget)
			{
				continue;
			}
			removeChild(widget);
			slot->addChild(widget);
			widget->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
		}
	}
	mLayoutDirty = true;
}
void LLStatusBar::setParcelHost(LLPanel* host)
{
	LLView* widgets[] = {
		mScriptOut, mHealthV, mTextHealth, mNoFly, mNoBuild, mNoScripts, mRestrictPush,
		mStatusNoVoice, mStatusSeeAV, mPFDirty, mPFDisabled, mBuyLand
	};
	if (host == mParcelHost)
	{
		return;
	}
	moveHosted(mParcelHost, host, widgets, sizeof(widgets) / sizeof(widgets[0]));
	mCompactParcelW = -1;
}
void LLStatusBar::setCompactHost(LLPanel* host)
{
	LLView* widgets[] = {
		mTextTime, mTextBalance, mTextUPC, mBuyCurrency, mTextFPS, mStatBtn
	};
	if (host == mCompactHost)
	{
		return;
	}
	moveHosted(mCompactHost, host, widgets, sizeof(widgets) / sizeof(widgets[0]));
	mCompactAccountW = -1;
}
void LLStatusBar::fitMenuBar()
{
	if (!gMenuBarView)
	{
		return;
	}
	const bool compact = gNavigationBar && gNavigationBar->isCompact();
	const S32 account = (compact || mMouselookHidden) ? 0 : layoutCompactAccount(0, 0, 0, false);
	const S32 chrome = STATUS_HPAD * 2 + STATUS_GROUP_GAP;
	const S32 room = llmax(0, getRect().getWidth() - account - chrome);
	const LLFontGL* font = LLFontGL::getFontSansSerif();
	S32 text = 0;
	S32 count = 0;
	const LLView::child_list_t* children = gMenuBarView->getChildList();
	for (LLView::child_list_const_iter_t it = children->begin(); it != children->end(); ++it)
	{
		LLMenuItemGL* item = dynamic_cast<LLMenuItemGL*>(*it);
		if (!item || !item->getVisible())
		{
			continue;
		}
		text += font->getWidth(item->getLabel());
		++count;
	}
	const S32 min_pad = 10;
	scaleMenuBar(compact);
	S32 pad = 20;
	if (count > 0)
	{
		while (pad > min_pad && text + count * pad > room)
		{
			--pad;
		}
	}
	gMenuBarView->setTitleSidePad(pad);
}
S32 LLStatusBar::layoutCompactParcelIcons(S32 right, S32 bottom, S32 row_h, bool place)
{
	struct Item
	{
		LLView* view;
		bool show;
	};
	const Item items[] = {
		{ mScriptOut, (mIconMask & STATUS_ICON_SCRIPT) != 0 },
		{ mHealthV, (mIconMask & STATUS_ICON_HEALTH) != 0 },
		{ mTextHealth, (mIconMask & STATUS_ICON_HEALTH) != 0 },
		{ mNoFly, (mIconMask & STATUS_ICON_NO_FLY) != 0 },
		{ mNoBuild, (mIconMask & STATUS_ICON_NO_BUILD) != 0 },
		{ mNoScripts, (mIconMask & STATUS_ICON_NO_SCRIPTS) != 0 },
		{ mRestrictPush, (mIconMask & STATUS_ICON_NO_PUSH) != 0 },
		{ mStatusNoVoice, (mIconMask & STATUS_ICON_NO_VOICE) != 0 },
		{ mStatusSeeAV, (mIconMask & STATUS_ICON_SEE_AV) != 0 },
		{ mPFDirty, (mIconMask & STATUS_ICON_PF_DIRTY) != 0 },
		{ mPFDisabled, (mIconMask & STATUS_ICON_PF_DISABLED) != 0 },
		{ mBuyLand, (mIconMask & STATUS_ICON_BUY_LAND) != 0 }
	};
	S32 width = 0;
	for (const Item& item : items)
	{
		if (!item.view)
		{
			continue;
		}
		if (!item.show)
		{
			if (place)
			{
				item.view->setVisible(FALSE);
			}
			continue;
		}
		if (width > 0)
		{
			width += 2;
		}
		S32 item_w = item.view->getRect().getWidth();
		if (item.view == mTextHealth)
		{
			item_w = S32(LLFontGL::getFontSansSerifSmall()->getWidth(std::string("100%")));
		}
		if (place)
		{
			place_hosted(item.view, right - width - item_w, bottom, row_h, item_w, item.view->getRect().getHeight());
		}
		width += item_w;
	}
	return width;
}
S32 LLStatusBar::layoutCompactAccount(S32 left, S32 bottom, S32 row_h, bool place)
{
	if (mMouselookHidden)
	{
		if (place)
		{
			mTextTime->setVisible(FALSE);
			mTextBalance->setVisible(FALSE);
			mTextFPS->setVisible(FALSE);
			mTextUPC->setVisible(FALSE);
			mBuyCurrency->setVisible(FALSE);
			mStatBtn->setVisible(FALSE);
		}
		return 0;
	}
	const S32 time_w = llmax(32, mTextTime->getTextPixelWidth() + 6);
	const S32 balance_w = llmax(32, mTextBalance->getTextPixelWidth() + 6);
	const S32 fps_w = mFPSWidth;
	const S32 buy_w = mBuyCurrency->getRect().getWidth();
	const S32 upc_w = mUPCSupported ? llmax(32, mTextUPC->getTextPixelWidth() + 6) : 0;
	const S32 stat_w = mStatBtn->getRect().getWidth();
	S32 x = left;
	if (place)
	{
		place_hosted(mTextTime, x, bottom, row_h, time_w, TEXT_HEIGHT);
		x += time_w + STATUS_BUY_GAP;
		place_hosted(mTextBalance, x, bottom, row_h, balance_w, TEXT_HEIGHT);
		x += balance_w + STATUS_BUY_GAP;
		place_hosted(mBuyCurrency, x, bottom, row_h, buy_w, mBuyCurrency->getRect().getHeight());
		x += buy_w + STATUS_BUY_GAP;
		if (mUPCSupported)
		{
			place_hosted(mTextUPC, x, bottom, row_h, upc_w, TEXT_HEIGHT);
			x += upc_w + STATUS_BUY_GAP;
		}
		else
		{
			mTextUPC->setVisible(FALSE);
		}
		place_hosted(mTextFPS, x, bottom, row_h, fps_w, TEXT_HEIGHT);
		x += fps_w + STATUS_BUY_GAP;
		place_hosted(mStatBtn, x, bottom, row_h, stat_w, mStatBtn->getRect().getHeight());
	}
	S32 width = time_w + STATUS_BUY_GAP + balance_w + STATUS_BUY_GAP + buy_w + STATUS_BUY_GAP + fps_w + STATUS_BUY_GAP + stat_w;
	if (mUPCSupported)
	{
		width += upc_w + STATUS_BUY_GAP;
	}
	return width;
}
void LLStatusBar::layoutBar(U32 icon_mask, S32 menu_right)
{
	mIconMask = icon_mask;
	mMenuRight = menu_right;
	mLayoutWidth = getRect().getWidth();
	mLayoutDirty = false;
	if (mCompactHost)
	{
		mLeftEnd = STATUS_HPAD + menu_right;
		mTextParcelName->setVisible(false);
		mSearchEditor->setVisible(false);
		mSearchBtn->setVisible(false);
		mSearchBevel->setVisible(false);
		mSGBandwidth->setVisible(false);
		mSGPacketLoss->setVisible(false);
		return;
	}
	LLRect r;
	const S32 y = STATUS_VPAD;
	mLeftEnd = STATUS_HPAD + menu_right;
	mTextParcelName->setVisible(false);
	mSearchEditor->setVisible(false);
	mSearchBtn->setVisible(false);
	mSearchBevel->setVisible(false);
	const S32 stat_width = mStatBtn->getRect().getWidth();
	const S32 buy_width = mBuyCurrency->getRect().getWidth();
	const S32 balance_width = mTextBalance->getTextPixelWidth() + 2;
	const S32 upc_width = mTextUPC->getTextPixelWidth() + 2;
	const S32 time_width = mTextTime->getTextPixelWidth() + 2;
	const S32 fps_width = mFPSWidth;
	const S32 need_balance = balance_width + STATUS_BUY_GAP + buy_width;
	const S32 need_upc = upc_width + STATUS_GROUP_GAP;
	const S32 need_time = time_width + STATUS_GROUP_GAP;
	const S32 need_fps = fps_width + STATUS_GROUP_GAP;
	const S32 need_stat = stat_width + STATUS_GROUP_GAP;
	S32 right = getRect().getWidth() - STATUS_HPAD;
	S32 available = right - mLeftEnd - STATUS_GROUP_GAP;
	const bool normal = !mMouselookHidden;
	const bool show_balance = normal;
	const bool show_time = normal;
	bool show_upc = false;
	bool show_stat = false;
	if (normal && mUPCSupported)
	{
		show_upc = true;
		available -= need_upc;
	}
	if (normal)
	{
		available -= need_balance + need_time + need_fps;
	}
	if (normal && available >= need_stat)
	{
		show_stat = true;
	}
	mStatBtn->setVisible(show_stat);
	mStatBtn->setEnabled(show_stat);
	mSGBandwidth->setVisible(show_stat);
	mSGPacketLoss->setVisible(show_stat);
	if (show_stat)
	{
		r.setOriginAndSize(right - stat_width, y + 2, stat_width, mStatBtn->getRect().getHeight());
		mStatBtn->setRect(r);
		S32 graph_right = right - 2;
		r.set(graph_right - SIM_STAT_WIDTH, y + MENU_BAR_HEIGHT - 1, graph_right, y + 1);
		mSGBandwidth->setRect(r);
		graph_right -= SIM_STAT_WIDTH + 2;
		r.set(graph_right - SIM_STAT_WIDTH, y + MENU_BAR_HEIGHT - 1, graph_right, y + 1);
		mSGPacketLoss->setRect(r);
		right -= need_stat;
	}
	mTextFPS->setVisible(normal);
	if (normal)
	{
		r.setOriginAndSize(right - fps_width, y - 2, fps_width, TEXT_HEIGHT);
		mTextFPS->setRect(r);
		right -= need_fps;
	}
	mTextUPC->setVisible(show_upc);
	if (show_upc)
	{
		r.setOriginAndSize(right - upc_width, y - 2, upc_width, TEXT_HEIGHT);
		mTextUPC->setRect(r);
		right -= need_upc;
	}
	mTextBalance->setVisible(show_balance);
	mBuyCurrency->setVisible(show_balance);
	if (show_balance)
	{
		r.setOriginAndSize(right - balance_width, y - 2, balance_width, TEXT_HEIGHT);
		mTextBalance->setRect(r);
		right -= balance_width + STATUS_BUY_GAP;
		r.setOriginAndSize(right - buy_width, y + 1, buy_width, mBuyCurrency->getRect().getHeight());
		mBuyCurrency->setRect(r);
		right -= buy_width + STATUS_GROUP_GAP;
	}
	mTextTime->setVisible(show_time);
	if (show_time)
	{
		r.setOriginAndSize(right - time_width, y - 2, time_width, TEXT_HEIGHT);
		mTextTime->setRect(r);
	}
}
void LLStatusBar::updateFPS()
{
	if (!mTextFPS)
	{
		return;
	}
	F32 raw = LLViewerStats::getInstance()->mFPSStat.getMeanPerSec();
	if (!(raw >= 0.f))
	{
		raw = 0.f;
	}
	const F32 dt = llclamp(LLFrameTimer::getFrameDeltaTimeF32(), 0.f, 0.1f);
	if (mSmoothFPS < 0.f)
	{
		mSmoothFPS = raw;
	}
	else
	{
		const F32 blend = 1.f - expf(-dt * 1.4f);
		mSmoothFPS += (raw - mSmoothFPS) * blend;
	}
	mFPSRefresh += dt;
	const S32 shown = ll_round(mSmoothFPS);
	if (mLastFPS < 0 || (shown != mLastFPS && mFPSRefresh >= 0.6f))
	{
		mFPSRefresh = 0.f;
		mLastFPS = shown;
		mTextFPS->setText(llformat("FPS: %d", mLastFPS));
	}
	LLColor4 color;
	if (mLastFPS <= 30)
	{
		color.set(1.f, 0.25f, 0.22f, 1.f);
	}
	else if (mLastFPS <= 60)
	{
		color.set(1.f, 0.86f, 0.2f, 1.f);
	}
	else if (mLastFPS <= 100)
	{
		color.set(0.3f, 0.92f, 0.38f, 1.f);
	}
	else
	{
		const F32 t = (F32)LLFrameTimer::getElapsedSeconds();
		const F32 wave = 0.5f + 0.5f * sinf(t * 2.2f);
		const F32 glint = powf(llmax(0.f, sinf(t * 4.5f)), 12.f);
		color.set(0.32f + 0.28f * wave + 0.12f * glint, 0.9f, 0.4f + 0.22f * wave + 0.12f * glint, 1.f);
	}
	mTextFPS->setColor(color);
}
void LLStatusBar::setVisibleForMouselook(bool visible)
{
	mMouselookHidden = !visible;
	setMouseOpaque(visible);
	mLayoutDirty = true;
}
void LLStatusBar::debitBalance(S32 debit)
{
	setBalance(getBalance() - debit);
}
void LLStatusBar::creditBalance(S32 credit)
{
	setBalance(getBalance() + credit);
}
void LLStatusBar::setBalance(S32 balance)
{
	mBalanceKnown = true;
	mTextBalance->setText(gHippoGridManager->getConnectedGrid()->getCurrencySymbol().c_str() +
		LLResMgr::getInstance()->getMonetaryString(balance - mUPC));
	mLayoutDirty = true;
	if (mBalance && (fabs((F32)(mBalance - balance)) > gSavedSettings.getF32("UISndMoneyChangeThreshold")))
	{
		if (mBalance > balance)
			make_ui_sound("UISndMoneyChangeDown");
		else
			make_ui_sound("UISndMoneyChangeUp");
	}
	if( balance != mBalance )
	{
		mBalanceTimer->reset(ICON_TIMER_EXPIRY);
		mBalance = balance;
	}
}
void LLStatusBar::setUPC(S32 upc)
{
	mTextUPC->setText("UPC " + LLResMgr::getInstance()->getMonetaryString(upc));
	mLayoutDirty = true;
	mUPC = upc;
	setBalance(mBalance);
}
void LLStatusBar::sendMoneyBalanceRequest(bool from_user)
{
	void cmdline_printchat(const std::string& message);
	if (from_user) cmdline_printchat(LLTrans::getString("refreshing balance"));
	if (!from_user && gStatusBar && !gStatusBar->mBalanceKnown && gStatusBar->mBalanceAutoAt <= 0.0)
	{
		gStatusBar->mBalanceAutoAt = LLFrameTimer::getTotalSeconds() + 120.0;
	}
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );
	gAgent.sendReliableMessage();
}
void LLStatusBar::setHealth(S32 health)
{
	mTextHealth->setText(llformat("%d%%", health));
	if( mHealth > health )
	{
		if (mHealth > (health + gSavedSettings.getF32("UISndHealthReductionThreshold")))
		{
			if (isAgentAvatarValid())
			{
				if (gAgentAvatarp->getSex() == SEX_FEMALE)
				{
					make_ui_sound("UISndHealthReductionF");
				}
				else
				{
					make_ui_sound("UISndHealthReductionM");
				}
			}
		}
		mHealthTimer->reset(ICON_TIMER_EXPIRY);
	}
	mHealth = health;
}
S32 LLStatusBar::getBalance() const
{
	return mBalance;
}
S32 LLStatusBar::getHealth() const
{
	return mHealth;
}
void LLStatusBar::setLandCredit(S32 credit)
{
	mSquareMetersCredit = credit;
}
void LLStatusBar::setLandCommitted(S32 committed)
{
	mSquareMetersCommitted = committed;
}
BOOL LLStatusBar::isUserTiered() const
{
	return (mSquareMetersCredit > 0);
}
S32 LLStatusBar::getSquareMetersCredit() const
{
	return mSquareMetersCredit;
}
S32 LLStatusBar::getSquareMetersCommitted() const
{
	return mSquareMetersCommitted;
}
S32 LLStatusBar::getSquareMetersLeft() const
{
	return mSquareMetersCredit - mSquareMetersCommitted;
}
static void onClickParcelInfo()
{
	LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());
	show_floater("about land");
}
static bool rebakeRegionCallback(const LLSD& n, const LLSD& r)
{
	if(!LLNotificationsUtil::getSelectedOption(n, r))
	{
		LLMenuOptionPathfindingRebakeNavmesh::getInstance()->sendRequestRebakeNavmesh();
		return true;
	}
	return false;
}
void LLStatusBar::createNavMeshStatusListenerForCurrentRegion()
{
	if (mNavMeshSlot.connected())
	{
		mNavMeshSlot.disconnect();
	}
	LLViewerRegion *currentRegion = gAgent.getRegion();
	if (currentRegion != NULL)
	{
		mNavMeshSlot = LLPathfindingManager::getInstance()->registerNavMeshListenerForRegion(currentRegion, boost::bind(&LLStatusBar::onNavMeshStatusChange, this, _2));
		LLPathfindingManager::getInstance()->requestGetNavMeshForRegion(currentRegion, true);
	}
}
void LLStatusBar::onNavMeshStatusChange(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	mIsNavMeshDirty = pNavMeshStatus.isValid() && (pNavMeshStatus.getStatus() != LLPathfindingNavMeshStatus::kComplete);
	refresh();
}
static void onClickScripts()
{
	LLViewerRegion* region = gAgent.getRegion();
	if(region && region->getRegionFlag(REGION_FLAGS_ESTATE_SKIP_SCRIPTS))
	{
		LLNotificationsUtil::add("ScriptsStopped");
	}
	else if(region && region->getRegionFlag(REGION_FLAGS_SKIP_SCRIPTS))
	{
		LLNotificationsUtil::add("ScriptsNotRunning");
	}
	else
	{
		LLNotificationsUtil::add("NoOutsideScripts");
	}
}
void LLStatusBar::setupDate()
{
	std::string day_list = getString("StatBarDaysOfWeek");
	size_t length = day_list.size();
	if(length < MAX_DATE_STRING_LENGTH)
	{
		std::string cur_word;
		for(size_t i = 0; i < length; ++i)
		{
			if(day_list[i] == ':')
			{
				sDays.push_back(cur_word);
				cur_word.clear();
			}
			else
			{
				cur_word.append(1, day_list[i]);
			}
		}
		sDays.push_back(cur_word);
	}
	std::string month_list = getString( "StatBarMonthsOfYear" );
	length = month_list.size();
	if(length < MAX_DATE_STRING_LENGTH)
	{
		std::string cur_word;
		for(size_t i = 0; i < length; ++i)
		{
			if(month_list[i] == ':')
			{
				sMonths.push_back(cur_word);
				cur_word.clear();
			}
			else
			{
				cur_word.append(1, month_list[i]);
			}
		}
		sMonths.push_back(cur_word);
	}
	if(sDays.size() < 7)
	{
		sDays.resize(7);
	}
	if(sMonths.size() < 12)
	{
		sMonths.resize(12);
	}
}
BOOL can_afford_transaction(S32 cost)
{
	return((cost <= 0)||((gStatusBar) && (gStatusBar->getBalance() >=cost)));
}
class LLBalanceHandler : public LLCommandHandler
{
public:
	LLBalanceHandler() : LLCommandHandler("balance", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() == 1
			&& tokens[0].asString() == "request")
		{
			LLStatusBar::sendMoneyBalanceRequest(true);
			return true;
		}
		return false;
	}
};
LLBalanceHandler gBalanceHandler;
