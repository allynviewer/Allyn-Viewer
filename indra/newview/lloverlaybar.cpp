/** 
 * @file lloverlaybar.cpp
 * @brief LLOverlayBar class implementation
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
#include "lloverlaybar.h"
#include "llaudioengine.h"
#include "floaterao.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llchatbar.h"
#include "llfloaterchatterbox.h"
#include "llmediaremotectrl.h"
#include "llpanelaudiovolume.h"
#include "llparcel.h"
#include "llviewertexturelist.h"
#include "llviewerjoystick.h"
#include "llviewermedia.h"
#include "llviewermenu.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "lluictrlfactory.h"
#include "llui.h"
#include "llrender2dutils.h"
#include "llvoiceclient.h"
#include "llvoavatarself.h"
#include "llvoiceremotectrl.h"
#include "llselectmgr.h"
#include "lltoolbar.h"
#include "wlfPanel_AdvSettings.h"
#include "llpanelnearbymedia.h"
#include "rlvhandler.h"
#include <boost/range/adaptor/reversed.hpp>
LLOverlayBar *gOverlayBar = NULL;
static const S32 OVERLAY_BUTTON_HEIGHT = 29;
static const S32 OVERLAY_ICON_BUTTON_WIDTH = 30;
static const S32 OVERLAY_ICON_PANEL_WIDTH = 34;
static const S32 OVERLAY_PILL_PAD = 3;
static const S32 OVERLAY_PILL_CAP = 8;
static const S32 OVERLAY_MOUSELOOK_SIZE = 30;
static const S32 OVERLAY_BESIDE_MAX_ICONS = 8;
static const S32 OVERLAY_BESIDE_MIN_WIDTH = 600;
void* LLOverlayBar::createVoiceRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;
	self->mVoiceRemote = new LLVoiceRemoteCtrl(std::string("voice_remote"));
	return self->mVoiceRemote;
}
void* LLOverlayBar::createChatBar(void* userdata)
{
	gChatBar = new LLChatBar();
	return gChatBar;
}
LLOverlayBar::LLOverlayBar()
	:	LLLayoutPanel(),
		mMediaRemote(NULL),
		mVoiceRemote(NULL),
		mAOButton(NULL),
		mVolumeButton(NULL),
		mEnvButton(NULL),
		mOverlayStack(NULL),
		mStackLift(0),
		mStackNaturalHeight(0),
		mBesideHoldWidth(-1),
		mBesideHoldIcons(-1),
		mBesideHoldCluster(-1),
		mAOFloaterWasOpen(false),
		mVolumeFloaterWasOpen(false),
		mEnvFloaterWasOpen(false),
		mVolumeMutedIcon(false),
		mVolumeIconReady(false),
		mMusicState(STOPPED),
		mOriginalIMLabel("")
{
	gOverlayBar = this;
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);
	mBuilt = false;
	LLCallbackMap::map_t factory_map;
	factory_map["voice_remote"] = LLCallbackMap(LLOverlayBar::createVoiceRemote, this);
	factory_map["chat_bar"] = LLCallbackMap(LLOverlayBar::createChatBar, this);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_overlaybar.xml", &factory_map);
}
bool LLOverlayBar::updateAdvSettingsPopup(const LLSD &data)
{
	if (data.asBoolean())
	{
		LLFloaterOverlayEnv::getInstance()->open();
	}
	else if (LLFloaterOverlayEnv::instanceExists() && LLFloaterOverlayEnv::getInstance()->getVisible())
	{
		LLFloaterOverlayEnv::getInstance()->close();
	}
	return true;
}
bool LLOverlayBar::updateVolumePopup(const LLSD &data)
{
	if (!LLFloaterOverlayVolume::instanceExists())
		return true;
	LLFloaterOverlayVolume* floater = LLFloaterOverlayVolume::getInstance();
	if (data.asBoolean())
	{
		if (!floater->getVisible())
			floater->open();
	}
	else if (floater->getVisible())
	{
		floater->close();
	}
	return true;
}
void LLOverlayBar::onClickAO(void*)
{
	if (!gOverlayBar)
		return;
	const bool opening = !LLFloaterAO::instanceVisible();
	LLFloaterAO::toggleInstance();
	if (opening)
	{
		if (LLFloaterAO* floater = LLFloaterAO::findInstance())
			placeFloaterAbove(floater, gOverlayBar->mAOButton);
	}
}
void LLOverlayBar::placeFloaterAbove(LLFloater* floater, LLView* anchor)
{
	if (!floater || !gFloaterView)
		return;
	bool placed = false;
	const std::string& control = floater->getRectControl();
	if (!control.empty() && gSavedSettings.controlExists(control))
	{
		const LLRect saved = gSavedSettings.getRect(control);
		if (saved.getWidth() > 0 && saved.getHeight() > 0)
		{
			floater->applyRectControl();
			placed = true;
		}
	}
	if (!placed && anchor)
	{
		const LLRect anchor_rect = anchor->calcScreenRect();
		floater->setOrigin(anchor_rect.mRight - floater->getRect().getWidth(), anchor_rect.mTop);
	}
	gFloaterView->adjustToFitScreen(floater, FALSE);
}
void LLOverlayBar::fitFloaterToPanel(LLFloater* floater, LLView* panel)
{
	if (!floater || !panel)
		return;
	const LLRect local = panel->getLocalBoundingRect();
	const S32 pad = LLFLOATER_CONTENT_PAD;
	const S32 panel_w = llmax(panel->getRect().getWidth(), local.getWidth());
	const S32 panel_h = llmax(panel->getRect().getHeight(), local.getHeight());
	panel->setOrigin(pad, pad);
	floater->reshape(panel_w + pad * 2, panel_h + LLFLOATER_HEADER_SIZE + pad + 4);
	floater->setResizeLimits(floater->getRect().getWidth(), floater->getRect().getHeight());
}
static void stashOverlayFloater(LLFloater* floater, bool& was_open)
{
	if (floater && floater->getVisible())
	{
		was_open = true;
		floater->setVisible(FALSE);
	}
}
void LLOverlayBar::stashOverlayFloaters(bool in_mouselook)
{
	if (in_mouselook)
	{
		stashOverlayFloater(LLFloaterAO::findInstance(), mAOFloaterWasOpen);
		if (LLFloaterOverlayVolume::instanceExists())
			stashOverlayFloater(LLFloaterOverlayVolume::getInstance(), mVolumeFloaterWasOpen);
		if (LLFloaterOverlayEnv::instanceExists())
			stashOverlayFloater(LLFloaterOverlayEnv::getInstance(), mEnvFloaterWasOpen);
		return;
	}
	if (mAOFloaterWasOpen)
	{
		if (LLFloaterAO* floater = LLFloaterAO::findInstance())
			floater->setVisible(TRUE);
		mAOFloaterWasOpen = false;
	}
	if (mVolumeFloaterWasOpen)
	{
		if (gSavedSettings.getBOOL("ShowVolumeSettingsPopup") && LLFloaterOverlayVolume::instanceExists())
			LLFloaterOverlayVolume::getInstance()->setVisible(TRUE);
		mVolumeFloaterWasOpen = false;
	}
	if (mEnvFloaterWasOpen)
	{
		if (gSavedSettings.getBOOL("wlfAdvSettingsPopup") && LLFloaterOverlayEnv::instanceExists())
			LLFloaterOverlayEnv::getInstance()->setVisible(TRUE);
		mEnvFloaterWasOpen = false;
	}
}
bool LLOverlayBar::updateChatVisible(const LLSD &data)
{
	mChatBar->getParent()->setVisible(data.asBoolean());
	return true;
}
bool LLOverlayBar::updateAORemoteVisible(const LLSD &data)
{
	mAORemoteContainer->setVisible(data.asBoolean());
	mAOLightContainer->setVisible(data.asBoolean());
	return true;
}
bool updateNearbyMediaFloater(const LLSD &data)
{
	LLFloaterNearbyMedia::updateClass();
	return true;
}
BOOL LLOverlayBar::postBuild()
{
	childSetAction("New IM",onClickIMReceived,this);
	childSetAction("Set Not Busy",onClickSetNotBusy,this);
	childSetAction("Mouselook",onClickMouselook,this);
	childSetAction("Stand Up",onClickStandUp,this);
	childSetAction("Cancel TP",onClickCancelTP,this);
 	childSetAction("Flycam",onClickFlycam,this);
	mCancelBtn = getChild<LLButton>("Cancel TP");
	setFocusRoot(TRUE);
	mBuilt = true;
	mUnreadCountStringPlural = getString("unread_count_string_plural");
	mChatbarAndButtons.connect(this,"chatbar_and_buttons");
	mNewIM.connect(this,"New IM");
	mNotBusy.connect(this,"Set Not Busy");
	mMouseLook.connect(this,"Mouselook");
	mStandUp.connect(this,"Stand Up");
	mFlyCam.connect(this,"Flycam");
	mChatBar.connect(this,"chat_bar");
	mVoiceRemoteContainer.connect(this,"voice_remote_container");
	mStateManagementContainer.connect(this,"state_management_buttons_container");
	mAORemoteContainer.connect(this,"ao_remote_container");
	mAOLightContainer.connect(this,"ao_light_container");
	mAdvSettingsContainer.connect(this,"AdvSettings_container");
	mMediaRemoteContainer.connect(this,"media_remote_container");
	mAOButton = getChild<LLButton>("overlay_ao_btn");
	mVolumeButton = getChild<LLButton>("overlay_volume_btn");
	mEnvButton = getChild<LLButton>("overlay_env_btn");
	childSetAction("overlay_ao_btn", onClickAO, this);
	LLFloaterOverlayVolume::getInstance();
	mMediaRemote = LLFloaterOverlayVolume::getInstance()->getRemote();
	updateAdvSettingsPopup(gSavedSettings.getBOOL("wlfAdvSettingsPopup"));
	updateVolumePopup(gSavedSettings.getBOOL("ShowVolumeSettingsPopup"));
	updateChatVisible(gSavedSettings.getBOOL("ChatVisible"));
	updateAORemoteVisible(gSavedSettings.getBOOL("EnableAORemote"));
	mOriginalIMLabel = mNewIM->getLabelSelected();
	if (LLView* parent = mNewIM->getParent())
	{
		if (parent != this)
		{
			parent->removeChild(mNewIM);
			addChild(mNewIM);
		}
	}
	LLFloatingBarStyle::styleIconButton(mNewIM);
	mNewIM->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	mOverlayStack = findChild<LLLayoutStack>("overlay_layout_panel");
	styleOverlayButton(mAOButton, mAORemoteContainer, "floating_ui/ao.png", true);
	styleOverlayButton(mVolumeButton, mMediaRemoteContainer, "floating_ui/volume.png", true);
	styleOverlayButton(mEnvButton, mAdvSettingsContainer, "floating_ui/environment.png", true);
	styleOverlayButton(mMouseLook, NULL, "floating_ui/mouselook.png", true);
	mMarginSlot = LLFloatingBarStyle::connectSetting("FloatingBarMargin", boost::bind(&LLOverlayBar::layoutOverlayStack, this));
	layoutOverlayStack();
	if (mVoiceRemote)
		mVoiceRemote->layoutControls();
	layoutButtons();
	sendChildToFront(mMouseLook);
	gSavedSettings.getControl("wlfAdvSettingsPopup")->getSignal()->connect(boost::bind(&LLOverlayBar::updateAdvSettingsPopup,this,_2));
	gSavedSettings.getControl("ShowVolumeSettingsPopup")->getSignal()->connect(boost::bind(&LLOverlayBar::updateVolumePopup,this,_2));
	gSavedSettings.getControl("ChatVisible")->getSignal()->connect(boost::bind(&LLOverlayBar::updateChatVisible,this,_2));
	gSavedSettings.getControl("EnableAORemote")->getSignal()->connect(boost::bind(&LLOverlayBar::updateAORemoteVisible,this,_2));
	gSavedSettings.getControl("ShowNearbyMediaFloater")->getSignal()->connect(boost::bind(&updateNearbyMediaFloater,_2));
	mAORemoteContainer->setVisible(gSavedSettings.getBOOL("EnableAORemote"));
	mAOLightContainer->setVisible(gSavedSettings.getBOOL("EnableAORemote"));
	return TRUE;
}
LLOverlayBar::~LLOverlayBar()
{
	mMarginSlot.disconnect();
}
void LLOverlayBar::styleOverlayButton(LLButton* button, LLView* container, const std::string& icon, bool label_to_tip)
{
	if (!button)
		return;
	if (label_to_tip)
		LLFloatingBarStyle::labelToToolTip(button);
	else
		button->setLabel(LLStringExplicit(""));
	LLFloatingBarStyle::styleIconButton(button);
	button->setImageOverlay(icon, LLFontGL::HCENTER, LLColor4::white);
	button->setImageOverlaySelectedColor(LLFloatingBarStyle::accentColor());
	button->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	if (LLLayoutPanel* panel = dynamic_cast<LLLayoutPanel*>(container))
	{
		panel->setMinDim(OVERLAY_ICON_PANEL_WIDTH);
		panel->reshape(OVERLAY_ICON_PANEL_WIDTH, panel->getRect().getHeight());
		const S32 left = (OVERLAY_ICON_PANEL_WIDTH - OVERLAY_ICON_BUTTON_WIDTH) / 2;
		button->setShape(LLRect(left, OVERLAY_BUTTON_HEIGHT, left + OVERLAY_ICON_BUTTON_WIDTH, 0));
	}
}
static void apply_stack_rect(LLView* stack, const LLRect& rect)
{
	if (stack && stack->getRect() != rect)
	{
		stack->setShape(rect);
	}
}
void LLOverlayBar::layoutOverlayStack()
{
	if (!mOverlayStack)
		return;
	const S32 margin = LLFloatingBarStyle::margin();
	const bool toolbar_visible = gToolBar && gToolBar->getVisible();
	mStackLift = toolbar_visible ? 0 : llmax(margin, OVERLAY_PILL_PAD);
	if (mStackNaturalHeight <= 0)
	{
		const S32 height = mOverlayStack->getRect().getHeight();
		if (height > 0)
		{
			mStackNaturalHeight = height;
		}
	}
	const S32 stack_h = mStackNaturalHeight > 0 ? mStackNaturalHeight : llmax(mOverlayStack->getRect().getHeight(), 1);
	const S32 width = getRect().getWidth();
	const S32 stack_left = margin;
	const S32 stack_right = llmax(stack_left, width - margin - OVERLAY_PILL_CAP);
	if (mOverlayStack->getRect().mLeft != stack_left || mOverlayStack->getRect().mRight != stack_right)
	{
		LLRect width_rect = mOverlayStack->getRect();
		width_rect.mLeft = stack_left;
		width_rect.mRight = stack_right;
		apply_stack_rect(mOverlayStack, width_rect);
	}
	mOverlayStack->updateLayout();
	LLView* remote_panels[5] = {
		mAOLightContainer, mAORemoteContainer, mVoiceRemoteContainer, mMediaRemoteContainer, mAdvSettingsContainer
	};
	S32 cluster_left = S32_MAX;
	S32 cluster_right = 0;
	const S32 placed_left = mOverlayStack->getRect().mLeft;
	for (LLView* panel : remote_panels)
	{
		if (!panel || !panel->getVisible() || panel->getRect().getWidth() <= 1)
		{
			continue;
		}
		const S32 left = placed_left + panel->getRect().mLeft;
		const S32 right = placed_left + panel->getRect().mRight;
		cluster_left = llmin(cluster_left, left);
		cluster_right = llmax(cluster_right, right);
	}
	const S32 cluster = cluster_right > cluster_left ? cluster_right - cluster_left : 0;
	const S32 icons = toolbar_visible ? gToolBar->getVisibleIconCount() : 0;
	bool beside = toolbar_visible && width >= OVERLAY_BESIDE_MIN_WIDTH && icons <= OVERLAY_BESIDE_MAX_ICONS && cluster > 0;
	if (gToolBar)
	{
		gToolBar->setCenterOverride(icons > OVERLAY_BESIDE_MAX_ICONS);
	}
	if (beside && mBesideHoldWidth == width && mBesideHoldIcons == icons && mBesideHoldCluster == cluster)
	{
		beside = false;
	}
	if (beside)
	{
		const S32 reserve = cluster + 2 * OVERLAY_PILL_CAP + LLFloatingBarStyle::BAR_GAP;
		if (gToolBar->getCompanionReserve() != reserve || gToolBar->isLayoutDirty())
		{
			gToolBar->setCompanionReserve(reserve);
			gToolBar->layoutNow();
		}
		S32 pill_right = 0;
		S32 pill_y = 0;
		gToolBar->localPointToOtherView(gToolBar->getPillRect().mRight, 0, &pill_right, &pill_y, this);
		const S32 secondary_left = cluster_left - OVERLAY_PILL_CAP;
		const bool pill_ready = gToolBar->getPillRect().getWidth() > 0;
		const bool fits = pill_ready && gToolBar->getRowCount() <= 1
			&& pill_right + LLFloatingBarStyle::BAR_GAP <= secondary_left;
		if (!fits)
		{
			beside = false;
			if (pill_ready)
			{
				mBesideHoldWidth = width;
				mBesideHoldIcons = icons;
				mBesideHoldCluster = cluster;
			}
		}
		else
		{
			mBesideHoldWidth = -1;
		}
	}
	if (gToolBar && !beside && gToolBar->getCompanionReserve() != 0)
	{
		gToolBar->setCompanionReserve(0);
		gToolBar->layoutNow();
	}
	if (gToolBar && gToolBar->isLayoutDirty())
	{
		gToolBar->layoutNow();
	}
	S32 target_bottom = mStackLift;
	if (beside)
	{
		const LLRect& pill = gToolBar->getPillRect();
		S32 unused = 0;
		S32 pill_bottom = 0;
		S32 pill_top = 0;
		gToolBar->localPointToOtherView(0, pill.mBottom, &unused, &pill_bottom, this);
		gToolBar->localPointToOtherView(0, pill.mTop, &unused, &pill_top, this);
		target_bottom = (pill_bottom + pill_top) / 2 - OVERLAY_BUTTON_HEIGHT / 2;
		if (target_bottom >= mStackLift)
		{
			beside = false;
			target_bottom = mStackLift;
			if (gToolBar->getCompanionReserve() != 0)
			{
				gToolBar->setCompanionReserve(0);
				gToolBar->layoutNow();
			}
		}
	}
	LLRect desired;
	desired.mLeft = stack_left;
	desired.mRight = stack_right;
	desired.mBottom = beside ? target_bottom : mStackLift;
	desired.mTop = mStackLift + stack_h;
	apply_stack_rect(mOverlayStack, desired);
	mOverlayStack->updateLayout();
	if (!beside)
	{
		return;
	}
	const S32 lift = mStackLift - mOverlayStack->getRect().mBottom;
	if (lift <= 0)
	{
		return;
	}
	for (S32 i = 0; i < mOverlayStack->getNumPanels(); ++i)
	{
		LLLayoutPanel* panel = mOverlayStack->getPanelByIndex(i);
		if (!panel || panel->getName() != "main_panel")
		{
			continue;
		}
		LLRect main_rect = panel->getRect();
		if (main_rect.mBottom != lift)
		{
			main_rect.translate(0, lift - main_rect.mBottom);
			panel->setRect(main_rect);
		}
		break;
	}
}
void LLOverlayBar::draw()
{
	layoutOverlayStack();
	layoutNewIM();
	layoutMouselookButton();
	S32 left = S32_MAX;
	S32 right = 0;
	S32 panel_bottom = 0;
	bool have_panel = false;
	const LLView* panels[5] = { mAOLightContainer, mAORemoteContainer, mVoiceRemoteContainer, mMediaRemoteContainer, mAdvSettingsContainer };
	for (const LLView* panel : panels)
	{
		if (!panel || !panel->getVisible() || panel->getRect().getWidth() <= 0)
			continue;
		LLRect screen = panel->getRect();
		if (const LLView* parent = panel->getParent())
		{
			screen.mLeft += parent->getRect().mLeft;
			screen.mRight += parent->getRect().mLeft;
			screen.mBottom += parent->getRect().mBottom;
		}
		left = llmin(left, screen.mLeft);
		right = llmax(right, screen.mRight);
		if (!have_panel || screen.mBottom < panel_bottom)
		{
			panel_bottom = screen.mBottom;
		}
		have_panel = true;
	}
	if (right > left && have_panel)
	{
		const S32 bottom = panel_bottom - OVERLAY_PILL_PAD;
		mStyle.drawPill(LLRect(left - OVERLAY_PILL_CAP, bottom + OVERLAY_BUTTON_HEIGHT + 2 * OVERLAY_PILL_PAD, right + OVERLAY_PILL_CAP - 2, bottom), LLFloatingBarStyle::barColor());
	}
	if (mMouseLook && mMouseLook->getVisible())
	{
		mStyle.drawPill(mMouseLook->getRect(), LLFloatingBarStyle::barColor());
	}
	if (mNewIM && mNewIM->getVisible())
	{
		mStyle.drawPill(mNewIMPill, LLFloatingBarStyle::barColor());
	}
	LLPanel::draw();
	if (mNewIM && mNewIM->getVisible())
	{
		LLUIImagePtr border = LLUI::getUIImage("Floating_Pill_Border");
		if (border.notNull())
		{
			const F32 wave = 0.5f + 0.5f * sinf((F32)LLFrameTimer::getElapsedSeconds() * 6.f);
			LLColor4 orange(1.f, 0.42f, 0.f, 0.2f + 0.8f * wave);
			border->draw(mNewIMPill, orange);
		}
	}
}
void LLOverlayBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	S32 delta_width = width - getRect().getWidth();
	S32 delta_height = height - getRect().getHeight();
	if (!delta_width && !delta_height && !sForceReshape)
		return;
	LLView::reshape(width, height, called_from_parent);
	if (mBuilt)
	{
		layoutOverlayStack();
		layoutButtons();
		layoutNewIM();
	}
}
void LLOverlayBar::layoutMouselookButton()
{
	if (!mMouseLook)
		return;
	const S32 width = OVERLAY_MOUSELOOK_SIZE;
	S32 right = getRect().getWidth() - LLFloatingBarStyle::margin();
	S32 bottom = mStackLift;
	bool found = false;
	const LLView* panels[5] = { mAOLightContainer, mAORemoteContainer, mVoiceRemoteContainer, mMediaRemoteContainer, mAdvSettingsContainer };
	for (const LLView* panel : panels)
	{
		if (!panel || !panel->getVisible() || panel->getRect().getWidth() <= 0)
		{
			continue;
		}
		LLRect anchor = panel->getRect();
		if (const LLView* parent = panel->getParent())
		{
			if (parent != this)
			{
				anchor.mLeft += parent->getRect().mLeft;
				anchor.mRight += parent->getRect().mLeft;
				anchor.mBottom += parent->getRect().mBottom;
			}
		}
		if (!found || anchor.mRight >= right)
		{
			found = true;
			right = anchor.mRight;
			bottom = anchor.mBottom;
		}
	}
	const S32 height = OVERLAY_MOUSELOOK_SIZE;
	const S32 y = bottom + OVERLAY_BUTTON_HEIGHT + OVERLAY_PILL_PAD + LLFloatingBarStyle::BAR_GAP;
	S32 left = right - width;
	if (left < 0)
		left = 0;
	const LLRect& cur = mMouseLook->getRect();
	if (cur.mLeft != left || cur.mBottom != y || cur.getWidth() != width || cur.getHeight() != height)
	{
		LLRect r;
		r.setOriginAndSize(left, y, width, height);
		mMouseLook->setShape(r);
	}
}
void LLOverlayBar::layoutButtons()
{
	if (mStateManagementContainer->getVisible())
	{
		U32 button_count = 0;
		const child_list_t& view_list = *(mStateManagementContainer->getChildList());
		for (LLView* viewp : view_list)
		{
			if(!viewp->getEnabled())
				continue;
			++button_count;
		}
		const S32 MAX_BAR_WIDTH = 600;
		S32 bar_width = llclamp(mStateManagementContainer->getRect().getWidth(), 0, MAX_BAR_WIDTH);
		const S32 MAX_BUTTON_WIDTH = 150;
		static LLCachedControl<S32> status_bar_pad("StatusBarPad",10);
		S32 segment_width = llclamp(lltrunc((F32)(bar_width) / (F32)button_count), 0, MAX_BUTTON_WIDTH);
		S32 btn_width = segment_width - status_bar_pad;
		S32 left = 0;
		S32 bottom = 1;
		for (LLView* viewp : boost::adaptors::reverse(view_list))
		{
			if(!viewp->getEnabled())
				continue;
			LLRect r = viewp->getRect();
			r.setOriginAndSize(left, bottom, btn_width, r.getHeight());
			viewp->setRect(r);
			left += segment_width;
		}
	}
	layoutNewIM();
}
void LLOverlayBar::layoutNewIM()
{
	if (!mNewIM)
	{
		return;
	}
	const LLFontGL* font = mNewIM->getFont();
	const S32 text_w = font ? font->getWidth(mNewIM->getLabelUnselected()) : 64;
	const S32 pill_h = 28;
	const S32 pad_y = 3;
	const S32 btn_h = pill_h - pad_y * 2;
	const S32 pad_x = 14;
	const S32 btn_w = llmax(text_w + pad_x * 2, btn_h);
	const S32 pill_w = btn_w + pad_y * 2;
	S32 center = getRect().getWidth() / 2;
	S32 bottom = LLFloatingBarStyle::BAR_GAP;
	if (gToolBar && gToolBar->getVisible() && gToolBar->getPillRect().getWidth() > 0)
	{
		const LLRect& pill = gToolBar->getPillRect();
		const LLRect& add = gToolBar->getAddRect();
		const S32 chrome_top = llmax(pill.mTop, add.mTop);
		const S32 inset = llmax(0, gToolBar->getRect().getHeight() - chrome_top);
		center = (pill.mLeft + pill.mRight) / 2;
		bottom = llmax(0, LLFloatingBarStyle::BAR_GAP - inset);
	}
	S32 left = center - pill_w / 2;
	left = llclamp(left, 0, llmax(0, getRect().getWidth() - pill_w));
	if (mChatBar && mChatBar->getVisible())
	{
		S32 chat_left = 0;
		S32 chat_bottom = 0;
		S32 chat_right = 0;
		S32 chat_top = 0;
		mChatBar->localPointToOtherView(0, 0, &chat_left, &chat_bottom, this);
		mChatBar->localPointToOtherView(mChatBar->getRect().getWidth(), mChatBar->getRect().getHeight(), &chat_right, &chat_top, this);
		const S32 btn_right = left + pill_w;
		if (btn_right > chat_left && left < chat_right && bottom < chat_top && bottom + pill_h > chat_bottom)
		{
			bottom = chat_top + LLFloatingBarStyle::BAR_GAP;
		}
	}
	mNewIMPill.setOriginAndSize(left, bottom, pill_w, pill_h);
	const LLRect button_rect(left + pad_y, bottom + pad_y + btn_h, left + pad_y + btn_w, bottom + pad_y);
	if (mNewIM->getRect() != button_rect)
	{
		mNewIM->setShape(button_rect);
	}
}
LLButton* LLOverlayBar::updateButtonVisiblity(LLButton* button, bool visible)
{
	if (button && (bool)button->getVisible() != visible)
	{
		button->setVisible(visible);
		sendChildToFront(button);
		moveChildToBackOfTabGroup(button);
	}
	return button;
}
void LLOverlayBar::refresh()
{
	bool buttons_changed = FALSE;
	int unread_count(gIMMgr->getIMUnreadCount());
	static const LLCachedControl<bool> per_conversation("NewIMsPerConversation");
	static const LLCachedControl<bool> reset_count("NewIMsPerConversationReset");
	if (per_conversation && (!reset_count || unread_count) && !LLFloaterChatterBox::instanceVisible())
	{
		unread_count = 0;
		for(std::set<LLHandle<LLFloater> >::const_iterator it = gIMMgr->getIMFloaterHandles().begin(); it != gIMMgr->getIMFloaterHandles().end(); ++it)
			if (LLFloaterIMPanel* im_floater = static_cast<LLFloaterIMPanel*>(it->get()))
				if (im_floater->getParent() != gFloaterView && im_floater->getNumUnreadMessages())
					++unread_count;
	}
	if (LLButton* button = updateButtonVisiblity(mNewIM, unread_count))
	{
		if (unread_count > 0)
		{
			if (unread_count > 1)
			{
				std::stringstream ss;
				ss << unread_count << " " << mUnreadCountStringPlural;
				button->setLabel(ss.str());
			}
			else
			{
				button->setLabel("1 " + mOriginalIMLabel);
			}
		}
		buttons_changed = true;
	}
	buttons_changed |= updateButtonVisiblity(mNotBusy, gAgent.isDoNotDisturb()) != NULL;
	buttons_changed |= updateButtonVisiblity(mFlyCam,LLViewerJoystick::getInstance()->getOverrideCamera()) != NULL;
	buttons_changed |= updateButtonVisiblity(mStandUp,isAgentAvatarValid() && gAgentAvatarp->isSitting() && !gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) != NULL;
	buttons_changed |= updateButtonVisiblity(mCancelBtn,(gAgent.getTeleportState() >= LLAgent::TELEPORT_START) &&	(gAgent.getTeleportState() <= LLAgent::TELEPORT_MOVING)) != NULL;
	if (mVoiceRemote)
		moveChildToBackOfTabGroup(mVoiceRemote);
	const bool muted = gSavedSettings.getBOOL("MuteAudio");
	if (mVolumeButton && (!mVolumeIconReady || muted != mVolumeMutedIcon))
	{
		mVolumeMutedIcon = muted;
		mVolumeIconReady = true;
		mVolumeButton->setImageOverlay(muted ? "floating_ui/volume_muted.png" : "floating_ui/volume.png", LLFontGL::HCENTER, LLColor4::white);
		mVolumeButton->setImageOverlaySelectedColor(LLFloatingBarStyle::accentColor());
	}
	static BOOL last_mouselook = FALSE;
	BOOL in_mouselook = gAgentCamera.cameraMouselook();
	if (last_mouselook != in_mouselook)
	{
		stashOverlayFloaters(in_mouselook);
		last_mouselook = in_mouselook;
		static const LLCachedControl<bool> enable_ao_remote("EnableAORemote", true);
		mMediaRemoteContainer->setVisible(!in_mouselook);
		mVoiceRemoteContainer->setVisible(!in_mouselook && LLVoiceClient::getInstance()->voiceEnabled());
		mAdvSettingsContainer->setVisible(!in_mouselook);
		mAORemoteContainer->setVisible(!in_mouselook && enable_ao_remote);
		mAOLightContainer->setVisible(!in_mouselook && enable_ao_remote);
		mStateManagementContainer->setVisible(!in_mouselook);
	}
	else if (in_mouselook)
	{
		stashOverlayFloaters(true);
	}
	if (mMouseLook && (bool)mMouseLook->getToggleState() != (bool)in_mouselook)
		mMouseLook->setToggleState(in_mouselook);
	if(!in_mouselook)
		mVoiceRemoteContainer->setVisible(LLVoiceClient::getInstance()->voiceEnabled());
	if (buttons_changed)
	{
		layoutButtons();
	}
}
void LLOverlayBar::onClickIMReceived(void*)
{
	gIMMgr->setFloaterOpen(TRUE);
}
void LLOverlayBar::onClickSetNotBusy(void*)
{
	gAgent.setDoNotDisturb(false);
}
void LLOverlayBar::onClickFlycam(void*)
{
	LLViewerJoystick::getInstance()->toggleFlycam();
}
void LLOverlayBar::onClickResetView(void* data)
{
	handle_reset_view();
}
void LLOverlayBar::onClickMouselook(void*)
{
	if (gAgentCamera.cameraMouselook())
		gAgentCamera.changeCameraToDefault();
	else
		gAgentCamera.changeCameraToMouselook();
}
void LLOverlayBar::onClickStandUp(void*)
{
	if ( (gRlvHandler.hasBehaviour(RLV_BHVR_UNSIT)) && (gAgentAvatarp) && (gAgentAvatarp->isSitting()) )
	{
		return;
	}
	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
}
void LLOverlayBar::onClickCancelTP(void* data)
{
	LLOverlayBar* self = (LLOverlayBar*)data;
	self->setCancelTPButtonVisible(FALSE, std::string("Cancel TP"));
	gAgent.teleportCancel();
	LL_INFOS() << "trying to cancel teleport" << LL_ENDL;
}
void LLOverlayBar::setCancelTPButtonVisible(BOOL b, const std::string& label)
{
	mCancelBtn->setVisible( b );
	mCancelBtn->setLabelSelected(label);
	mCancelBtn->setLabelUnselected(label);
}
void LLOverlayBar::audioFilterPlay()
{
	if (gOverlayBar && gOverlayBar->mMusicState != PLAYING)
	{
		gOverlayBar->mMusicState = PLAYING;
	}
}
void LLOverlayBar::audioFilterStop()
{
	if (gOverlayBar && gOverlayBar->mMusicState != STOPPED)
	{
		gOverlayBar->mMusicState = STOPPED;
	}
}
void LLOverlayBar::mediaStop(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	LLViewerParcelMedia::stop();
}
void LLOverlayBar::toggleMediaPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	if (LLViewerParcelMedia::getStatus() == LLViewerMediaImpl::MEDIA_PAUSED)
	{
		LLViewerParcelMedia::start();
	}
	else if(LLViewerParcelMedia::getStatus() == LLViewerMediaImpl::MEDIA_PLAYING)
	{
		LLViewerParcelMedia::pause();
	}
	else
	{
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		if (parcel)
		{
			LLViewerParcelMedia::play(parcel);
		}
	}
}
void LLOverlayBar::toggleMusicPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	if (gOverlayBar->mMusicState != PLAYING)
	{
		gOverlayBar->mMusicState = PLAYING;
		if (gAudiop)
		{
			LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
			if ( parcel )
			{
				{
					LLViewerParcelMedia::playStreamingMusic(parcel);
				}
			}
		}
	}
	else
	{
		gOverlayBar->mMusicState = STOPPED;
		if (gAudiop)
		{
			gAudiop->stopInternetStream();
		}
	}
}
